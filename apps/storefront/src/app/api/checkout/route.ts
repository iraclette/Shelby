import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { createBogOrder } from "@/lib/bog";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isOptionalString, isUuid, isValidEmail } from "@/lib/validate";

type CheckoutRequest = {
  customerName: string;
  customerEmail: string;
  customerPhone?: string;
  pickupShopId: string;
  items: { productId: string; quantity: number }[];
};

export async function POST(request: NextRequest) {
  // Hits the live BOG payment API twice per call, so this is the most
  // expensive route to spam — keep the window tight.
  if (!checkRateLimit(`checkout:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ error: "Too many requests, try again later." }, { status: 429 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const body = (await request.json()) as CheckoutRequest;

  if (
    !isNonEmptyString(body.customerName, 200) ||
    !isValidEmail(body.customerEmail) ||
    !isOptionalString(body.customerPhone, 30) ||
    !isUuid(body.pickupShopId) ||
    !body.items?.length ||
    !body.items.every(
      (item) => isUuid(item.productId) && Number.isInteger(item.quantity) && item.quantity >= 1 && item.quantity <= 100,
    )
  ) {
    return NextResponse.json({ error: "Missing or invalid fields." }, { status: 400 });
  }

  // Never trust prices from the client — look up each product's current
  // sell_price server-side and compute the total from that.
  const productIds = body.items.map((item) => item.productId);
  const { data: products, error: productsError } = await supabaseAdmin
    .from("products")
    .select("id, sell_price")
    .in("id", productIds);

  if (productsError || !products || products.length !== productIds.length) {
    return NextResponse.json({ error: "One or more products could not be found." }, { status: 400 });
  }

  const priceById = new Map(products.map((p) => [p.id as string, p.sell_price as number]));
  const basket = body.items.map((item) => ({
    productId: item.productId,
    quantity: item.quantity,
    unitPrice: priceById.get(item.productId)!,
  }));
  const total = basket.reduce((sum, item) => sum + item.quantity * item.unitPrice, 0);

  const { data: order, error: orderError } = await supabaseAdmin
    .from("orders")
    .insert({
      customer_name: body.customerName,
      customer_email: body.customerEmail,
      customer_phone: body.customerPhone ?? null,
      pickup_shop_id: body.pickupShopId,
      total,
    })
    .select("id")
    .single();

  if (orderError || !order) {
    return NextResponse.json({ error: "Could not create the order." }, { status: 500 });
  }

  const { error: itemsError } = await supabaseAdmin.from("order_items").insert(
    basket.map((item) => ({
      order_id: order.id,
      product_id: item.productId,
      quantity: item.quantity,
      unit_price: item.unitPrice,
    })),
  );

  if (itemsError) {
    await supabaseAdmin.from("orders").update({ status: "failed" }).eq("id", order.id);
    return NextResponse.json({ error: "Could not create the order." }, { status: 500 });
  }

  const origin = request.nextUrl.origin;

  try {
    const { orderId: bogOrderId, redirectUrl } = await createBogOrder({
      externalOrderId: order.id,
      totalAmount: total,
      basket,
      callbackUrl: `${origin}/api/bog-webhook`,
      successUrl: `${origin}/checkout/success?order=${order.id}`,
      failUrl: `${origin}/checkout/fail?order=${order.id}`,
    });

    await supabaseAdmin.from("orders").update({ bog_order_id: bogOrderId }).eq("id", order.id);

    return NextResponse.json({ redirectUrl });
  } catch (err) {
    await supabaseAdmin.from("orders").update({ status: "failed" }).eq("id", order.id);
    const message = err instanceof Error ? err.message : "Payment provider error.";
    return NextResponse.json({ error: message }, { status: 502 });
  }
}
