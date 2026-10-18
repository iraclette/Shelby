"use client";

import Link from "next/link";
import Image from "next/image";
import { useCart } from "@/lib/cart";
import { productImageUrl } from "@/lib/supabase";
import Skeleton from "@/components/Skeleton";
import { useI18n } from "@/i18n/client";
import { fmt, formatPrice } from "@/i18n/format";

export default function CartPage() {
  const { lines, setQuantity, removeItem, subtotal, hydrated } = useCart();
  const { dict } = useI18n();

  if (!hydrated) {
    return (
      <div className="mx-auto max-w-3xl px-6 py-16" aria-busy="true">
        <Skeleton className="h-9 w-48" />
        <div className="mt-8 divide-y divide-line border-y border-line">
          {[0, 1].map((i) => (
            <div key={i} className="flex gap-4 py-4">
              <Skeleton className="h-20 w-20 flex-shrink-0" />
              <div className="flex-1 space-y-2">
                <Skeleton className="h-5 w-1/2" />
                <Skeleton className="h-4 w-20" />
                <Skeleton className="h-7 w-40" />
              </div>
            </div>
          ))}
        </div>
      </div>
    );
  }

  if (lines.length === 0) {
    return (
      <div className="mx-auto max-w-2xl px-6 py-24 text-center">
        <h1 className="font-display text-3xl text-paper">{dict.common.cartEmpty}</h1>
        <Link href="/products" className="mt-6 inline-block text-brass hover:underline">
          {dict.common.browseCollection}
        </Link>
      </div>
    );
  }

  return (
    <div className="mx-auto max-w-3xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">{dict.cart.title}</h1>

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
              <p className="text-sm text-paper-dim tabular-nums">{formatPrice(dict, line.unitPrice)}</p>
              <div className="mt-2 flex flex-wrap items-center gap-3">
                <input
                  type="number"
                  min={1}
                  aria-label={dict.cart.quantity}
                  value={line.quantity}
                  onChange={(e) => setQuantity(line.productId, parseInt(e.target.value, 10) || 0)}
                  className="w-16 border border-line bg-ink-soft px-2 py-1 text-center text-paper"
                />
                <p className="text-paper tabular-nums">{formatPrice(dict, line.quantity * line.unitPrice)}</p>
                <button
                  onClick={() => removeItem(line.productId)}
                  className="text-sm text-paper-dim hover:text-brass"
                >
                  {dict.cart.remove}
                </button>
              </div>
            </div>
          </div>
        ))}
      </div>

      <div className="mt-6 flex flex-wrap items-center justify-between gap-4">
        <div>
          <p className="text-lg text-paper tabular-nums">{fmt(dict.cart.subtotal, { amount: formatPrice(dict, subtotal) })}</p>
          <p className="mt-1 text-sm text-paper-dim">{dict.cart.deliveryHint}</p>
        </div>
        <Link
          href="/checkout"
          className="border border-brass px-8 py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink active:translate-y-px"
        >
          {dict.cart.checkout}
        </Link>
      </div>
    </div>
  );
}
