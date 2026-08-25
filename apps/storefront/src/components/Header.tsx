"use client";

import { useState } from "react";
import Link from "next/link";
import AdminLink from "./AdminLink";
import CartLink from "./CartLink";

const navLinkClass = "block py-3 transition-colors hover:text-brass md:inline md:py-0";

export default function Header({ showHandmadeLeather }: { showHandmadeLeather: boolean }) {
  const [menuOpen, setMenuOpen] = useState(false);

  return (
    <header className="sticky top-0 z-10 border-b border-line bg-ink/95 backdrop-blur">
      <div className="mx-auto flex max-w-6xl items-center justify-between px-6 py-5">
        <Link href="/" className="font-display text-2xl tracking-wide text-paper" onClick={() => setMenuOpen(false)}>
          Shelby
        </Link>

        <nav className="hidden items-center gap-8 text-sm tracking-[0.15em] text-paper-dim uppercase md:flex">
          <Link href="/" className={navLinkClass}>
            Home
          </Link>
          <Link href="/products" className={navLinkClass}>
            Shop
          </Link>
          {showHandmadeLeather && (
            <Link href="/handmade-leather" className={navLinkClass}>
              Handmade Leather
            </Link>
          )}
          <Link href="/account" className={navLinkClass}>
            Account
          </Link>
          <AdminLink linkClassName={navLinkClass} />
          <CartLink className={navLinkClass} />
        </nav>

        <button
          onClick={() => setMenuOpen((open) => !open)}
          aria-label={menuOpen ? "Close menu" : "Open menu"}
          aria-expanded={menuOpen}
          className="flex h-11 w-11 items-center justify-center text-paper md:hidden"
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

      {menuOpen && (
        <nav className="border-t border-line px-6 pb-4 text-sm tracking-[0.15em] text-paper-dim uppercase md:hidden">
          <Link href="/" className={navLinkClass} onClick={() => setMenuOpen(false)}>
            Home
          </Link>
          <Link href="/products" className={navLinkClass} onClick={() => setMenuOpen(false)}>
            Shop
          </Link>
          {showHandmadeLeather && (
            <Link href="/handmade-leather" className={navLinkClass} onClick={() => setMenuOpen(false)}>
              Handmade Leather
            </Link>
          )}
          <Link href="/account" className={navLinkClass} onClick={() => setMenuOpen(false)}>
            Account
          </Link>
          <AdminLink linkClassName={navLinkClass} onNavigate={() => setMenuOpen(false)} />
          <CartLink className={navLinkClass} onNavigate={() => setMenuOpen(false)} />
        </nav>
      )}
    </header>
  );
}
