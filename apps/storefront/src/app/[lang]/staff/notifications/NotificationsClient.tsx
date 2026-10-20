"use client";

import { useCallback, useEffect, useState } from "react";
import type { Session } from "@supabase/supabase-js";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";
import Skeleton from "@/components/Skeleton";
import { useI18n } from "@/i18n/client";
import { TELEGRAM_EVENTS, type TelegramEvent } from "@/lib/telegramText";

type Link = { id: string; chat_name: string | null; events: TelegramEvent[] };

const BOT_USERNAME = process.env.NEXT_PUBLIC_TELEGRAM_BOT_USERNAME;

// 24 URL-safe characters: Telegram's /start parameter allows A-Z, a-z, 0-9, _ and -.
function randomCode() {
  const bytes = crypto.getRandomValues(new Uint8Array(18));
  return btoa(String.fromCharCode(...bytes)).replace(/\+/g, "-").replace(/\//g, "_");
}

export default function NotificationsClient() {
  return (
    <StaffAuthGate>{({ session, signOut }) => <Notifications session={session} onSignOut={signOut} />}</StaffAuthGate>
  );
}

function Notifications({ session, onSignOut }: { session: Session; onSignOut: () => void }) {
  const supabase = getSupabaseClient();
  const { dict, lang } = useI18n();
  const t = dict.notifications;

  const [links, setLinks] = useState<Link[]>([]);
  const [loading, setLoading] = useState(true);
  const [connecting, setConnecting] = useState(false);
  const [error, setError] = useState("");

  const load = useCallback(async () => {
    const { data } = await supabase.from("telegram_links").select("id, chat_name, events").order("created_at");
    setLinks((data as Link[]) ?? []);
    setLoading(false);
  }, [supabase]);

  // Linking finishes in the Telegram app; pick the new chat up when they come back here.
  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect -- loads from Supabase on mount
    load();
    const onVisible = () => document.visibilityState === "visible" && load();
    document.addEventListener("visibilitychange", onVisible);
    return () => document.removeEventListener("visibilitychange", onVisible);
  }, [load]);

  async function connect() {
    setError("");
    setConnecting(true);
    const code = randomCode();
    const { error: insertError } = await supabase
      .from("telegram_link_codes")
      .insert({ code, profile_id: session.user.id, lang });
    if (insertError) {
      setConnecting(false);
      setError(t.failed);
      return;
    }
    // On a phone this opens the Telegram app on the bot, with the code ready to send.
    window.location.href = `https://t.me/${BOT_USERNAME}?start=${code}`;
    setTimeout(() => setConnecting(false), 3000);
  }

  async function toggleEvent(link: Link, event: TelegramEvent) {
    const events = link.events.includes(event) ? link.events.filter((e) => e !== event) : [...link.events, event];
    setLinks((current) => current.map((l) => (l.id === link.id ? { ...l, events } : l)));
    const { error: updateError } = await supabase.from("telegram_links").update({ events }).eq("id", link.id);
    if (updateError) {
      setError(t.failed);
      load();
    }
  }

  async function disconnect(link: Link) {
    if (!confirm(t.confirmDisconnect)) return;
    const { error: deleteError } = await supabase.from("telegram_links").delete().eq("id", link.id);
    if (deleteError) setError(t.failed);
    load();
  }

  return (
    <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper">{t.title}</h1>
        <button onClick={onSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          {dict.common.signOut}
        </button>
      </div>
      <p className="mt-2 text-sm text-paper-dim">{t.intro}</p>

      {!BOT_USERNAME ? (
        <p className="mt-6 text-sm text-paper-dim">{t.notSetUp}</p>
      ) : (
        <>
          <button
            onClick={connect}
            disabled={connecting}
            className="mt-6 flex min-h-14 w-full items-center justify-center rounded-md bg-brass text-base font-medium text-ink disabled:opacity-60"
          >
            {connecting ? t.connecting : t.connect}
          </button>
          <p className="mt-2 text-center text-xs text-paper-dim">{t.connectHint}</p>
        </>
      )}

      {error && <p className="mt-4 text-sm text-red-400">{error}</p>}

      <div className="mt-8 space-y-4">
        {loading && <Skeleton className="h-40 w-full rounded-md" />}
        {!loading && links.length === 0 && <p className="text-sm text-paper-dim">{t.none}</p>}
        {links.map((link) => (
          <div key={link.id} className="rounded-md border border-line p-4">
            <div className="flex items-center justify-between gap-3">
              <p className="font-medium text-paper">{link.chat_name || "Telegram"}</p>
              <button onClick={() => disconnect(link)} className="min-h-11 text-sm text-paper-dim active:text-red-400">
                {t.disconnect}
              </button>
            </div>
            <p className="mt-2 text-xs text-paper-dim">{t.sends}</p>
            <div className="mt-1">
              {TELEGRAM_EVENTS.map((event) => (
                <label key={event} className="flex min-h-11 items-center gap-3 text-sm text-paper">
                  <input
                    type="checkbox"
                    checked={link.events.includes(event)}
                    onChange={() => toggleEvent(link, event)}
                    className="h-5 w-5 accent-brass"
                  />
                  {t.events[event]}
                </label>
              ))}
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
