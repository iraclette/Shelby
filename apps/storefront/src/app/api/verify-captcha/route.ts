import { NextRequest, NextResponse } from "next/server";
import "server-only";

const VERIFY_URL = "https://challenges.cloudflare.com/turnstile/v0/siteverify";

export async function POST(request: NextRequest) {
  const secretKey = process.env.TURNSTILE_SECRET_KEY;

  // Fails open until Cloudflare is set up (see Turnstile.tsx's matching
  // behavior) — must be configured before real launch, this isn't a
  // long-term stance.
  if (!secretKey) {
    console.warn("TURNSTILE_SECRET_KEY not configured — CAPTCHA check skipped.");
    return NextResponse.json({ ok: true });
  }

  const { token } = (await request.json()) as { token?: string };
  if (!token) {
    return NextResponse.json({ ok: false, error: "Missing CAPTCHA token." }, { status: 400 });
  }

  const res = await fetch(VERIFY_URL, {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: new URLSearchParams({ secret: secretKey, response: token }),
    cache: "no-store",
  });

  const data = (await res.json()) as { success: boolean };
  if (!data.success) {
    return NextResponse.json({ ok: false, error: "CAPTCHA verification failed." }, { status: 400 });
  }

  return NextResponse.json({ ok: true });
}
