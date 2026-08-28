import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isOptionalString, isUuid, isValidEmail } from "@/lib/validate";

type CustomOrderRequest = {
  customerName: string;
  customerEmail: string;
  customerPhone?: string;
  preferredShopId: string;
  itemDescription: string;
};

export async function POST(request: NextRequest) {
  if (!checkRateLimit(`custom-orders:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ error: "Too many requests, try again later." }, { status: 429 });
  }

  const body = (await request.json()) as CustomOrderRequest;

  if (
    !isNonEmptyString(body.customerName, 200) ||
    !isValidEmail(body.customerEmail) ||
    !isOptionalString(body.customerPhone, 30) ||
    !isUuid(body.preferredShopId) ||
    !isNonEmptyString(body.itemDescription, 2000)
  ) {
    return NextResponse.json({ error: "Missing or invalid fields." }, { status: 400 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const { error } = await supabaseAdmin.from("custom_leather_orders").insert({
    customer_name: body.customerName,
    customer_email: body.customerEmail,
    customer_phone: body.customerPhone ?? null,
    preferred_shop_id: body.preferredShopId,
    item_description: body.itemDescription,
  });

  if (error) {
    return NextResponse.json({ error: "Could not submit the request." }, { status: 500 });
  }

  return NextResponse.json({ ok: true });
}
