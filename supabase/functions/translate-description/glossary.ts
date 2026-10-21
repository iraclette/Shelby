// Shop vocabulary Microsoft Translator gets wrong, found by translating every
// description on the site (2026-10-21). Three layers, used by translator.ts:
//
// 1. Brand and product names (every Latin-script phrase in a Georgian or
//    Russian description: "Antler", "Al Nimr", "Eau de Parfum") are kept
//    exactly as written, via Translator's dynamic-dictionary markup.
// 2. Georgian → English: exact Georgian word forms come out as written here.
//    (Not used for Russian: a fixed Russian form breaks the agreement around
//    it, so Russian is translated from the corrected English instead.)
// 3. After translating: whole-word and word-stem fixes. Stem fixes keep the
//    ending, so every form stays right: долговечный → стойкий, Долговечность →
//    Стойкость.
//
// Add to these lists as staff spot new mistakes; the descriptions page lets
// them fix a translation by hand in the meantime.

export type Lang = "ka" | "en" | "ru";

// Georgian word forms, matched whole (Georgian has no letter case).
const KA_TO_EN: Record<string, string> = {
  // The scent trail (sillage). Came out "slide", "flight", "Sleigh".
  შლეიფი: "sillage",
  შლეიფს: "sillage",
  შლეიფისთვის: "sillage",
  შლეიფით: "sillage",
  // Longevity of a perfume; came out "sustainable"/"Sustainability".
  მდგრადობა: "Longevity",
  მდგრადი: "long-lasting",
  მდგრად: "long-lasting",
  // Of the sillage; came out "guiding".
  გამყოლი: "long-lasting",
  // The bottle; came out "vial".
  ფლაკონი: "bottle",
  ფლაკონში: "bottle",
  // Scent families; came out "chipril" and "fugire".
  შიპრული: "chypre",
  ფუჟერული: "fougère",
  ოლფაქტორული: "olfactory",
  ნოტები: "notes",
  ნოტი: "note",
  // The fragrance, never "flavor".
  არომატი: "fragrance",
  არომატის: "fragrance",
  არომატს: "the fragrance",
  ნეროლი: "neroli",
  ავაზა: "cheetah", // came out "Thief"
  ბრენდი: "Brand",
  უნისექსადაც: "as a unisex fragrance",
  ნუსხავს: "captivates",
  გადაწყვეტილი: "finished",
  გალესილი: "sharpened", // cuticle nippers; came out "Plastered"
  // Makeup setting spray; came out "Spray Fixator".
  "მაკიაჟის შესასხურებელი ფიქსატორი": "makeup setting spray",
  "მაკიაჟის ფიქსაციის სპრეი": "makeup setting spray",
  "ფიქსაციის სპრეს": "setting spray",
};

const WORD_FIXES: Record<Lang, Record<string, string>> = {
  ka: {},
  en: {
    vial: "bottle",
    vials: "bottles",
    Olfactorial: "Olfactory",
    guayac: "guaiac",
    "non-stick": "non-sticky", // makeup texture, not a frying pan
  },
  ru: {
    Долголетие: "Стойкость",
    примечания: "ноты", // the notes of a perfume
    заметки: "ноты",
    Клеймо: "Бренд", // "Brand"
    Silage: "шлейф",
    Sylage: "шлейф",
    "Heart Note": "Нота сердца",
    "Heart Notes": "Ноты сердца",
  },
};

const STEM_FIXES: Record<Lang, Record<string, string>> = {
  ka: {},
  en: {},
  ru: {
    устойчив: "стойк", // longevity, not stability
    долговечн: "стойк",
    затяжн: "стойк",
    силляж: "шлейф", // before силл, which would leave силляж → шлейфяж
    силлаж: "шлейф",
    силл: "шлейф",
    антипригарн: "нелипк", // a non-sticky texture, not a non-stick pan
  },
};

const GEORGIAN_LETTER = "\\u10A0-\\u10FF\\u1C90-\\u1CBF";
const LATIN_WORD = String.raw`[A-Za-z][A-Za-z0-9'’&™]*(?:[.\-][A-Za-z0-9]+)*`;
const LATIN_PHRASE = new RegExp(`${LATIN_WORD}(?: +(?:${LATIN_WORD}|[0-9]+))*`, "g");

const escapeRegExp = (s: string) => s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
const dictionary = (source: string, translation: string) =>
  `<mstrans:dictionary translation="${translation}">${source}</mstrans:dictionary>`;
const keepAsIs = (phrase: string) => (phrase.includes('"') ? phrase : dictionary(phrase, phrase));

// Brand and product names in a Georgian or Russian text.
export function latinPhrases(text: string): string[] {
  return [...new Set(text.match(LATIN_PHRASE) ?? [])].filter((phrase) => phrase.length >= 2);
}

// Text to send for a direct translation out of Georgian or Russian.
// Every replacement is a single pass over the original text, so nothing is
// ever matched inside markup an earlier replacement inserted.
const anyOf = (words: string[]) =>
  [...words].sort((a, b) => b.length - a.length).map(escapeRegExp).join("|"); // longest first

const KA_TERMS = new RegExp(`(?<![${GEORGIAN_LETTER}])(?:${anyOf(Object.keys(KA_TO_EN))})(?![${GEORGIAN_LETTER}])`, "g");

export function prepare(text: string, from: Lang, to: Lang): string {
  if (from === "en") return text; // everything is Latin script; see preparePivot
  const names = LATIN_PHRASE.source;
  const pattern = from === "ka" && to === "en" ? new RegExp(`${names}|${KA_TERMS.source}`, "g") : LATIN_PHRASE;
  return text.replace(pattern, (match) => {
    if (match in KA_TO_EN) return dictionary(match, KA_TO_EN[match]);
    return match.length >= 2 ? keepAsIs(match) : match;
  });
}

// English text on its way to Russian: keep the names the Georgian original had
// in Latin script, which English → Russian would otherwise translate
// ("Antler" → «Рог»).
export function preparePivot(english: string, names: string[]): string {
  if (names.length === 0) return english;
  return english.replace(new RegExp(`(?<![A-Za-z])(?:${anyOf(names)})(?![A-Za-z])`, "g"), keepAsIs);
}

const sameCase = (original: string, replacement: string) =>
  original[0] === original[0].toUpperCase() ? replacement[0].toUpperCase() + replacement.slice(1) : replacement;

export function fixOutput(text: string, to: Lang): string {
  // Translator returns markup it couldn't parse as-is; never show it.
  let fixed = text.replace(/<mstrans:dictionary translation="[^"]*"?>([\s\S]*?)<\/mstrans:dictionary>/g, "$1");
  for (const [wrong, right] of Object.entries(WORD_FIXES[to])) {
    fixed = fixed.replace(new RegExp(`(?<!\\p{L})${escapeRegExp(wrong)}(?!\\p{L})`, "gu"), right);
  }
  for (const [stem, replacement] of Object.entries(STEM_FIXES[to])) {
    const pattern = `(?<!\\p{L})[${stem[0]}${stem[0].toUpperCase()}]${escapeRegExp(stem.slice(1))}`;
    fixed = fixed.replace(new RegExp(pattern, "gu"), (match) => sameCase(match, replacement));
  }
  // Russian spelling after к: стойкый → стойкий, нелипкых → нелипких.
  if (to === "ru") fixed = fixed.replace(/(стойк|нелипк)ы/giu, "$1и");
  return fixed;
}
