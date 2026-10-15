"use client";

import { useEffect, useState, type FormEvent } from "react";
import { useCart } from "@/lib/cart";
import { fetchShops, type Shop } from "@/lib/supabase";
import { useI18n } from "@/i18n/client";
import { fmt, formatPrice } from "@/i18n/format";

export default function CheckoutPage() {
  const { lines, subtotal, clear } = useCart();
  const { dict } = useI18n();
  const [shops, setShops] = useState<Shop[]>([]);
  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [phone, setPhone] = useState("");
  const [shopId, setShopId] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    fetchShops().then((data) => {
      setShops(data);
      if (data.length > 0) setShopId(data[0].id);
    });
  }, []);

  if (lines.length === 0) {
    return (
      <div className="mx-auto max-w-2xl px-6 py-24 text-center">
        <h1 className="font-display text-3xl text-paper">{dict.common.cartEmpty}</h1>
      </div>
    );
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);

    try {
      const res = await fetch("/api/checkout", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          customerName: name,
          customerEmail: email,
          customerPhone: phone || undefined,
          pickupShopId: shopId,
          items: lines.map((line) => ({ productId: line.productId, quantity: line.quantity })),
        }),
      });

      const data = await res.json();
      if (!res.ok) {
        setError(dict.errors[data.code] ?? dict.errors.generic);
        setSubmitting(false);
        return;
      }

      clear();
      window.location.href = data.redirectUrl;
    } catch {
      setError(dict.errors.network);
      setSubmitting(false);
    }
  }

  return (
    <div className="mx-auto max-w-xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">{dict.checkout.title}</h1>
      <p className="mt-2 text-paper-dim">{fmt(dict.cart.subtotal, { amount: formatPrice(dict, subtotal) })}</p>

      <form onSubmit={handleSubmit} className="mt-8 space-y-5">
        <div>
          <label className="block text-sm text-paper-dim">{dict.common.fullName}</label>
          <input
            required
            value={name}
            onChange={(e) => setName(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">{dict.common.email}</label>
          <input
            required
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">{dict.common.phoneOptional}</label>
          <input
            value={phone}
            onChange={(e) => setPhone(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">{dict.checkout.pickupFrom}</label>
          <select
            required
            value={shopId}
            onChange={(e) => setShopId(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          >
            {shops.map((shop) => (
              <option key={shop.id} value={shop.id}>
                {shop.name}
              </option>
            ))}
          </select>
        </div>

        {error && <p className="text-sm text-red-400">{error}</p>}

        <button
          type="submit"
          disabled={submitting || !shopId}
          className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
        >
          {submitting ? dict.checkout.redirecting : dict.checkout.pay}
        </button>
      </form>
    </div>
  );
}
