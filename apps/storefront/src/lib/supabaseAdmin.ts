import { createClient, type SupabaseClient } from "@supabase/supabase-js";
import "server-only";

// Service-role client: bypasses RLS entirely. Only ever imported from
// server-side code (API routes) — `server-only` makes it a build error to
// accidentally import this from a Client Component and leak the key.
//
// Lazily constructed on first use rather than at module load: `next build`
// imports route modules just to inspect their exports, and a top-level
// `createClient(...)` call would throw during that step (crashing the
// entire build) whenever the env vars aren't set yet — e.g. before they've
// been added on Render. Deferring the check to actual request handling
// means a missing key only fails the specific route that needs it.
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
