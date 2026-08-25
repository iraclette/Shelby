"use client";

import { useEffect, useState, type FormEvent } from "react";
import { fetchShops, type Shop } from "@/lib/supabase";

export default function HandmadeLeatherPage() {
  const [shops, setShops] = useState<Shop[]>([]);
  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [phone, setPhone] = useState("");
  const [shopId, setShopId] = useState("");
  const [description, setDescription] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [submitted, setSubmitted] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    fetchShops().then((data) => {
      setShops(data);
      if (data.length > 0) setShopId(data[0].id);
    });
  }, []);

  if (submitted) {
    return (
      <div className="mx-auto max-w-2xl px-6 py-24 text-center">
        <h1 className="font-display text-3xl text-paper">Request received</h1>
        <p className="mt-4 text-paper-dim">
          Thanks — we&apos;ll be in touch about your handmade leather piece soon.
        </p>
      </div>
    );
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);

    try {
      const res = await fetch("/api/custom-orders", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          customerName: name,
          customerEmail: email,
          customerPhone: phone || undefined,
          preferredShopId: shopId,
          itemDescription: description,
        }),
      });

      const data = await res.json();
      if (!res.ok) {
        setError(data.error ?? "Something went wrong.");
        setSubmitting(false);
        return;
      }

      setSubmitted(true);
    } catch {
      setError("Could not reach the server. Try again.");
      setSubmitting(false);
    }
  }

  return (
    <div className="mx-auto max-w-xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">Handmade Leather</h1>
      <p className="mt-2 text-paper-dim">
        Tell us what you&apos;d like made — a wallet, a belt, or anything else — and we&apos;ll follow up with details and a
        quote.
      </p>

      <form onSubmit={handleSubmit} className="mt-8 space-y-5">
        <div>
          <label className="block text-sm text-paper-dim">Full name</label>
          <input
            required
            value={name}
            onChange={(e) => setName(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">Email</label>
          <input
            required
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">Phone (optional)</label>
          <input
            value={phone}
            onChange={(e) => setPhone(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">Preferred shop</label>
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
        <div>
          <label className="block text-sm text-paper-dim">What would you like made?</label>
          <textarea
            required
            rows={5}
            value={description}
            onChange={(e) => setDescription(e.target.value)}
            placeholder="e.g. a tan leather wallet with 4 card slots and a coin pocket"
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>

        {error && <p className="text-sm text-red-400">{error}</p>}

        <button
          type="submit"
          disabled={submitting || !shopId}
          className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
        >
          {submitting ? "Sending…" : "Send request"}
        </button>
      </form>
    </div>
  );
}
