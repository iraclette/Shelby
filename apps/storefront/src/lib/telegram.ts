import "server-only";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { fmt } from "@/i18n/format";
import { TELEGRAM_TEXT, telegramLocale, type TelegramEvent } from "@/lib/telegramText";
import type { Locale } from "@/i18n/config";

// Sends through the Shelby bot (TELEGRAM_BOT_TOKEN). Returns false when Telegram says the
// chat is gone or blocked the bot, so the caller can forget it.
export async function sendTelegramMessage(chatId: number, text: string): Promise<boolean> {
  const token = process.env.TELEGRAM_BOT_TOKEN;
  if (!token) return true;

  const res = await fetch(`https://api.telegram.org/bot${token}/sendMessage`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    // Telegram's limit is 4096 characters; a long customer message gets cut, not dropped.
    body: JSON.stringify({ chat_id: chatId, text: text.slice(0, 4000), link_preview_options: { is_disabled: true } }),
    cache: "no-store",
  });
  if (res.ok) return true;
  const body = await res.text();
  if (res.status === 403 || (res.status === 400 && body.includes("chat not found"))) return false;
  throw new Error(`Telegram sendMessage failed: ${res.status} ${body}`);
}

// Messages every linked chat that wants this event, in that chat's language. Best-effort:
// meant to run inside after(), so a Telegram hiccup never fails the visitor's request.
export async function notifyStaff(event: TelegramEvent, vars: (lang: Locale) => Record<string, string | number>) {
  if (!process.env.TELEGRAM_BOT_TOKEN) return;

  const supabaseAdmin = getSupabaseAdmin();
  const { data: links, error } = await supabaseAdmin.from("telegram_links").select("chat_id, lang").contains("events", [event]);
  if (error) {
    console.error("Telegram links lookup failed:", error);
    return;
  }

  await Promise.all(
    (links ?? []).map(async (link) => {
      const lang = telegramLocale(link.lang);
      try {
        const delivered = await sendTelegramMessage(link.chat_id, fmt(TELEGRAM_TEXT[lang].events[event], vars(lang)));
        if (!delivered) await supabaseAdmin.from("telegram_links").delete().eq("chat_id", link.chat_id);
      } catch (err) {
        console.error("Telegram notification failed:", err);
      }
    }),
  );
}

// ", preferred shop: Shelby" in the chat's language, or "" when no shop was picked.
export function preferredShopText(lang: Locale, shopName: string | null | undefined) {
  return shopName ? fmt(TELEGRAM_TEXT[lang].preferredShop, { shop: shopName }) : "";
}

export async function shopName(shopId: string | null | undefined) {
  if (!shopId) return null;
  const { data } = await getSupabaseAdmin().from("shops").select("name").eq("id", shopId).maybeSingle();
  return (data?.name as string | undefined) ?? null;
}
