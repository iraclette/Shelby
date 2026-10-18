"use client";

import { useState, type FormEvent } from "react";
import { useI18n } from "@/i18n/client";

// Shown instead of "Add to cart" when nothing is in stock. The shop sees these
// requests in the Shop Console and gets back to the visitor once it's restocked.
export default function StockRequestForm({ productId }: { productId: string }) {
  const { dict } = useI18n();
  const [contact, setContact] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [submitted, setSubmitted] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);

    try {
      const res = await fetch("/api/stock-requests", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ productId, contact }),
      });

      const data = await res.json();
      if (!res.ok) {
        setError(dict.errors[data.code] ?? dict.errors.generic);
        setSubmitting(false);
        return;
      }

      setSubmitted(true);
    } catch {
      setError(dict.errors.network);
      setSubmitting(false);
    }
  }

  return (
    <div className="mt-6 border border-line bg-ink-soft p-5">
      <p className="text-sm tracking-[0.15em] text-paper uppercase">{dict.product.outOfStock}</p>
      {submitted ? (
        <p className="mt-2 text-sm text-paper-dim">{dict.product.notifySent}</p>
      ) : (
        <form onSubmit={handleSubmit} className="mt-2 space-y-3">
          <label htmlFor="stock-request-contact" className="block text-sm text-paper-dim">
            {dict.product.notifyHint}
          </label>
          <input
            id="stock-request-contact"
            required
            minLength={5}
            maxLength={254}
            autoComplete="tel"
            placeholder={dict.product.notifyPlaceholder}
            value={contact}
            onChange={(e) => setContact(e.target.value)}
            className="w-full border border-line bg-ink px-3 py-2 text-paper"
          />
          {error && <p className="text-sm text-red-400">{error}</p>}
          <button
            type="submit"
            disabled={submitting}
            className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink active:translate-y-px disabled:opacity-50"
          >
            {dict.product.notifySubmit}
          </button>
        </form>
      )}
    </div>
  );
}
