import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { sendContactNotification } from "@/lib/email";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isPhoneOrEmail, isValidEmail } from "@/lib/validate";

type ContactRequest = {
  name: string;
  contact: string; // phone or email, whichever the visitor gave
  message: string;
};

export async function POST(request: NextRequest) {
  // Also protects the Resend send quota once a real sending domain replaces
  // the sandbox address (see src/lib/email.ts).
  if (!checkRateLimit(`contact:${getClientIp(request)}`, 5, 10 * 60 * 1000)) {
    return NextResponse.json({ code: "rate_limited", error: "Too many requests, try again later." }, { status: 429 });
  }

  const body = (await request.json()) as ContactRequest;

  const contact = typeof body.contact === "string" ? body.contact.trim() : "";
  if (!isNonEmptyString(body.name, 200) || !isPhoneOrEmail(contact) || !isNonEmptyString(body.message, 5000)) {
    return NextResponse.json({ code: "invalid_fields", error: "Missing or invalid fields." }, { status: 400 });
  }

  const supabaseAdmin = getSupabaseAdmin();
  const { data: inserted, error } = await supabaseAdmin
    .from("contact_messages")
    // The column is still called email (older Shop Consoles read it by that name); it holds either.
    .insert({ name: body.name, email: contact, message: body.message })
    .select("id")
    .single();

  if (error || !inserted) {
    return NextResponse.json({ code: "send_failed", error: "Could not send your message." }, { status: 500 });
  }

  // Best-effort: the message is already safely in the admin's Help page
  // regardless of what happens here.
  try {
    await sendContactNotification({
      name: body.name,
      contact,
      replyTo: isValidEmail(contact) ? contact : undefined,
      message: body.message,
    });
    await supabaseAdmin.from("contact_messages").update({ email_sent: true }).eq("id", inserted.id);
  } catch (err) {
    console.error("Contact notification email failed:", err);
  }

  return NextResponse.json({ ok: true });
}
