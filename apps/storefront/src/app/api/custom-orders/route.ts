import { NextRequest, NextResponse, after } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { notifyStaff, preferredShopText, shopName } from "@/lib/telegram";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isPhoneOrEmail, isUuid, isValidEmail } from "@/lib/validate";

type CustomOrderRequest = {
  customerName: string;
  customerContact: string; // phone or email, whichever the visitor gave
  preferredShopId: string;
  itemDescription: string;
};

export async function POST(request: NextRequest) {
  if (!checkRateLimit(`custom-orders:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ code: "rate_limited", error: "Too many requests, try again later." }, { status: 429 });
  }

  const body = (await request.json()) as CustomOrderRequest;

  const contact = typeof body.customerContact === "string" ? body.customerContact.trim() : "";
  if (
    !isNonEmptyString(body.customerName, 200) ||
    !isPhoneOrEmail(contact) ||
    !isUuid(body.preferredShopId) ||
    !isNonEmptyString(body.itemDescription, 2000)
  ) {
    return NextResponse.json({ code: "invalid_fields", error: "Missing or invalid fields." }, { status: 400 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const { error } = await supabaseAdmin.from("custom_leather_orders").insert({
    customer_name: body.customerName,
    // customer_email is NOT NULL, so a phone-only request stores "" there.
    customer_email: isValidEmail(contact) ? contact : "",
    customer_phone: isValidEmail(contact) ? null : contact,
    preferred_shop_id: body.preferredShopId,
    item_description: body.itemDescription,
  });

  if (error) {
    return NextResponse.json({ code: "send_failed", error: "Could not submit the request." }, { status: 500 });
  }

  after(async () => {
    const shop = await shopName(body.preferredShopId);
    await notifyStaff("custom_order", (lang) => ({
      name: body.customerName,
      contact,
      shop: preferredShopText(lang, shop),
      description: body.itemDescription,
    }));
  });

  return NextResponse.json({ ok: true });
}
