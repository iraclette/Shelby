import { NextRequest, NextResponse } from "next/server";
import { getSupabaseAdmin } from "@/lib/supabaseAdmin";
import { sendTelegramMessage } from "@/lib/telegram";
import { TELEGRAM_TEXT, telegramLocale } from "@/lib/telegramText";
import { fmt } from "@/i18n/format";

// Telegram calls this for every message sent to the Shelby bot (registered with setWebhook,
// secret_token = TELEGRAM_WEBHOOK_SECRET). The bot only understands:
//   /start <code>  link this chat to the account that made the code (Notifications page)
//   /stop          unlink this chat
const CODE_LIFETIME_MS = 30 * 60 * 1000;

type TelegramUpdate = {
  message?: {
    text?: string;
    chat: { id: number; type: string; first_name?: string; last_name?: string; username?: string; title?: string };
    from?: { language_code?: string };
  };
};

export async function POST(request: NextRequest) {
  const secret = process.env.TELEGRAM_WEBHOOK_SECRET;
  if (!secret || request.headers.get("x-telegram-bot-api-secret-token") !== secret) {
    return NextResponse.json({ error: "Unauthorized." }, { status: 401 });
  }

  const message = ((await request.json()) as TelegramUpdate).message;
  // Private chats only; anything else (groups, edits, stickers) is ignored. Telegram needs a 200 either way.
  if (!message?.text || message.chat.type !== "private") return NextResponse.json({ ok: true });

  const chatId = message.chat.id;
  const fallbackLang = telegramLocale(message.from?.language_code);
  const [command, argument] = message.text.trim().split(/\s+/, 2);
  const supabaseAdmin = getSupabaseAdmin();

  if (command === "/start" && argument) {
    const { data: code } = await supabaseAdmin
      .from("telegram_link_codes")
      .select("code, profile_id, lang, created_at, profiles(full_name)")
      .eq("code", argument)
      .maybeSingle();

    if (!code || Date.now() - new Date(code.created_at as string).getTime() > CODE_LIFETIME_MS) {
      await sendTelegramMessage(chatId, TELEGRAM_TEXT[fallbackLang].linkExpired);
      return NextResponse.json({ ok: true });
    }

    const lang = telegramLocale(code.lang as string);
    const chatName =
      [message.chat.first_name, message.chat.last_name].filter(Boolean).join(" ") || message.chat.username || null;
    // A chat belongs to one account: relinking moves it (keeping its chosen events).
    const { error } = await supabaseAdmin
      .from("telegram_links")
      .upsert({ chat_id: chatId, profile_id: code.profile_id, chat_name: chatName, lang }, { onConflict: "chat_id" });
    await supabaseAdmin.from("telegram_link_codes").delete().eq("code", code.code);
    // Codes nobody used: no reason to keep them around.
    await supabaseAdmin
      .from("telegram_link_codes")
      .delete()
      .lt("created_at", new Date(Date.now() - CODE_LIFETIME_MS).toISOString());

    if (error) {
      console.error("Telegram link failed:", error);
      return NextResponse.json({ ok: true });
    }
    const profile = code.profiles as unknown as { full_name: string | null } | null;
    await sendTelegramMessage(chatId, fmt(TELEGRAM_TEXT[lang].linked, { name: profile?.full_name || "Shelby" }));
    return NextResponse.json({ ok: true });
  }

  if (command === "/stop") {
    const { data: link } = await supabaseAdmin.from("telegram_links").select("lang").eq("chat_id", chatId).maybeSingle();
    await supabaseAdmin.from("telegram_links").delete().eq("chat_id", chatId);
    await sendTelegramMessage(chatId, TELEGRAM_TEXT[link ? telegramLocale(link.lang as string) : fallbackLang].stopped);
    return NextResponse.json({ ok: true });
  }

  await sendTelegramMessage(chatId, TELEGRAM_TEXT[fallbackLang].help);
  return NextResponse.json({ ok: true });
}
