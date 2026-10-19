import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { checkRateLimit, getClientIp } from "@/lib/rateLimit";
import { isNonEmptyString, isOptionalString, isUuid, isValidEmail } from "@/lib/validate";

type JobApplicationRequest = {
  name: string;
  phone: string;
  email?: string;
  preferredShopId?: string; // omitted = any shop
  availability: string;
  languages: string[];
  about: string;
  adult: boolean;
};

const AVAILABILITY = ["full_time", "part_time", "either"];
const LANGUAGES = ["ka", "ru", "en"];

export async function POST(request: NextRequest) {
  if (!checkRateLimit(`jobs:${getClientIp(request)}`, 3, 10 * 60 * 1000)) {
    return NextResponse.json({ code: "rate_limited", error: "Too many requests, try again later." }, { status: 429 });
  }

  const body = (await request.json()) as JobApplicationRequest;

  if (
    !isNonEmptyString(body.name, 200) ||
    !isNonEmptyString(body.phone, 30) ||
    !isOptionalString(body.email, 254) ||
    (body.email && !isValidEmail(body.email)) ||
    (body.preferredShopId !== undefined && !isUuid(body.preferredShopId)) ||
    !AVAILABILITY.includes(body.availability) ||
    !Array.isArray(body.languages) ||
    !body.languages.every((lang) => LANGUAGES.includes(lang)) ||
    !isNonEmptyString(body.about, 2000) ||
    body.adult !== true
  ) {
    return NextResponse.json({ code: "invalid_fields", error: "Missing or invalid fields." }, { status: 400 });
  }

  const { error } = await getSupabaseAdmin().from("job_applications").insert({
    name: body.name.trim(),
    phone: body.phone.trim(),
    email: body.email || null,
    preferred_shop_id: body.preferredShopId ?? null,
    availability: body.availability,
    languages: [...new Set(body.languages)],
    about: body.about.trim(),
  });

  if (error) {
    console.error("Job application insert failed:", error);
    return NextResponse.json({ code: "send_failed", error: "Could not submit the application." }, { status: 500 });
  }

  return NextResponse.json({ ok: true });
}
