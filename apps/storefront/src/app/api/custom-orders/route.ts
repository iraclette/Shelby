import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";

type CustomOrderRequest = {
  customerName: string;
  customerEmail: string;
  customerPhone?: string;
  preferredShopId: string;
  itemDescription: string;
};

export async function POST(request: NextRequest) {
  const body = (await request.json()) as CustomOrderRequest;

  if (!body.customerName || !body.customerEmail || !body.preferredShopId || !body.itemDescription) {
    return NextResponse.json({ error: "Missing required fields." }, { status: 400 });
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
