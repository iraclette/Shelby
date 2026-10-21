// Shared by the proxy, server components and client components, so no server-only imports here.

export const LOCALES = ["en", "ka", "ru"] as const;
export type Locale = (typeof LOCALES)[number];

export const DEFAULT_LOCALE: Locale = "en";
export const LANG_COOKIE = "shelby-lang";

// Each language named in itself, as the switcher shows it.
export const LOCALE_LABELS: Record<Locale, { short: string; name: string }> = {
  en: { short: "EN", name: "English" },
  ka: { short: "ქარ", name: "ქართული" },
  ru: { short: "РУ", name: "Русский" },
};

// "/ru/products/1" -> "/products/1"; unprefixed paths come back unchanged.
export function stripLocale(pathname: string): string {
  const prefix = LOCALES.find((locale) => pathname === `/${locale}` || pathname.startsWith(`/${locale}/`));
  return prefix ? pathname.slice(prefix.length + 1) || "/" : pathname;
}

export function hasLocale(value: string | undefined | null): value is Locale {
  return LOCALES.includes(value as Locale);
}
