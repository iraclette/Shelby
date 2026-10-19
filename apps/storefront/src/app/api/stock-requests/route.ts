import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isUuid } from "@/lib/validate";

type StockRequest = {
  productId: string;
  variantId?: string; // the shade/size that's out, for products with variants
  contact: string; // phone or email
};

export async function POST(request: NextRequest) {
  if (!checkRateLimit(`stock-requests:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ code: "rate_limited", error: "Too many requests, try again later." }, { status: 429 });
  }

  const body = (await request.json()) as StockRequest;
  const contact = typeof body.contact === "string" ? body.contact.trim() : "";

  if (
    !isUuid(body.productId) ||
    (body.variantId !== undefined && !isUuid(body.variantId)) ||
    !isNonEmptyString(contact, 254) ||
    contact.length < 5
  ) {
    return NextResponse.json({ code: "invalid_fields", error: "Missing or invalid fields." }, { status: 400 });
  }

  const { error } = await getSupabaseAdmin().from("stock_requests").insert({
    product_id: body.productId,
    variant_id: body.variantId ?? null,
    contact,
  });

  // 23505: they already have an open request for this product, which is fine.
  // 23503: the variant doesn't exist (or was deleted meanwhile).
  if (error?.code === "23503") {
    return NextResponse.json({ code: "invalid_fields", error: "Missing or invalid fields." }, { status: 400 });
  }
  if (error && error.code !== "23505") {
    return NextResponse.json({ code: "send_failed", error: "Could not submit the request." }, { status: 500 });
  }

  return NextResponse.json({ ok: true });
}
