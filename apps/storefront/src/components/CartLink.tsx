"use client";

import Link from "next/link";
import { useCart } from "@/lib/cart";

export default function CartLink() {
  const { itemCount } = useCart();

  return (
    <Link href="/cart" className="transition-colors hover:text-brass">
      Cart{itemCount > 0 && <span className="text-brass"> ({itemCount})</span>}
    </Link>
  );
}
