// Microsoft Translator calls for translate-description, kept free of Deno APIs
// so the same code can be tried out from Node against real descriptions.
import { fixOutput, latinPhrases, prepare, preparePivot, type Lang } from "./glossary.ts";

export class TranslatorError extends Error {}

export function createTranslator(key: string, region: string) {
  async function call(text: string, from: Lang, to: Lang, attempt = 0): Promise<string> {
    const url = new URL("https://api.cognitive.microsofttranslator.com/translate");
    url.searchParams.set("api-version", "3.0");
    url.searchParams.set("from", from);
    url.searchParams.set("to", to);

    const response = await fetch(url, {
      method: "POST",
      headers: {
        "Ocp-Apim-Subscription-Key": key,
        "Ocp-Apim-Subscription-Region": region,
        "Content-Type": "application/json",
      },
      body: JSON.stringify([{ Text: text }]),
    });
    // The free tier also limits characters per minute; wait it out a few times.
    if (response.status === 429 && attempt < 3) {
      const seconds = Number(response.headers.get("Retry-After")) || 15;
      await new Promise((resolve) => setTimeout(resolve, seconds * 1000));
      return call(text, from, to, attempt + 1);
    }
    if (!response.ok) {
      // 401: wrong key or region. 403: the free tier's monthly allowance is used up.
      const body = await response.json().catch(() => null);
      throw new TranslatorError(`Translator error ${response.status}: ${body?.error?.message ?? response.statusText}`);
    }

    const [result] = (await response.json()) as { translations: { text: string }[] }[];
    return fixOutput(result.translations[0].text, to);
  }

  // Georgian goes to English directly (with the glossary), and to Russian by
  // way of that English: Microsoft's Georgian → Russian is much weaker than its
  // English → Russian, and the glossary fixes carry over. staffEnglish is used
  // for that instead when staff wrote the English themselves.
  return async function translate(
    from: Lang,
    text: string,
    targets: Lang[],
    staffEnglish?: string,
  ): Promise<Partial<Record<Lang, string>>> {
    const out: Partial<Record<Lang, string>> = {};
    if (targets.includes("en")) out.en = await call(prepare(text, from, "en"), from, "en");
    if (targets.includes("ru")) {
      if (from === "ka") {
        const english = out.en ?? staffEnglish ?? (await call(prepare(text, from, "en"), from, "en"));
        out.ru = await call(preparePivot(english, latinPhrases(text)), "en", "ru");
      } else {
        out.ru = await call(prepare(text, from, "ru"), from, "ru");
      }
    }
    if (targets.includes("ka")) out.ka = await call(prepare(text, from, "ka"), from, "ka");
    return out;
  };
}
