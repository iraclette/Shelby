// Translates a product's description from the language staff last wrote in
// (descriptions.source) into every storefront language that is missing or was
// machine-translated before. Called by the storefront staff pages and Shop
// Console (2.3.3+) right after they save a description; see
// 0038_description_translations.sql for the stored shape.
//
// Runs with the caller's own token, so only owners/admins can trigger it and
// the products write policy still applies. Needs the ANTHROPIC_API_KEY secret.

import Anthropic from "npm:@anthropic-ai/sdk@0.137.0";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const ANON_KEY = Deno.env.get("SUPABASE_ANON_KEY")!;

const LANGUAGES: Record<string, string> = { ka: "Georgian", en: "English", ru: "Russian" };

type Entry = { text?: string; auto?: boolean };
type Descriptions = { source?: string } & Record<string, Entry | string | undefined>;

const anthropic = new Anthropic(); // ANTHROPIC_API_KEY

function json(body: unknown, status: number) {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "Content-Type": "application/json" },
  });
}

async function translate(productName: string, sourceLang: string, text: string, targets: string[]) {
  const response = await anthropic.beta.messages.create({
    model: "claude-opus-5-5",
    max_tokens: 16000,
    // Re-run on Anthropic's recommended fallback model if the request is declined.
    betas: ["server-side-fallback-2026-07-01"],
    fallbacks: "default",
    output_config: {
      effort: "medium",
      format: {
        type: "json_schema",
        schema: {
          type: "object",
          properties: Object.fromEntries(targets.map((lang) => [lang, { type: "string" }])),
          required: targets,
          additionalProperties: false,
        },
      },
    },
    system:
      "You translate product descriptions for Shelby, a perfume and cosmetics shop in Gori, Georgia. " +
      "Translate faithfully and naturally, the way a good local shop in that language would write it: " +
      "keep the meaning, tone and line breaks, and don't add or drop claims. " +
      "Keep brand names, product names, shade numbers and volumes exactly as written. " +
      "Georgian uses the formal register (თქვენ) and the shop's own words: " +
      "სუნამო / არომატი / სურნელი for perfume, fragrance and scent, მდგრადი for long-lasting, ფლაკონი for the bottle.",
    messages: [
      {
        role: "user",
        content:
          `Product: ${productName}\n\n` +
          `Translate this ${LANGUAGES[sourceLang]} description into ${targets.map((l) => LANGUAGES[l]).join(", ")}. ` +
          `Return one field per language code (${targets.join(", ")}).\n\n` +
          `<description>\n${text}\n</description>`,
      },
    ],
  });

  if (response.stop_reason === "refusal") throw new Error("The translation request was declined.");
  if (response.stop_reason === "max_tokens") throw new Error("The translation was cut off.");
  const block = response.content.find((b) => b.type === "text");
  if (!block || block.type !== "text") throw new Error("No translation in the response.");
  return JSON.parse(block.text) as Record<string, string>;
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
      .select("name, descriptions")
      .eq("id", productId)
      .maybeSingle();
    if (productError || !product) return json({ error: "Product not found." }, 404);

    const descriptions = (product.descriptions ?? {}) as Descriptions;
    const source = descriptions.source;
    const sourceEntry = source ? (descriptions[source] as Entry | undefined) : undefined;
    const text = sourceEntry?.text?.trim();
    if (!source || !(source in LANGUAGES) || !text) return json({ translated: [] }, 200);

    const targets = Object.keys(LANGUAGES).filter((lang) => {
      if (lang === source) return false;
      const entry = descriptions[lang] as Entry | undefined;
      return !entry?.text || entry.auto !== false;
    });
    if (targets.length === 0) return json({ translated: [] }, 200);

    const translations = await translate(product.name, source, text, targets);

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
    if (error instanceof Anthropic.APIError) {
      return json({ error: `Translation service error (${error.status}): ${error.message}` }, 502);
    }
    return json({ error: error instanceof Error ? error.message : String(error) }, 500);
  }
});
