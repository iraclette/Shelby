"use client";

import { useEffect, useState } from "react";
import Link from "next/link";
import { getSupabaseClient } from "@/lib/supabase";

// Shows the staff-only nav links (Photos, Inventory), but only once the
// signed-in session belongs to a staff profile with role owner/admin — a
// customer session (or no session at all) renders nothing. Same Supabase
// Auth session as /account and /staff/photos; which table it resolves to
// (customers vs profiles) is what decides which of those a person actually
// is, not which login form they used.
export default function AdminLink() {
  const supabase = getSupabaseClient();
  const [isAdmin, setIsAdmin] = useState(false);

  useEffect(() => {
    async function checkRole(userId: string | undefined) {
      if (!userId) {
        setIsAdmin(false);
        return;
      }
      const { data } = await supabase.from("profiles").select("role").eq("id", userId).maybeSingle();
      setIsAdmin(data?.role === "owner" || data?.role === "admin");
    }

    supabase.auth.getSession().then(({ data }) => checkRole(data.session?.user.id));
    const { data: sub } = supabase.auth.onAuthStateChange((_event, session) => {
      checkRole(session?.user.id);
    });
    return () => sub.subscription.unsubscribe();
  }, [supabase]);

  if (!isAdmin) return null;

  return (
    <>
      <Link href="/staff/photos" className="transition-colors hover:text-brass">
        Photos
      </Link>
      <Link href="/staff/inventory" className="transition-colors hover:text-brass">
        Inventory
      </Link>
    </>
  );
}
