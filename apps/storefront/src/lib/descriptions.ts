import type { SupabaseClient } from "@supabase/supabase-js";
import type { Locale } from "@/i18n/config";

// products.descriptions (0038_description_translations.sql): one entry per
// language, "auto" when the translate-description edge function wrote it.
export type DescriptionEntry = { text: string; auto: boolean };
export type Descriptions = { source?: Locale } & Partial<Record<Locale, DescriptionEntry>>;

// The visitor's language, else whatever staff wrote.
export function descriptionFor(
  product: { description: string | null; descriptions: Descriptions | null },
  locale: Locale,
): string | null {
  return product.descriptions?.[locale]?.text || product.description;
}

// Applies what staff typed: a language with text becomes staff-written, a
// cleared one goes back to being translated. The source (what the others are
// translated from) is the preferred language if staff wrote in it, else
// another edited one, else the current source.
export function withStaffEdits(
  current: Descriptions,
  edits: Partial<Record<Locale, string>>,
  preferredSource: Locale,
): Descriptions {
  const next: Descriptions = { ...current };
  for (const [lang, text] of Object.entries(edits) as [Locale, string][]) {
    if (text.trim()) next[lang] = { text: text.trim(), auto: false };
    else delete next[lang];
  }
  const written = (Object.keys(edits) as Locale[]).filter((lang) => next[lang]);
  const source = written.includes(preferredSource)
    ? preferredSource
    : (written[0] ?? (current.source && next[current.source] ? current.source : undefined));
  // Nothing written by staff is left, so neither are translations of it.
  return source ? { ...next, source } : {};
}

// Fills in the other languages. Takes a few seconds; resolves to an error
// message, or null when done.
export async function translateDescription(supabase: SupabaseClient, productId: string): Promise<string | null> {
  const { error } = await supabase.functions.invoke("translate-description", { body: { product_id: productId } });
  return error ? error.message : null;
}
