import "server-only";

// Best-effort: the message is already saved to contact_messages before this runs.
const RESEND_API_URL = "https://api.resend.com/emails";

export async function sendContactNotification(params: { name: string; email: string; message: string }): Promise<void> {
  const apiKey = process.env.RESEND_API_KEY;
  const to = process.env.CONTACT_EMAIL;
  if (!apiKey || !to) {
    throw new Error("RESEND_API_KEY / CONTACT_EMAIL are not configured.");
  }

  const res = await fetch(RESEND_API_URL, {
    method: "POST",
    headers: {
      Authorization: `Bearer ${apiKey}`,
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      // onboarding@resend.dev only delivers to the Resend account's own email until a domain is verified.
      from: "Shelby Storefront <onboarding@resend.dev>",
      to: [to],
      reply_to: params.email,
      subject: `Contact form: ${params.name}`,
      text: `From: ${params.name} <${params.email}>\n\n${params.message}`,
    }),
    cache: "no-store",
  });

  if (!res.ok) {
    throw new Error(`Resend send failed: ${res.status} ${await res.text()}`);
  }
}
