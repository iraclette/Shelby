// Translates a product's description from the language staff last wrote in
// (descriptions.source) into every storefront language that is missing or was
// machine-translated before. Called by the storefront staff pages and Shop
// Console (2.3.3+) right after they save a description; see
// 0038_description_translations.sql for the stored shape.
//
// Runs with the caller's own token, so only owners/admins can trigger it and
// the products write policy still applies. Translation is Microsoft Translator's
// free tier (F0: 2M characters a month, never billed); needs the
// AZURE_TRANSLATOR_KEY and AZURE_TRANSLATOR_REGION secrets.

import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const ANON_KEY = Deno.env.get("SUPABASE_ANON_KEY")!;

const TRANSLATOR_KEY = Deno.env.get("AZURE_TRANSLATOR_KEY")!;
const TRANSLATOR_REGION = Deno.env.get("AZURE_TRANSLATOR_REGION")!;

const LANGUAGES = ["ka", "en", "ru"]; // Translator uses the same codes

type Entry = { text?: string; auto?: boolean };
type Descriptions = { source?: string } & Record<string, Entry | string | undefined>;

function json(body: unknown, status: number) {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

class TranslatorError extends Error {}

async function translate(sourceLang: string, text: string, targets: string[]) {
  const url = new URL("https://api.cognitive.microsofttranslator.com/translate");
  url.searchParams.set("api-version", "3.0");
  url.searchParams.set("from", sourceLang);
  for (const lang of targets) url.searchParams.append("to", lang);

  const response = await fetch(url, {
    method: "POST",
    headers: {
      "Ocp-Apim-Subscription-Key": TRANSLATOR_KEY,
      "Ocp-Apim-Subscription-Region": TRANSLATOR_REGION,
      "Content-Type": "application/json",
    },
    body: JSON.stringify([{ Text: text }]),
  });
  if (!response.ok) {
    // 401: wrong key or region. 429/403: the free tier's monthly allowance is used up.
    const body = await response.json().catch(() => null);
    throw new TranslatorError(`Translator error ${response.status}: ${body?.error?.message ?? response.statusText}`);
  }

  const [result] = (await response.json()) as { translations: { text: string; to: string }[] }[];
  return Object.fromEntries(result.translations.map((t) => [t.to, t.text])) as Record<string, string>;
}

Deno.serve(async (req) => {
  try {
    const callerClient = createClient(SUPABASE_URL, ANON_KEY, {
      global: { headers: { Authorization: req.headers.get("Authorization") ?? "" } },
    });

    const { data: userData, error: userError } = await callerClient.auth.getUser();
    if (userError || !userData.user) return json({ error: "Not signed in." }, 401);

    const { data: callerProfile } = await callerClient
      .from("profiles")
      .select("role")
      .eq("id", userData.user.id)
      .maybeSingle();
    if (!callerProfile || !["owner", "admin"].includes(callerProfile.role)) {
      return json({ error: "Not authorized." }, 403);
    }

    const { product_id: productId } = await req.json();
    if (typeof productId !== "string") return json({ error: "product_id is required." }, 400);

    const { data: product, error: productError } = await callerClient
      .from("products")
      .select("descriptions")
      .eq("id", productId)
      .maybeSingle();
    if (productError || !product) return json({ error: "Product not found." }, 404);

    const descriptions = (product.descriptions ?? {}) as Descriptions;
    const source = descriptions.source;
    const sourceEntry = source ? (descriptions[source] as Entry | undefined) : undefined;
    const text = sourceEntry?.text?.trim();
    if (!source || !LANGUAGES.includes(source) || !text) return json({ translated: [] }, 200);

    const targets = LANGUAGES.filter((lang) => {
      if (lang === source) return false;
      const entry = descriptions[lang] as Entry | undefined;
      return !entry?.text || entry.auto !== false;
    });
    if (targets.length === 0) return json({ translated: [] }, 200);

    const translations = await translate(source, text, targets);

    const { data: applied, error: applyError } = await callerClient.rpc("apply_description_translations", {
      target_product: productId,
      source_lang: source,
      source_text: sourceEntry!.text,
      translations,
    });
    if (applyError) return json({ error: applyError.message }, 500);
    // false: the description was edited again meanwhile; that save's own call translates it.
    return json({ translated: applied ? targets : [] }, 200);
  } catch (error) {
    if (error instanceof TranslatorError) return json({ error: error.message }, 502);
    return json({ error: error instanceof Error ? error.message : String(error) }, 500);
  }
});
