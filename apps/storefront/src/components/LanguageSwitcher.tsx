"use client";

import { LANG_COOKIE, LOCALES, LOCALE_LABELS, type Locale } from "@/i18n/config";
import { useI18n } from "@/i18n/client";

function saveLocale(locale: Locale) {
  document.cookie = `${LANG_COOKIE}=${locale}; path=/; max-age=${60 * 60 * 24 * 365}; samesite=lax`;
}

// Saves the choice in the cookie the proxy reads, then reloads so every server-rendered
// part of the page comes back in the new language. The cart is in localStorage, so it survives.
export default function LanguageSwitcher({ className = "" }: { className?: string }) {
  const { lang, dict } = useI18n();

  function choose(locale: Locale) {
    if (locale === lang) return;
    saveLocale(locale);
    window.location.reload();
  }

  return (
    <div role="group" aria-label={dict.nav.language} className={`flex items-center gap-1 normal-case tracking-normal ${className}`}>
      {LOCALES.map((locale) => (
        <button
          key={locale}
          type="button"
          lang={locale}
          onClick={() => choose(locale)}
          aria-pressed={locale === lang}
          title={LOCALE_LABELS[locale].name}
          className={`min-h-11 min-w-11 px-2 text-xs transition-colors lg:min-h-0 lg:min-w-0 lg:py-1 ${
            locale === lang ? "text-brass" : "text-paper-dim hover:text-paper"
          }`}
        >
          {LOCALE_LABELS[locale].short}
        </button>
      ))}
    </div>
  );
}
