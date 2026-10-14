import { createClient, type SupabaseClient } from "@supabase/supabase-js";
import "server-only";

// Service-role client, bypasses RLS. Server only.
// Created lazily so `next build` doesn't crash without env vars.
let cached: SupabaseClient | null = null;

export function getSupabaseAdmin(): SupabaseClient {
  if (cached) return cached;

  const url = process.env.NEXT_PUBLIC_SUPABASE_URL;
  const key = process.env.SUPABASE_SECRET_KEY;
  if (!url || !key) {
    throw new Error("Missing NEXT_PUBLIC_SUPABASE_URL or SUPABASE_SECRET_KEY.");
  }

  cached = createClient(url, key);
  return cached;
}
