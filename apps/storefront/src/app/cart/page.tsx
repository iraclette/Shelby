"use client";

import Link from "next/link";
import Image from "next/image";
import { useCart } from "@/lib/cart";
import { productImageUrl } from "@/lib/supabase";

export default function CartPage() {
  const { lines, setQuantity, removeItem, subtotal } = useCart();

  if (lines.length === 0) {
    return (
      <div className="mx-auto max-w-2xl px-6 py-24 text-center">
        <h1 className="font-display text-3xl text-paper">Your cart is empty</h1>
        <Link href="/products" className="mt-6 inline-block text-brass hover:underline">
          Browse the collection
        </Link>
      </div>
    );
  }

  return (
    <div className="mx-auto max-w-3xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">Your Cart</h1>

      <div className="mt-8 divide-y divide-line border-y border-line">
        {lines.map((line) => (
          <div key={line.productId} className="flex gap-4 py-4">
            <div className="relative h-20 w-20 flex-shrink-0 overflow-hidden border border-line bg-ink-elevated">
              {line.imagePath && (
                <Image
                  src={productImageUrl(line.imagePath)}
                  alt={line.name}
                  fill
                  sizes="80px"
                  className="object-cover"
                />
              )}
            </div>
            <div className="min-w-0 flex-1">
              <p className="truncate text-paper">{line.name}</p>
              <p className="text-sm text-paper-dim">GEL {line.unitPrice.toFixed(2)}</p>
              <div className="mt-2 flex flex-wrap items-center gap-3">
                <input
                  type="number"
                  min={1}
                  value={line.quantity}
                  onChange={(e) => setQuantity(line.productId, parseInt(e.target.value, 10) || 0)}
                  className="w-16 border border-line bg-ink-soft px-2 py-1 text-center text-paper"
                />
                <p className="text-paper">GEL {(line.quantity * line.unitPrice).toFixed(2)}</p>
                <button
                  onClick={() => removeItem(line.productId)}
                  className="text-sm text-paper-dim hover:text-brass"
                >
                  Remove
                </button>
              </div>
            </div>
          </div>
        ))}
      </div>

      <div className="mt-6 flex items-center justify-between">
        <p className="text-lg text-paper">Subtotal: GEL {subtotal.toFixed(2)}</p>
        <Link
          href="/checkout"
          className="border border-brass px-8 py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink"
        >
          Checkout
        </Link>
      </div>
    </div>
  );
}
