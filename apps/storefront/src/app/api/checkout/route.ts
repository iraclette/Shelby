import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { createBogOrder } from "@/lib/bog";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isOptionalString, isUuid, isValidEmail } from "@/lib/validate";
import { DELIVERY } from "@/lib/business";

type DeliveryMethod = "pickup" | "inex_branch" | "inex_home";

type CheckoutRequest = {
  customerName: string;
  customerEmail: string;
  customerPhone?: string;
  deliveryMethod?: DeliveryMethod; // absent = pickup, for carts opened before delivery existed
  pickupShopId?: string; // pickup only
  deliveryCity?: string; // Inex only
  deliveryAddress?: string; // street address for home delivery, preferred branch (optional) for branch
  items: { productId: string; variantId?: string; quantity: number }[];
};

const DELIVERY_FEES: Record<DeliveryMethod, number> = {
  pickup: 0,
  inex_branch: DELIVERY.branchPrice,
  inex_home: DELIVERY.homePrice,
};

export async function POST(request: NextRequest) {
  // Hits the live BOG payment API twice per call, so this is the most
  // expensive route to spam — keep the window tight.
  if (!checkRateLimit(`checkout:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ code: "rate_limited", error: "Too many requests, try again later." }, { status: 429 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const body = (await request.json()) as CheckoutRequest;
  const method: DeliveryMethod = body.deliveryMethod ?? "pickup";
  const isDelivery = method !== "pickup";

  if (
    !(method in DELIVERY_FEES) ||
    !isNonEmptyString(body.customerName, 200) ||
    !isValidEmail(body.customerEmail) ||
    // The courier calls before delivering, so a phone is required for Inex.
    (isDelivery ? !isNonEmptyString(body.customerPhone, 30) : !isOptionalString(body.customerPhone, 30)) ||
    (method === "pickup" && !isUuid(body.pickupShopId)) ||
    (isDelivery && !isNonEmptyString(body.deliveryCity, 100)) ||
    (method === "inex_home" ? !isNonEmptyString(body.deliveryAddress, 300) : !isOptionalString(body.deliveryAddress, 300)) ||
    !body.items?.length ||
    !body.items.every(
      (item) =>
        isUuid(item.productId) &&
        (item.variantId === undefined || isUuid(item.variantId)) &&
        Number.isInteger(item.quantity) &&
        item.quantity >= 1 &&
        item.quantity <= 100,
    )
  ) {
    return NextResponse.json({ code: "invalid_fields", error: "Missing or invalid fields." }, { status: 400 });
  }

  // Never trust prices from the client — look up each product's current
  // sell_price server-side and compute the total from that.
  // Two shades of one product are two lines with the same productId.
  const productIds = [...new Set(body.items.map((item) => item.productId))];
  const { data: products, error: productsError } = await supabaseAdmin
    .from("products")
    .select("id, sell_price, categories(visible_on_storefront), product_variants(id)")
    .in("id", productIds);

  // A cart saved before its category was hidden from the website can't be bought online.
  const hidden = (products ?? []).some(
    (p) => (p.categories as unknown as { visible_on_storefront: boolean } | null)?.visible_on_storefront === false,
  );
  // A product with variants is bought as one of them; one without, as itself.
  const variantIdsByProduct = new Map(
    (products ?? []).map((p) => [p.id as string, (p.product_variants as { id: string }[]).map((v) => v.id)]),
  );
  const variantsMatch = body.items.every((item) => {
    const variantIds = variantIdsByProduct.get(item.productId) ?? [];
    return variantIds.length === 0 ? item.variantId === undefined : variantIds.includes(item.variantId ?? "");
  });
  if (productsError || !products || products.length !== productIds.length || hidden || !variantsMatch) {
    return NextResponse.json({ code: "product_not_found", error: "One or more products could not be found." }, { status: 400 });
  }

  const priceById = new Map(products.map((p) => [p.id as string, p.sell_price as number]));
  const basket = body.items.map((item) => ({
    productId: item.productId,
    variantId: item.variantId ?? null,
    quantity: item.quantity,
    unitPrice: priceById.get(item.productId)!,
  }));
  const deliveryFee = DELIVERY_FEES[method];
  // Rounded to tetri: summed floats can land on 39.500000001, which BOG would reject.
  const total = Math.round((basket.reduce((sum, item) => sum + item.quantity * item.unitPrice, 0) + deliveryFee) * 100) / 100;

  const { data: levels, error: levelsError } = await supabaseAdmin
    .from("inventory_levels")
    .select("shop_id, product_id, variant_id, quantity")
    .in("product_id", productIds);
  if (levelsError) {
    return NextResponse.json({ code: "order_failed", error: "Could not create the order." }, { status: 500 });
  }
  // shop -> stockKey -> units. Products with variants keep their stock per variant.
  const stockKey = (productId: string, variantId?: string | null) => (variantId ? `${productId}:${variantId}` : productId);
  const stock = new Map<string, Map<string, number>>();
  for (const level of levels ?? []) {
    const shopStock = stock.get(level.shop_id as string) ?? new Map<string, number>();
    const key = stockKey(level.product_id as string, level.variant_id as string | null);
    shopStock.set(key, (shopStock.get(key) ?? 0) + (level.quantity as number));
    stock.set(level.shop_id as string, shopStock);
  }

  // Delivery orders ship from whichever shop holds the most of what was ordered;
  // that shop's stock is what confirm_online_order() takes from once it's paid.
  let fulfilmentShopId = body.pickupShopId ?? "";
  if (isDelivery) {
    const coverage = [...stock.entries()].map(([shopId, shopStock]) => [
      shopId,
      body.items.reduce(
        (sum, item) =>
          sum + Math.max(0, Math.min(shopStock.get(stockKey(item.productId, item.variantId)) ?? 0, item.quantity)),
        0,
      ),
    ] as const);
    fulfilmentShopId = coverage.sort((a, b) => b[1] - a[1])[0]?.[0] ?? "";
  }

  // Everything has to come from that one shop, since that's where the stock is taken from.
  const shopStock = stock.get(fulfilmentShopId);
  const short = body.items.filter((item) => (shopStock?.get(stockKey(item.productId, item.variantId)) ?? 0) < item.quantity);
  if (short.length > 0) {
    return NextResponse.json(
      {
        code: isDelivery ? "out_of_stock" : "out_of_stock_pickup",
        error: "Not enough stock for some items.",
        productIds: short.map((item) => item.productId),
      },
      { status: 409 },
    );
  }

  const { data: order, error: orderError } = await supabaseAdmin
    .from("orders")
    .insert({
      customer_name: body.customerName,
      customer_email: body.customerEmail,
      customer_phone: body.customerPhone || null,
      pickup_shop_id: fulfilmentShopId,
      delivery_method: method,
      delivery_fee: deliveryFee,
      delivery_city: isDelivery ? body.deliveryCity!.trim() : null,
      delivery_address: isDelivery ? body.deliveryAddress?.trim() || null : null,
      total,
    })
    .select("id")
    .single();

  if (orderError || !order) {
    return NextResponse.json({ code: "order_failed", error: "Could not create the order." }, { status: 500 });
  }

  const { error: itemsError } = await supabaseAdmin.from("order_items").insert(
    basket.map((item) => ({
      order_id: order.id,
      product_id: item.productId,
      variant_id: item.variantId,
      quantity: item.quantity,
      unit_price: item.unitPrice,
    })),
  );

  if (itemsError) {
    await supabaseAdmin.from("orders").update({ status: "failed" }).eq("id", order.id);
    return NextResponse.json({ code: "order_failed", error: "Could not create the order." }, { status: 500 });
  }

  const origin = request.nextUrl.origin;

  try {
    const { orderId: bogOrderId, redirectUrl } = await createBogOrder({
      externalOrderId: order.id,
      totalAmount: total,
      // BOG wants the basket to add up to the total, so delivery goes in as its own line.
      basket: deliveryFee > 0 ? [...basket, { productId: "delivery", quantity: 1, unitPrice: deliveryFee }] : basket,
      callbackUrl: `${origin}/api/bog-webhook`,
      successUrl: `${origin}/checkout/success?order=${order.id}`,
      failUrl: `${origin}/checkout/fail?order=${order.id}`,
    });

    await supabaseAdmin.from("orders").update({ bog_order_id: bogOrderId }).eq("id", order.id);

    return NextResponse.json({ redirectUrl });
  } catch (err) {
    await supabaseAdmin.from("orders").update({ status: "failed" }).eq("id", order.id);
    const message = err instanceof Error ? err.message : "Payment provider error.";
    return NextResponse.json({ code: "payment_failed", error: message }, { status: 502 });
  }
}
