import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { sendContactNotification } from "@/lib/email";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isValidEmail } from "@/lib/validate";

type ContactRequest = {
  name: string;
  email: string;
  message: string;
};

export async function POST(request: NextRequest) {
  // Also protects the Resend send quota once a real sending domain replaces
  // the sandbox address (see src/lib/email.ts).
  if (!checkRateLimit(`contact:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ error: "Too many requests, try again later." }, { status: 429 });
  }

  const body = (await request.json()) as ContactRequest;

  if (!isNonEmptyString(body.name, 200) || !isValidEmail(body.email) || !isNonEmptyString(body.message, 5000)) {
    return NextResponse.json({ error: "Missing or invalid fields." }, { status: 400 });
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
