"use client";

import { useState } from "react";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";
import Skeleton from "@/components/Skeleton";
import { translateDescription, withStaffEdits, type Descriptions } from "@/lib/descriptions";
import { LOCALE_LABELS, LOCALES, type Locale } from "@/i18n/config";
import { useI18n } from "@/i18n/client";
import { fmt } from "@/i18n/format";

type Product = { id: string; name: string; sku: string };
type Drafts = Record<Locale, string>;

function draftsFrom(descriptions: Descriptions): Drafts {
  return Object.fromEntries(LOCALES.map((lang) => [lang, descriptions[lang]?.text ?? ""])) as Drafts;
}

export default function StaffDescriptionsClient() {
  return <StaffAuthGate>{({ signOut }) => <DescriptionsManager onSignOut={signOut} />}</StaffAuthGate>;
}

function DescriptionsManager({ onSignOut }: { onSignOut: () => void }) {
  const supabase = getSupabaseClient();
  const { dict, lang } = useI18n();

  const [query, setQuery] = useState("");
  const [results, setResults] = useState<Product[]>([]);
  const [searching, setSearching] = useState(false);

  const [selected, setSelected] = useState<Product | null>(null);
  const [descriptions, setDescriptions] = useState<Descriptions | null>(null);
  const [drafts, setDrafts] = useState<Drafts>(draftsFrom({}));
  const [lastEdited, setLastEdited] = useState<Locale>(lang);
  const [busy, setBusy] = useState(false);
  const [statusMessage, setStatusMessage] = useState("");
  const [bulkMessage, setBulkMessage] = useState("");
  const [bulkRunning, setBulkRunning] = useState(false);

  // Products with a description that some language is still missing (e.g. from
  // before translations existed, or saved by an older Shop Console).
  async function translateMissing() {
    setBulkRunning(true);
    setBulkMessage("");
    const { data } = await supabase.from("products").select("id, descriptions").not("descriptions->source", "is", null);
    const pending = ((data as { id: string; descriptions: Descriptions }[]) ?? []).filter((row) =>
      LOCALES.some((locale) => !row.descriptions[locale]?.text),
    );
    let failed = 0;
    for (const [i, row] of pending.entries()) {
      setBulkMessage(fmt(dict.staff.translatingProgress, { done: i, total: pending.length }));
      if (await translateDescription(supabase, row.id)) failed += 1;
    }
    setBulkRunning(false);
    setBulkMessage(
      fmt(failed > 0 ? dict.staff.translatedSomeFailed : dict.staff.translatedAll, {
        n: pending.length - failed,
        failed,
      }),
    );
  }

  async function runSearch(text: string) {
    setQuery(text);
    if (text.trim().length < 2) {
      setResults([]);
      return;
    }
    setSearching(true);
    const { data } = await supabase
      .from("products")
      .select("id, name, sku")
      .ilike("name", `%${text.trim()}%`)
      .order("name")
      .limit(20);
    setResults((data as Product[]) ?? []);
    setSearching(false);
  }

  async function loadDescriptions(productId: string) {
    const { data } = await supabase.from("products").select("descriptions").eq("id", productId).maybeSingle();
    const loaded = (data?.descriptions as Descriptions | null) ?? {};
    setDescriptions(loaded);
    setDrafts(draftsFrom(loaded));
    return loaded;
  }

  function selectProduct(product: Product) {
    setSelected(product);
    setDescriptions(null);
    setStatusMessage("");
    loadDescriptions(product.id);
  }

  async function save() {
    if (!selected || !descriptions) return;
    const edits: Partial<Drafts> = {};
    for (const locale of LOCALES) {
      if (drafts[locale].trim() !== (descriptions[locale]?.text ?? "").trim()) edits[locale] = drafts[locale];
    }
    if (Object.keys(edits).length === 0) return;

    setBusy(true);
    const next = withStaffEdits(descriptions, edits, lastEdited);
    const { error } = await supabase.from("products").update({ descriptions: next }).eq("id", selected.id);
    if (error) {
      setBusy(false);
      setStatusMessage(fmt(dict.staff.descriptionSaveFailed, { error: error.message }));
      return;
    }

    setStatusMessage(dict.staff.translating);
    const translateError = await translateDescription(supabase, selected.id);
    await loadDescriptions(selected.id);
    setBusy(false);
    setStatusMessage(
      translateError ? fmt(dict.staff.translateFailed, { error: translateError }) : dict.staff.translated,
    );
  }

  if (selected) {
    return (
      <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
        <button
          onClick={() => setSelected(null)}
          className="mb-4 min-h-11 text-sm text-paper-dim active:text-brass"
        >
          {dict.staff.backToSearch}
        </button>
        <h1 className="font-display text-2xl text-paper">{selected.name}</h1>
        <p className="text-sm text-paper-dim">{selected.sku}</p>
        <p className="mt-4 text-sm text-paper-dim">{dict.staff.descriptionsHint}</p>

        {!descriptions ? (
          <div className="mt-6 space-y-4" aria-busy="true">
            {LOCALES.map((locale) => (
              <Skeleton key={locale} className="h-28 w-full rounded-md" />
            ))}
          </div>
        ) : (
          <div className="mt-6 space-y-5">
            {LOCALES.map((locale) => {
              const entry = descriptions[locale];
              const badge = !entry
                ? ""
                : entry.auto
                  ? dict.staff.autoTranslated
                  : descriptions.source === locale
                    ? dict.staff.original
                    : dict.staff.writtenByStaff;
              return (
                <div key={locale}>
                  <div className="flex items-center justify-between">
                    <label htmlFor={`description-${locale}`} className="block text-sm text-paper-dim">
                      {LOCALE_LABELS[locale].name}
                    </label>
                    {badge && <span className="text-xs text-brass">{badge}</span>}
                  </div>
                  <textarea
                    id={`description-${locale}`}
                    lang={locale}
                    value={drafts[locale]}
                    onChange={(e) => {
                      setDrafts((current) => ({ ...current, [locale]: e.target.value }));
                      setLastEdited(locale);
                    }}
                    rows={5}
                    disabled={busy}
                    className="mt-1 w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass disabled:opacity-60"
                  />
                </div>
              );
            })}
            <button
              onClick={save}
              disabled={busy}
              className="flex min-h-14 w-full items-center justify-center rounded-md bg-brass text-base font-medium text-ink disabled:opacity-60"
            >
              {busy ? dict.staff.saving : dict.staff.saveDescriptions}
            </button>
            {statusMessage && <p className="text-sm text-paper-dim">{statusMessage}</p>}
          </div>
        )}
      </div>
    );
  }

  return (
    <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper">{dict.staff.productDescriptions}</h1>
        <button onClick={onSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          {dict.common.signOut}
        </button>
      </div>
      <input
        value={query}
        onChange={(e) => runSearch(e.target.value)}
        placeholder={dict.common.searchProducts}
        autoCapitalize="none"
        className="mt-4 w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
      />
      <button
        onClick={translateMissing}
        disabled={bulkRunning}
        className="mt-3 min-h-11 w-full rounded-md border border-line text-sm text-paper active:border-brass disabled:opacity-60"
      >
        {dict.staff.translateMissing}
      </button>
      {bulkMessage && <p className="mt-2 text-sm text-paper-dim">{bulkMessage}</p>}
      <div className="mt-4 space-y-2">
        {searching &&
          results.length === 0 &&
          [0, 1, 2].map((i) => <Skeleton key={i} className="h-14 w-full rounded-md" />)}
        {!searching && query.trim().length >= 2 && results.length === 0 && (
          <p className="text-sm text-paper-dim">{fmt(dict.common.noMatch, { query })}</p>
        )}
        {results.map((product) => (
          <button
            key={product.id}
            onClick={() => selectProduct(product)}
            className="flex min-h-14 w-full items-center justify-between rounded-md border border-line px-4 text-left active:border-brass"
          >
            <span className="text-paper">{product.name}</span>
            <span className="text-xs text-paper-dim">{product.sku}</span>
          </button>
        ))}
      </div>
    </div>
  );
}
