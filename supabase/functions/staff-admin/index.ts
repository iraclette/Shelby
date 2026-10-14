// Creates/deletes Shop Console staff logins. Verifies the caller is an admin,
// then uses the service-role key (only available here).

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const ANON_KEY = Deno.env.get("SUPABASE_ANON_KEY")!;
const SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;

function json(body: unknown, status: number) {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

Deno.serve(async (req) => {
  try {
    const authHeader = req.headers.get("Authorization") ?? "";

    // Scoped to the caller's own token — RLS applies, so this can only ever
    // read the caller's own profile row, never anyone else's.
    const callerClient = createClient(SUPABASE_URL, ANON_KEY, {
      global: { headers: { Authorization: authHeader } },
    });

    const { data: userData, error: userError } = await callerClient.auth.getUser();
    if (userError || !userData.user) {
      return json({ error: "Not signed in." }, 401);
    }

    const { data: callerProfile, error: profileError } = await callerClient
      .from("profiles")
      .select("role")
      .eq("id", userData.user.id)
      .maybeSingle();

    if (profileError || !callerProfile || !["owner", "admin"].includes(callerProfile.role)) {
      return json({ error: "Not authorized." }, 403);
    }

    const body = await req.json();
    const admin = createClient(SUPABASE_URL, SERVICE_ROLE_KEY);

    // Best-effort; never fails the account operation.
    async function logAudit(action: string, entityId: string, detail: string) {
      try {
        await admin.from("audit_log").insert({
          actor_id: userData.user!.id,
          action,
          entity_type: "profile",
          entity_id: entityId,
          detail,
        });
      } catch (_err) {
        // fire and forget
      }
    }

    if (body.action === "create") {
      const { username, password, full_name, role, shop_id } = body;
      if (!username || !password || !role) {
        return json({ error: "username, password, and role are required." }, 400);
      }

      // Mirrors the desktop login convention: a short username, not a real
      // email — Supabase Auth still needs an email shape underneath.
      const email = String(username).includes("@") ? username : `${username}@shop.local`;

      const { data: created, error: createError } = await admin.auth.admin.createUser({
        email,
        password,
        email_confirm: true,
        user_metadata: { full_name },
      });
      if (createError || !created.user) {
        return json({ error: createError?.message ?? "Could not create the account." }, 400);
      }

      // handle_new_user() already created the profile; set the requested fields.
      const { error: updateError } = await admin
        .from("profiles")
        .update({ role, shop_id: shop_id || null })
        .eq("id", created.user.id);
      if (updateError) {
        return json({ error: updateError.message }, 400);
      }

      await logAudit("staff_created", created.user.id, `Created ${full_name || username} (${role})`);

      return json({ ok: true, id: created.user.id });
    }

    if (body.action === "delete") {
      const { profile_id } = body;
      if (!profile_id) {
        return json({ error: "profile_id is required." }, 400);
      }
      // Fetched before the delete so the audit detail can name who was
      // removed — deleteUser cascades the profiles row away immediately.
      const { data: deletedProfile } = await admin
        .from("profiles")
        .select("full_name")
        .eq("id", profile_id)
        .maybeSingle();

      // profiles.id references auth.users(id) on delete cascade, so
      // deleting the auth user removes the profile row too.
      const { error: deleteError } = await admin.auth.admin.deleteUser(profile_id);
      if (deleteError) {
        return json({ error: deleteError.message }, 400);
      }

      await logAudit("staff_deleted", profile_id, `Removed ${deletedProfile?.full_name ?? profile_id}`);

      return json({ ok: true }, 200);
    }

    return json({ error: `Unknown action: ${body.action}` }, 400);
  } catch (err) {
    return json({ error: err instanceof Error ? err.message : String(err) }, 500);
  }
});
