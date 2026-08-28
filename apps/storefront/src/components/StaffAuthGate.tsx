"use client";

import { useEffect, useState, type ReactNode } from "react";
import type { Session } from "@supabase/supabase-js";
import { getSupabaseClient } from "@/lib/supabase";
import Turnstile from "@/components/Turnstile";

// Mirrors the Shop Console's login convention: staff sign in with a short
// username, not a full email — anything without "@" gets this fixed fake
// domain appended.
function toEmail(identifier: string) {
  const trimmed = identifier.trim();
  return trimmed.includes("@") ? trimmed : `${trimmed}@shop.local`;
}

export function CenteredMessage({ children }: { children: ReactNode }) {
  return (
    <div className="flex min-h-dvh items-center justify-center px-4 text-center">
      <div>{children}</div>
    </div>
  );
}

// Shared by every staff-only storefront page (/staff/photos, /staff/inventory,
// ...): shows a login form until a session exists, then checks the signed-in
// profile's role and only renders `children` for owner/admin. Everything here
// is a UX convenience — the real enforcement is is_admin() RLS server-side.
type StaffContext = { session: Session; signOut: () => Promise<void> };

export default function StaffAuthGate({ children }: { children: (ctx: StaffContext) => ReactNode }) {
  const supabase = getSupabaseClient();

  const [checkingSession, setCheckingSession] = useState(true);
  const [session, setSession] = useState<Session | null>(null);
  const [isAdmin, setIsAdmin] = useState<boolean | null>(null);

  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [loginError, setLoginError] = useState("");
  const [signingIn, setSigningIn] = useState(false);

  const captchaEnabled = Boolean(process.env.NEXT_PUBLIC_TURNSTILE_SITE_KEY);
  const [captchaToken, setCaptchaToken] = useState<string | null>(null);
  const [captchaKey, setCaptchaKey] = useState(0);

  async function verifyCaptcha(): Promise<boolean> {
    if (!captchaEnabled) return true;
    if (!captchaToken) {
      setLoginError("Please complete the CAPTCHA challenge.");
      return false;
    }
    const res = await fetch("/api/verify-captcha", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ token: captchaToken }),
    });
    const data = await res.json();
    if (!data.ok) {
      setLoginError(data.error ?? "CAPTCHA verification failed.");
      setCaptchaToken(null);
      setCaptchaKey((k) => k + 1);
      return false;
    }
    return true;
  }

  useEffect(() => {
    supabase.auth.getSession().then(({ data }) => {
      setSession(data.session);
      setCheckingSession(false);
    });
    const { data: sub } = supabase.auth.onAuthStateChange((_event, newSession) => {
      setSession(newSession);
      if (!newSession) setIsAdmin(null);
    });
    return () => sub.subscription.unsubscribe();
  }, [supabase]);

  useEffect(() => {
    if (!session) return;
    supabase
      .from("profiles")
      .select("role")
      .eq("id", session.user.id)
      .maybeSingle()
      .then(({ data }) => {
        setIsAdmin(data?.role === "owner" || data?.role === "admin");
      });
  }, [session, supabase]);

  async function handleSignIn(event: React.FormEvent) {
    event.preventDefault();
    setLoginError("");
    setSigningIn(true);

    if (!(await verifyCaptcha())) {
      setSigningIn(false);
      return;
    }

    const { error } = await supabase.auth.signInWithPassword({
      email: toEmail(username),
      password,
    });
    setSigningIn(false);
    if (error) setLoginError(error.message);
  }

  async function handleSignOut() {
    await supabase.auth.signOut();
  }

  if (checkingSession) {
    return <CenteredMessage>Loading…</CenteredMessage>;
  }

  if (!session) {
    return (
      <CenteredMessage>
        <form onSubmit={handleSignIn} className="w-full max-w-sm space-y-4">
          <h1 className="font-display text-2xl text-paper">Shelby Staff</h1>
          {loginError && <p className="text-sm text-red-400">{loginError}</p>}
          <input
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            placeholder="Username"
            autoCapitalize="none"
            autoCorrect="off"
            className="w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
          />
          <input
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            type="password"
            placeholder="Password"
            className="w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
          />
          <Turnstile key={captchaKey} onToken={setCaptchaToken} />
          <button
            type="submit"
            disabled={signingIn || (captchaEnabled && !captchaToken)}
            className="w-full rounded-md bg-brass px-4 py-3 text-base font-medium text-ink disabled:opacity-60"
          >
            {signingIn ? "Signing in…" : "Sign in"}
          </button>
        </form>
      </CenteredMessage>
    );
  }

  if (isAdmin === null) {
    return <CenteredMessage>Loading…</CenteredMessage>;
  }

  if (!isAdmin) {
    return (
      <CenteredMessage>
        <p className="text-paper-dim">This account isn&apos;t an admin, so it can&apos;t access this page.</p>
        <button onClick={handleSignOut} className="mt-4 text-sm text-brass underline">
          Sign out
        </button>
      </CenteredMessage>
    );
  }

  return <>{children({ session, signOut: handleSignOut })}</>;
}
