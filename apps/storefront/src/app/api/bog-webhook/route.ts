import { NextRequest, NextResponse, after } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { verifyBogCallbackSignature, getBogOrderStatus } from "@/lib/bog";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { notifyStaff, shopName } from "@/lib/telegram";
import { TELEGRAM_TEXT } from "@/lib/telegramText";
import { fmt } from "@/i18n/format";

export async function POST(request: NextRequest) {
  // Flood protection only; the signature check is the real gate.
  if (!checkRateLimit(`bog-webhook:${getClientIp(request)}`, 30, 60 * 1000)) {
    return NextResponse.json({ error: "Too many requests." }, { status: 429 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const rawBody = await request.text();
  const signature = request.headers.get("Callback-Signature");

  if (!verifyBogCallbackSignature(rawBody, signature)) {
    return NextResponse.json({ error: "Invalid signature." }, { status: 401 });
  }

  const payload = JSON.parse(rawBody) as { body?: { order_id?: string } };
  const bogOrderId = payload.body?.order_id;
  if (!bogOrderId) {
    return NextResponse.json({ error: "Missing order_id." }, { status: 400 });
  }

  // Don't trust the callback; fetch the real status from BOG.
  const status = await getBogOrderStatus(bogOrderId);

  if (status === "completed") {
    const { data: order } = await supabaseAdmin
      .from("orders")
      .select("id, status, customer_name, total, delivery_method, delivery_city, pickup_shop_id")
      .eq("bog_order_id", bogOrderId)
      .maybeSingle();

    if (order) {
      await supabaseAdmin.rpc("confirm_online_order", { p_order_id: order.id });

      // BOG can call back more than once; only the call that actually confirms it notifies.
      if (order.status === "pending") {
        after(async () => {
          const shop = await shopName(order.pickup_shop_id);
          await notifyStaff("online_order", (lang) => ({
            total: Number(order.total).toFixed(2),
            customer: order.customer_name,
            how:
              order.delivery_method === "pickup"
                ? fmt(TELEGRAM_TEXT[lang].pickup, { shop: shop ?? "?" })
                : fmt(TELEGRAM_TEXT[lang].delivery, { city: order.delivery_city ?? "?" }),
          }));
        });
      }
    }
  }

  // BOG requires HTTP 200 to consider the callback delivered, regardless of
  // what we did with it — otherwise it'll keep retrying.
  return NextResponse.json({ ok: true });
}
