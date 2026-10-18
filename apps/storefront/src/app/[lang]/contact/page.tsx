"use client";

import { useState, type FormEvent } from "react";
import { useI18n } from "@/i18n/client";
import { SOCIALS } from "@/lib/business";
import { FacebookIcon, InstagramIcon } from "@/components/SocialIcons";

const socialLinkClass =
  "inline-flex h-11 items-center gap-2 text-sm text-paper-dim transition-colors hover:text-brass";

export default function ContactPage() {
  const { dict } = useI18n();
  const [name, setName] = useState("");
  const [email, setEmail] = useState("");
  const [message, setMessage] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [submitted, setSubmitted] = useState(false);
  const [error, setError] = useState<string | null>(null);

  if (submitted) {
    return (
      <div className="mx-auto max-w-2xl px-6 py-24 text-center">
        <h1 className="font-display text-3xl text-paper">{dict.contact.sentTitle}</h1>
        <p className="mt-4 text-paper-dim">{dict.contact.sentText}</p>
      </div>
    );
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);

    try {
      const res = await fetch("/api/contact", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name, email, message }),
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
    <div className="mx-auto max-w-xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">{dict.contact.title}</h1>
      <p className="mt-2 text-paper-dim">{dict.contact.intro}</p>

      <section className="mt-10">
        <h2 className="text-xs tracking-[0.2em] text-brass uppercase">{dict.contact.messageShop}</h2>
        <ul className="mt-3 divide-y divide-line border-y border-line">
          {Object.entries(SOCIALS).map(([shop, links]) => (
            <li key={shop} className="flex flex-wrap items-center justify-between gap-x-6 py-2">
              <span className="font-display text-lg text-paper">{shop}</span>
              <span className="flex gap-5">
                {links.instagram && (
                  <a href={links.instagram} target="_blank" rel="noopener noreferrer" className={socialLinkClass}>
                    <InstagramIcon className="h-4 w-4" />
                    Instagram
                  </a>
                )}
                {links.facebook && (
                  <a href={links.facebook} target="_blank" rel="noopener noreferrer" className={socialLinkClass}>
                    <FacebookIcon className="h-4 w-4" />
                    Facebook
                  </a>
                )}
              </span>
            </li>
          ))}
        </ul>
      </section>

      <h2 className="mt-12 text-xs tracking-[0.2em] text-brass uppercase">{dict.contact.orWriteHere}</h2>
      <form onSubmit={handleSubmit} className="mt-4 space-y-5">
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
          <label className="block text-sm text-paper-dim">{dict.contact.message}</label>
          <textarea
            required
            rows={5}
            value={message}
            onChange={(e) => setMessage(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>

        {error && <p className="text-sm text-red-400">{error}</p>}

        <button
          type="submit"
          disabled={submitting}
          className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
        >
          {submitting ? dict.common.sending : dict.contact.send}
        </button>
      </form>
    </div>
  );
}
