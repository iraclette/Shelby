"use client";

import { useState, type FormEvent } from "react";
import type { Shop } from "@/lib/supabase";
import { useI18n } from "@/i18n/client";

const inputClass = "mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper";
const LANGUAGES = ["ka", "ru", "en"] as const;

export default function JobsForm({ shops }: { shops: Shop[] }) {
  const { dict } = useI18n();
  const [name, setName] = useState("");
  const [phone, setPhone] = useState("");
  const [email, setEmail] = useState("");
  const [shopId, setShopId] = useState(""); // "" = any shop
  const [availability, setAvailability] = useState("either");
  const [languages, setLanguages] = useState<string[]>(["ka"]);
  const [about, setAbout] = useState("");
  const [adult, setAdult] = useState(false);
  const [submitting, setSubmitting] = useState(false);
  const [submitted, setSubmitted] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const languageLabels = { ka: dict.jobs.langKa, ru: dict.jobs.langRu, en: dict.jobs.langEn };
  const availabilityOptions = [
    ["full_time", dict.jobs.fullTime],
    ["part_time", dict.jobs.partTime],
    ["either", dict.jobs.either],
  ] as const;

  if (submitted) {
    return (
      <div className="self-start border border-line bg-ink-soft px-6 py-14 text-center">
        <h2 className="font-display text-2xl text-paper">{dict.jobs.sentTitle}</h2>
        <p className="mt-3 text-paper-dim">{dict.jobs.sentText}</p>
      </div>
    );
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);

    try {
      const res = await fetch("/api/jobs", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name,
          phone,
          email: email || undefined,
          preferredShopId: shopId || undefined,
          availability,
          languages,
          about,
          adult,
        }),
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
    <form onSubmit={handleSubmit} className="space-y-5">
      <div>
        <label className="block text-sm text-paper-dim">{dict.common.fullName}</label>
        <input required value={name} onChange={(e) => setName(e.target.value)} className={inputClass} />
      </div>
      <div className="grid gap-5 sm:grid-cols-2">
        <div>
          <label className="block text-sm text-paper-dim">{dict.jobs.phone}</label>
          <input
            required
            type="tel"
            autoComplete="tel"
            value={phone}
            onChange={(e) => setPhone(e.target.value)}
            className={inputClass}
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">{dict.jobs.emailOptional}</label>
          <input type="email" value={email} onChange={(e) => setEmail(e.target.value)} className={inputClass} />
        </div>
      </div>
      <div>
        <label className="block text-sm text-paper-dim">{dict.jobs.shop}</label>
        <select value={shopId} onChange={(e) => setShopId(e.target.value)} className={inputClass}>
          <option value="">{dict.jobs.anyShop}</option>
          {shops.map((shop) => (
            <option key={shop.id} value={shop.id}>
              {shop.name}
            </option>
          ))}
        </select>
      </div>
      <fieldset>
        <legend className="text-sm text-paper-dim">{dict.jobs.availability}</legend>
        <div className="mt-2 grid grid-cols-3 gap-2">
          {availabilityOptions.map(([value, label]) => (
            <label
              key={value}
              className={`flex cursor-pointer items-center justify-center border px-2 py-2.5 text-center text-sm transition-colors has-[:focus-visible]:outline-2 has-[:focus-visible]:outline-offset-2 has-[:focus-visible]:outline-brass ${
                availability === value ? "border-brass bg-ink-soft text-paper" : "border-line text-paper-dim hover:border-paper-dim"
              }`}
            >
              <input
                type="radio"
                name="availability"
                value={value}
                checked={availability === value}
                onChange={() => setAvailability(value)}
                className="sr-only"
              />
              {label}
            </label>
          ))}
        </div>
      </fieldset>
      <fieldset>
        <legend className="text-sm text-paper-dim">{dict.jobs.languages}</legend>
        <div className="mt-2 flex flex-wrap gap-x-6 gap-y-2">
          {LANGUAGES.map((lang) => (
            <label key={lang} className="flex cursor-pointer items-center gap-2 text-paper">
              <input
                type="checkbox"
                checked={languages.includes(lang)}
                onChange={(e) =>
                  setLanguages((current) =>
                    e.target.checked ? [...current, lang] : current.filter((l) => l !== lang),
                  )
                }
                className="accent-[var(--color-brass)]"
              />
              {languageLabels[lang]}
            </label>
          ))}
        </div>
      </fieldset>
      <div>
        <label className="block text-sm text-paper-dim">{dict.jobs.about}</label>
        <textarea
          required
          rows={5}
          maxLength={2000}
          value={about}
          onChange={(e) => setAbout(e.target.value)}
          placeholder={dict.jobs.aboutPlaceholder}
          className={inputClass}
        />
      </div>
      <label className="flex cursor-pointer items-center gap-3 text-paper">
        <input
          required
          type="checkbox"
          checked={adult}
          onChange={(e) => setAdult(e.target.checked)}
          className="accent-[var(--color-brass)]"
        />
        {dict.jobs.adult}
      </label>

      {error && <p className="text-sm text-red-400">{error}</p>}

      <button
        type="submit"
        disabled={submitting}
        className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink active:translate-y-px disabled:opacity-50"
      >
        {submitting ? dict.common.sending : dict.jobs.send}
      </button>
    </form>
  );
}
