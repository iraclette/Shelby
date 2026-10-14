"use client";

import { useEffect, useState } from "react";
import Link from "next/link";
import { getSupabaseClient } from "@/lib/supabase";

// Staff nav links, only rendered for owner/admin profiles.
export default function AdminLink({
  linkClassName = "transition-colors hover:text-brass",
  onNavigate,
}: {
  linkClassName?: string;
  onNavigate?: () => void;
}) {
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
      <Link href="/staff/photos" className={linkClassName} onClick={onNavigate}>
        Photos
      </Link>
      <Link href="/staff/inventory" className={linkClassName} onClick={onNavigate}>
        Inventory
      </Link>
    </>
  );
}
