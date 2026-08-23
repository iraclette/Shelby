import { createClient } from "@supabase/supabase-js";
import "server-only";

// Service-role client: bypasses RLS entirely. Only ever imported from
// server-side code (API routes) — `server-only` makes it a build error to
// accidentally import this from a Client Component and leak the key.
export const supabaseAdmin = createClient(
  process.env.NEXT_PUBLIC_SUPABASE_URL!,
  process.env.SUPABASE_SECRET_KEY!,
);
