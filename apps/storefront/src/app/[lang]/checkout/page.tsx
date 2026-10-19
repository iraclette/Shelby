"use client";

import { useEffect, useState, type FormEvent } from "react";
import Link from "next/link";
import { useCart } from "@/lib/cart";
import { fetchShops, type Shop } from "@/lib/supabase";
import Skeleton, { FieldSkeleton } from "@/components/Skeleton";
import { useI18n } from "@/i18n/client";
import { fmt, formatPrice } from "@/i18n/format";
import { DELIVERY } from "@/lib/business";

type DeliveryMethod = "pickup" | "inex_branch" | "inex_home";

// Display only: the server works the fee out again and charges that.
const FEES: Record<DeliveryMethod, number> = {
  pickup: 0,
  inex_branch: DELIVERY.branchPrice,
  inex_home: DELIVERY.homePrice,
};

const inputClass = "mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper";

export default function CheckoutPage() {
  const { lines, subtotal, clear, hydrated } = useCart();
  const { dict } = useI18n();
  const [shops, setShops] = useState<Shop[]>([]);
  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [phone, setPhone] = useState("");
  const [shopId, setShopId] = useState("");
  const [method, setMethod] = useState<DeliveryMethod>("pickup");
  const [city, setCity] = useState("");
  const [address, setAddress] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    fetchShops().then((data) => {
      setShops(data);
      if (data.length > 0) setShopId(data[0].id);
    });
  }, []);

  if (!hydrated) {
    return (
      <div className="mx-auto max-w-xl space-y-5 px-6 py-16" aria-busy="true">
        <Skeleton className="h-9 w-48" />
        <Skeleton className="h-5 w-40" />
        <div className="space-y-5 pt-3">
          {[0, 1, 2, 3].map((i) => (
            <FieldSkeleton key={i} />
          ))}
        </div>
      </div>
    );
  }

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
          deliveryMethod: method,
          pickupShopId: method === "pickup" ? shopId : undefined,
          deliveryCity: method === "pickup" ? undefined : city,
          deliveryAddress: method === "pickup" ? undefined : address || undefined,
          items: lines.map((line) => ({ productId: line.productId, variantId: line.variantId ?? undefined, quantity: line.quantity })),
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

  const isDelivery = method !== "pickup";
  const fee = FEES[method];
  const options: { value: DeliveryMethod; label: string; hint: string; price: string }[] = [
    { value: "pickup", label: dict.checkout.pickupOption, hint: "", price: dict.checkout.free },
    { value: "inex_branch", label: dict.checkout.branchOption, hint: dict.checkout.branchHint, price: formatPrice(dict, FEES.inex_branch) },
    { value: "inex_home", label: dict.checkout.homeOption, hint: dict.checkout.homeHint, price: formatPrice(dict, FEES.inex_home) },
  ];

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
          <label className="block text-sm text-paper-dim">
            {isDelivery ? dict.checkout.phoneForCourier : dict.common.phoneOptional}
          </label>
          <input
            required={isDelivery}
            type="tel"
            value={phone}
            onChange={(e) => setPhone(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <fieldset>
          <legend className="text-sm text-paper-dim">{dict.checkout.howToGet}</legend>
          <div className="mt-2 space-y-2">
            {options.map((option) => (
              <label
                key={option.value}
                className={`flex cursor-pointer items-start gap-3 border px-4 py-3 transition-colors ${
                  method === option.value ? "border-brass bg-ink-soft" : "border-line hover:border-paper-dim"
                }`}
              >
                <input
                  type="radio"
                  name="delivery"
                  value={option.value}
                  checked={method === option.value}
                  onChange={() => setMethod(option.value)}
                  className="mt-1 accent-[var(--color-brass)]"
                />
                <span className="flex-1">
                  <span className="block text-paper">{option.label}</span>
                  {option.hint && <span className="block text-xs text-paper-dim">{option.hint}</span>}
                </span>
                <span className="text-sm text-brass">{option.price}</span>
              </label>
            ))}
          </div>
        </fieldset>

        {method === "pickup" ? (
          <div>
            <label className="block text-sm text-paper-dim">{dict.checkout.pickupFrom}</label>
            {shops.length === 0 ? (
              <Skeleton className="mt-1 h-10.5 w-full" />
            ) : (
              <select required value={shopId} onChange={(e) => setShopId(e.target.value)} className={inputClass}>
                {shops.map((shop) => (
                  <option key={shop.id} value={shop.id}>
                    {shop.name}
                  </option>
                ))}
              </select>
            )}
          </div>
        ) : (
          <>
            <div>
              <label className="block text-sm text-paper-dim">{dict.checkout.city}</label>
              <input required value={city} onChange={(e) => setCity(e.target.value)} className={inputClass} />
            </div>
            <div>
              <label className="block text-sm text-paper-dim">
                {method === "inex_home" ? dict.checkout.address : dict.checkout.branchNote}
              </label>
              <input
                required={method === "inex_home"}
                value={address}
                onChange={(e) => setAddress(e.target.value)}
                className={inputClass}
              />
            </div>
          </>
        )}

        <dl className="space-y-1 border-t border-line pt-4 text-sm">
          <div className="flex justify-between text-paper-dim">
            <dt>{dict.checkout.items}</dt>
            <dd>{formatPrice(dict, subtotal)}</dd>
          </div>
          <div className="flex justify-between text-paper-dim">
            <dt>{dict.checkout.delivery}</dt>
            <dd>{fee > 0 ? formatPrice(dict, fee) : dict.checkout.free}</dd>
          </div>
          <div className="flex justify-between text-base text-paper">
            <dt>{dict.checkout.total}</dt>
            <dd>{formatPrice(dict, subtotal + fee)}</dd>
          </div>
        </dl>

        {error && <p className="text-sm text-red-400">{error}</p>}

        <button
          type="submit"
          disabled={submitting || (method === "pickup" && !shopId)}
          className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
        >
          {submitting ? dict.checkout.redirecting : dict.checkout.pay}
        </button>
        <p className="text-center text-xs text-paper-dim">
          {dict.checkout.agree}{" "}
          <Link href="/terms" className="underline hover:text-brass">
            {dict.footer.terms}
          </Link>
        </p>
      </form>
    </div>
  );
}
