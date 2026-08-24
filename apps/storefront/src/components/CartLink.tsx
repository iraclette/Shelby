"use client";

import Link from "next/link";
import { useCart } from "@/lib/cart";

export default function CartLink({
  className = "transition-colors hover:text-brass",
  onNavigate,
}: {
  className?: string;
  onNavigate?: () => void;
}) {
  const { itemCount } = useCart();

  return (
    <Link href="/cart" className={className} onClick={onNavigate}>
      Cart{itemCount > 0 && <span className="text-brass"> ({itemCount})</span>}
    </Link>
  );
}
