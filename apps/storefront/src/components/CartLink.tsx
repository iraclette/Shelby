"use client";

import Link from "next/link";
import { useCart } from "@/lib/cart";
import { useI18n } from "@/i18n/client";

export default function CartLink({
  className = "transition-colors hover:text-brass",
  onNavigate,
}: {
  className?: string;
  onNavigate?: () => void;
}) {
  const { itemCount } = useCart();
  const { dict } = useI18n();

  return (
    <Link href="/cart" className={className} onClick={onNavigate}>
      {dict.nav.cart}{itemCount > 0 && <span className="text-brass"> ({itemCount})</span>}
    </Link>
  );
}
