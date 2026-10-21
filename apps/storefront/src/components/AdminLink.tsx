"use client";

import { useEffect, useRef, useState, type ReactNode } from "react";
import Link from "next/link";
import { getSupabaseClient } from "@/lib/supabase";
import { useI18n } from "@/i18n/client";

// Staff nav links, only rendered for owner/admin profiles.
export default function AdminLink({
  linkClassName = "transition-colors hover:text-brass",
  onNavigate,
  dropdown = false,
}: {
  linkClassName?: string;
  onNavigate?: () => void;
  dropdown?: boolean; // collapse the links under one "Staff" menu
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

  const links = (
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
  if (!dropdown) return links;

  // The desktop header has no room for five more links next to the shop's own.
  return <StaffMenu label={dict.nav.staff}>{links}</StaffMenu>;
}

function StaffMenu({ label, children }: { label: string; children: ReactNode }) {
  const menu = useRef<HTMLDetailsElement>(null);
  const close = () => menu.current?.removeAttribute("open");

  // Close when clicking anywhere else, like any other dropdown.
  useEffect(() => {
    function onPointerDown(event: PointerEvent) {
      if (menu.current && !menu.current.contains(event.target as Node)) close();
    }
    document.addEventListener("pointerdown", onPointerDown);
    return () => document.removeEventListener("pointerdown", onPointerDown);
  }, []);

  return (
    <details ref={menu} className="group relative">
      <summary className="flex cursor-pointer list-none items-center gap-1 whitespace-nowrap transition-colors hover:text-brass [&::-webkit-details-marker]:hidden">
        {label}
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" className="h-3 w-3 transition-transform group-open:rotate-180">
          <path strokeLinecap="round" d="M6 9l6 6 6-6" />
        </svg>
      </summary>
      <div
        onClick={close}
        className="absolute right-0 mt-4 flex min-w-52 flex-col gap-4 border border-line bg-ink px-5 py-4 whitespace-nowrap shadow-lg"
      >
        {children}
      </div>
    </details>
  );
}
