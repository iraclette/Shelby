import "server-only";
import { getLocale } from "../server";
import type { Locale } from "../config";
import en, { type LegalTexts } from "./en";
import ka from "./ka";
import ru from "./ru";

const legal: Record<Locale, LegalTexts> = { en, ka, ru };

export async function getLegalTexts(): Promise<LegalTexts> {
  return legal[await getLocale()];
}
