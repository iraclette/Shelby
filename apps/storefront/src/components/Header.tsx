"use client";

import { Suspense, use, useState } from "react";
import Image from "next/image";
import Link from "next/link";
import { usePathname } from "next/navigation";
import AdminLink from "./AdminLink";
import CartLink from "./CartLink";
import LanguageSwitcher from "./LanguageSwitcher";
import { useI18n } from "@/i18n/client";
import { stripLocale } from "@/i18n/config";

const navLinkClass = "block py-3 whitespace-nowrap transition-colors hover:text-brass lg:inline lg:py-0";
const activeNavLinkClass = `${navLinkClass} text-paper`;

// URLs carry no language prefix (see proxy.ts), so the pathname compares directly.
function useNavClass() {
  const pathname = stripLocale(usePathname()); // /ru/products when opened from a search result
  return (href: string) => {
    const active = href === "/" ? pathname === "/" : pathname === href || pathname.startsWith(`${href}/`);
    return { className: active ? activeNavLinkClass : navLinkClass, "aria-current": active ? ("page" as const) : undefined };
  };
}

function HandmadeLeatherLink({ show, onClick }: { show: Promise<boolean>; onClick?: () => void }) {
  const { dict } = useI18n();
  const navClass = useNavClass();
  if (!use(show)) return null;
  return (
    <Link href="/handmade-leather" {...navClass("/handmade-leather")} onClick={onClick}>
      {dict.nav.handmadeLeather}
    </Link>
  );
}

export default function Header({ showHandmadeLeather }: { showHandmadeLeather: Promise<boolean> }) {
  const [menuOpen, setMenuOpen] = useState(false);
  const { dict } = useI18n();
  const navClass = useNavClass();

  return (
    <header className="sticky top-0 z-10 border-b border-line bg-ink/95 backdrop-blur">
      <div className="mx-auto flex max-w-6xl items-center justify-between px-6 py-5">
        <Link
          href="/"
          className="flex shrink-0 items-center gap-3 font-display text-2xl tracking-wide text-paper"
          onClick={() => setMenuOpen(false)}
        >
          <Image src="/logo.png" alt="" width={40} height={40} priority className="h-10 w-10" />
          Shelby
        </Link>

        <nav className="hidden items-center gap-8 text-sm tracking-[0.15em] text-paper-dim uppercase lg:flex">
          <Link href="/" {...navClass("/")}>
            {dict.nav.home}
          </Link>
          <Link href="/products" {...navClass("/products")}>
            {dict.nav.shop}
          </Link>
          <Suspense fallback={null}>
            <HandmadeLeatherLink show={showHandmadeLeather} />
          </Suspense>
          <Link href="/account" {...navClass("/account")}>
            {dict.nav.account}
          </Link>
          <AdminLink linkClassName={navLinkClass} dropdown />
          <CartLink className={navLinkClass} />
          <LanguageSwitcher className="-mx-2" />
        </nav>

        <div className="flex items-center lg:hidden">
          <LanguageSwitcher />
          <button
            onClick={() => setMenuOpen((open) => !open)}
            aria-label={menuOpen ? dict.nav.closeMenu : dict.nav.openMenu}
            aria-expanded={menuOpen}
            className="flex h-11 w-11 items-center justify-center text-paper lg:hidden"
          >
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" className="h-6 w-6">
              {menuOpen ? (
                <path strokeLinecap="round" d="M6 6l12 12M18 6L6 18" />
              ) : (
                <path strokeLinecap="round" d="M3 6h18M3 12h18M3 18h18" />
              )}
            </svg>
          </button>
        </div>
      </div>

      {menuOpen && (
        <nav className="border-t border-line px-6 pb-4 text-sm tracking-[0.15em] text-paper-dim uppercase lg:hidden">
          <Link href="/" {...navClass("/")} onClick={() => setMenuOpen(false)}>
            {dict.nav.home}
          </Link>
          <Link href="/products" {...navClass("/products")} onClick={() => setMenuOpen(false)}>
            {dict.nav.shop}
          </Link>
          <Suspense fallback={null}>
            <HandmadeLeatherLink show={showHandmadeLeather} onClick={() => setMenuOpen(false)} />
          </Suspense>
          <Link href="/account" {...navClass("/account")} onClick={() => setMenuOpen(false)}>
            {dict.nav.account}
          </Link>
          <AdminLink linkClassName={navLinkClass} onNavigate={() => setMenuOpen(false)} />
          <CartLink className={navLinkClass} onNavigate={() => setMenuOpen(false)} />
        </nav>
      )}
    </header>
  );
}
