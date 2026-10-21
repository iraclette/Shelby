"use client";

import { useEffect, useState } from "react";
import Link from "next/link";
import { getSupabaseClient } from "@/lib/supabase";
import { useI18n } from "@/i18n/client";

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
  const { dict } = useI18n();

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
      <Link href="/staff/new-product" className={linkClassName} onClick={onNavigate}>
        {dict.nav.addProduct}
      </Link>
      <Link href="/staff/photos" className={linkClassName} onClick={onNavigate}>
        {dict.nav.photos}
      </Link>
      <Link href="/staff/descriptions" className={linkClassName} onClick={onNavigate}>
        {dict.nav.descriptions}
      </Link>
      <Link href="/staff/inventory" className={linkClassName} onClick={onNavigate}>
        {dict.nav.inventory}
      </Link>
      <Link href="/staff/notifications" className={linkClassName} onClick={onNavigate}>
        {dict.nav.notifications}
      </Link>
    </>
  );
}
