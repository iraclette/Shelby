import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { verifyBogCallbackSignature, getBogOrderStatus } from "@/lib/bog";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";

export async function POST(request: NextRequest) {
  // Signature verification below is the real gate; this is just cheap flood
  // insurance, generous enough that legitimate BOG callback traffic never
  // hits it.
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

  // The callback only says "something happened for this order" — it's not
  // trusted as the payment outcome itself. Re-fetch the authoritative
  // status directly from BOG before confirming anything.
  const status = await getBogOrderStatus(bogOrderId);

  if (status === "completed") {
    const { data: order } = await supabaseAdmin
      .from("orders")
      .select("id")
      .eq("bog_order_id", bogOrderId)
      .maybeSingle();

    if (order) {
      await supabaseAdmin.rpc("confirm_online_order", { p_order_id: order.id });
    }
  }

  // BOG requires HTTP 200 to consider the callback delivered, regardless of
  // what we did with it — otherwise it'll keep retrying.
  return NextResponse.json({ ok: true });
}
