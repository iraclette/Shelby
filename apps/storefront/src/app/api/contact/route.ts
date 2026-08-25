import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { sendContactNotification } from "@/lib/email";

type ContactRequest = {
  name: string;
  email: string;
  message: string;
};

export async function POST(request: NextRequest) {
  const body = (await request.json()) as ContactRequest;

  if (!body.name || !body.email || !body.message) {
    return NextResponse.json({ error: "Missing required fields." }, { status: 400 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const { data: inserted, error } = await supabaseAdmin
    .from("contact_messages")
    .insert({ name: body.name, email: body.email, message: body.message })
    .select("id")
    .single();

  if (error || !inserted) {
    return NextResponse.json({ error: "Could not send your message." }, { status: 500 });
  }

  // Best-effort: the message is already safely in the admin's Help page
  // regardless of what happens here.
  try {
    await sendContactNotification({ name: body.name, email: body.email, message: body.message });
    await supabaseAdmin.from("contact_messages").update({ email_sent: true }).eq("id", inserted.id);
  } catch (err) {
    console.error("Contact notification email failed:", err);
  }

  return NextResponse.json({ ok: true });
}
