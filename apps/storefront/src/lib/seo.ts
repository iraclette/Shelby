import type { Metadata } from "next";
import { LOCALES, type Locale } from "@/i18n/config";

export const SITE_URL = (process.env.NEXT_PUBLIC_SITE_URL ?? "https://shelby.ge").replace(/\/$/, "");

export const OG_LOCALES: Record<Locale, string> = { en: "en_US", ka: "ka_GE", ru: "ru_RU" };

const localized = (locale: Locale, path: string) => `/${locale}${path === "/" ? "" : path}`;

// For a page that reads the same in every language: canonical is this
// language's own URL, and hreflang points search engines at the other two.
// x-default is the plain URL, which picks a language per visitor.
export function languageAlternates(path: string, locale: Locale): Metadata["alternates"] {
  return {
    canonical: localized(locale, path),
    languages: {
      ...Object.fromEntries(LOCALES.map((l) => [l, localized(l, path)])),
      "x-default": path,
    },
  };
}

// One line of plain text, cut at a word, for meta descriptions.
export function snippet(text: string | null | undefined, max = 160): string | undefined {
  if (!text) return undefined;
  const flat = text.replace(/\s+/g, " ").trim();
  if (flat.length <= max) return flat;
  return flat.slice(0, flat.lastIndexOf(" ", max - 1)).replace(/[,.;:–—-]+$/, "") + "…";
}
