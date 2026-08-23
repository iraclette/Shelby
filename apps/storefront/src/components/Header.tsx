import Link from "next/link";
import CartLink from "./CartLink";

export default function Header() {
  return (
    <header className="sticky top-0 z-10 border-b border-line bg-ink/95 backdrop-blur">
      <div className="mx-auto flex max-w-6xl items-center justify-between px-6 py-5">
        <Link href="/" className="font-display text-2xl tracking-wide text-paper">
          Shelby
        </Link>
        <nav className="flex items-center gap-8 text-sm tracking-[0.15em] text-paper-dim uppercase">
          <Link href="/" className="transition-colors hover:text-brass">
            Home
          </Link>
          <Link href="/products" className="transition-colors hover:text-brass">
            Shop
          </Link>
          <CartLink />
        </nav>
      </div>
    </header>
  );
}
