"use client";

import { Suspense, use, useState } from "react";
import Link from "next/link";
import AdminLink from "./AdminLink";
import CartLink from "./CartLink";
import LanguageSwitcher from "./LanguageSwitcher";
import { useI18n } from "@/i18n/client";

const navLinkClass = "block py-3 transition-colors hover:text-brass lg:inline lg:py-0";

function HandmadeLeatherLink({ show, onClick }: { show: Promise<boolean>; onClick?: () => void }) {
  const { dict } = useI18n();
  if (!use(show)) return null;
  return (
    <Link href="/handmade-leather" className={navLinkClass} onClick={onClick}>
      {dict.nav.handmadeLeather}
    </Link>
  );
}

export default function Header({ showHandmadeLeather }: { showHandmadeLeather: Promise<boolean> }) {
  const [menuOpen, setMenuOpen] = useState(false);
  const { dict } = useI18n();

  return (
    <header className="sticky top-0 z-10 border-b border-line bg-ink/95 backdrop-blur">
      <div className="mx-auto flex max-w-6xl items-center justify-between px-6 py-5">
        <Link href="/" className="font-display text-2xl tracking-wide text-paper" onClick={() => setMenuOpen(false)}>
          Shelby
        </Link>

        <nav className="hidden items-center gap-8 text-sm tracking-[0.15em] text-paper-dim uppercase lg:flex">
          <Link href="/" className={navLinkClass}>
            {dict.nav.home}
          </Link>
          <Link href="/products" className={navLinkClass}>
            {dict.nav.shop}
          </Link>
          <Suspense fallback={null}>
            <HandmadeLeatherLink show={showHandmadeLeather} />
          </Suspense>
          <Link href="/account" className={navLinkClass}>
            {dict.nav.account}
          </Link>
          <AdminLink linkClassName={navLinkClass} />
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
          <Link href="/" className={navLinkClass} onClick={() => setMenuOpen(false)}>
            {dict.nav.home}
          </Link>
          <Link href="/products" className={navLinkClass} onClick={() => setMenuOpen(false)}>
            {dict.nav.shop}
          </Link>
          <Suspense fallback={null}>
            <HandmadeLeatherLink show={showHandmadeLeather} onClick={() => setMenuOpen(false)} />
          </Suspense>
          <Link href="/account" className={navLinkClass} onClick={() => setMenuOpen(false)}>
            {dict.nav.account}
          </Link>
          <AdminLink linkClassName={navLinkClass} onNavigate={() => setMenuOpen(false)} />
          <CartLink className={navLinkClass} onNavigate={() => setMenuOpen(false)} />
        </nav>
      )}
    </header>
  );
}
