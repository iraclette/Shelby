"use client";

import { useEffect, useState, type FormEvent } from "react";
import type { EmailOtpType, Session } from "@supabase/supabase-js";
import { getSupabaseClient } from "@/lib/supabase";
import Turnstile from "@/components/Turnstile";
import Skeleton, { FieldSkeleton } from "@/components/Skeleton";
import { useI18n } from "@/i18n/client";

// Same username login as the Shop Console, so admins can sign in here too.
function toSignInEmail(identifier: string) {
  const trimmed = identifier.trim();
  return trimmed.includes("@") ? trimmed : `${trimmed}@shop.local`;
}

type Customer = { full_name: string | null; email: string };

export default function AccountClient() {
  const supabase = getSupabaseClient();
  const { dict } = useI18n();

  const [checkingSession, setCheckingSession] = useState(true);
  const [session, setSession] = useState<Session | null>(null);
  const [customer, setCustomer] = useState<Customer | null>(null);

  const [mode, setMode] = useState<"sign-in" | "sign-up">("sign-in");
  const [fullName, setFullName] = useState("");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [notice, setNotice] = useState<string | null>(null);

  const captchaEnabled = Boolean(process.env.NEXT_PUBLIC_TURNSTILE_SITE_KEY);
  const [captchaToken, setCaptchaToken] = useState<string | null>(null);
  const [captchaKey, setCaptchaKey] = useState(0);

  async function verifyCaptcha(): Promise<boolean> {
    if (!captchaEnabled) return true;
    if (!captchaToken) {
      setError(dict.errors.captchaRequired);
      return false;
    }
    const res = await fetch("/api/verify-captcha", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ token: captchaToken }),
    });
    const data = await res.json();
    if (!data.ok) {
      setError(dict.errors.captchaFailed);
      setCaptchaToken(null);
      setCaptchaKey((k) => k + 1);
      return false;
    }
    return true;
  }

  useEffect(() => {
    // The confirmation email links here (not to supabase.co) with a one-time token_hash;
    // trading it in confirms the email and signs the customer in.
    const params = new URLSearchParams(window.location.search);
    const tokenHash = params.get("token_hash");
    const type = params.get("type") as EmailOtpType | null;
    const confirmed = tokenHash && type
      ? supabase.auth.verifyOtp({ token_hash: tokenHash, type }).then(({ error }) => {
          window.history.replaceState(null, "", window.location.pathname); // a reload must not reuse the spent token
          if (error) setError((error.code && dict.account.authErrors[error.code]) || error.message);
        })
      : Promise.resolve();

    confirmed.then(() => supabase.auth.getSession()).then(({ data }) => {
      setSession(data.session);
      setCheckingSession(false);
    });
    const { data: sub } = supabase.auth.onAuthStateChange((_event, newSession) => {
      setSession(newSession);
      if (!newSession) setCustomer(null);
    });
    return () => sub.subscription.unsubscribe();
  }, [supabase, dict]);

  useEffect(() => {
    if (!session) return;
    supabase
      .from("customers")
      .select("full_name, email")
      .eq("id", session.user.id)
      .maybeSingle()
      .then(({ data }) => setCustomer(data));
  }, [session, supabase]);

  function authErrorMessage(error: { code?: string; message: string }) {
    return (error.code && dict.account.authErrors[error.code]) || error.message;
  }

  async function handleSignIn(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);

    if (!(await verifyCaptcha())) {
      setSubmitting(false);
      return;
    }

    const { error } = await supabase.auth.signInWithPassword({ email: toSignInEmail(email), password });
    setSubmitting(false);
    if (error) setError(authErrorMessage(error));
  }

  async function handleSignUp(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setNotice(null);
    setSubmitting(true);

    if (!(await verifyCaptcha())) {
      setSubmitting(false);
      return;
    }

    const { data, error } = await supabase.auth.signUp({
      email,
      password,
      // Back to this page signed in. Must match Supabase's Redirect URLs allowlist,
      // otherwise Supabase falls back to the Site URL.
      options: {
        data: { full_name: fullName, account_type: "customer" },
        emailRedirectTo: `${window.location.origin}/account`,
      },
    });

    setSubmitting(false);
    if (error) {
      setError(authErrorMessage(error));
      return;
    }
    if (!data.session) {
      setNotice(dict.account.confirmEmail);
      setMode("sign-in");
    }
  }

  async function handleSignOut() {
    // Just this browser: the default (global) would sign them out on their other devices too.
    await supabase.auth.signOut({ scope: "local" });
  }

  if (checkingSession) {
    return (
      <div className="mx-auto max-w-xl px-6 py-16" aria-busy="true">
        <Skeleton className="h-9 w-40" />
        <Skeleton className="mt-4 h-5 w-56" />
        <div className="mt-8 space-y-5">
          <FieldSkeleton />
          <FieldSkeleton />
          <Skeleton className="h-11.5 w-full" />
        </div>
      </div>
    );
  }

  if (session) {
    return (
      <div className="mx-auto max-w-xl px-6 py-16">
        <h1 className="font-display text-3xl text-paper">{dict.account.yourAccount}</h1>
        <p className="mt-4 text-paper-dim">
          {dict.account.signedInAs} <span className="text-paper">{customer?.email ?? session.user.email}</span>
        </p>
        {customer?.full_name && <p className="text-paper-dim">{customer.full_name}</p>}
        <button
          onClick={handleSignOut}
          className="mt-8 border border-line px-6 py-2 text-sm tracking-[0.1em] text-paper-dim uppercase transition-colors hover:border-brass hover:text-brass"
        >
          {dict.common.signOut}
        </button>
      </div>
    );
  }

  return (
    <div className="mx-auto max-w-xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">
        {mode === "sign-in" ? dict.account.signIn : dict.account.createAccount}
      </h1>

      <div className="mt-4 flex gap-6 text-sm tracking-[0.1em] uppercase">
        <button
          onClick={() => { setMode("sign-in"); setError(null); }}
          className={mode === "sign-in" ? "text-brass" : "text-paper-dim"}
        >
          {dict.account.signIn}
        </button>
        <button
          onClick={() => { setMode("sign-up"); setError(null); }}
          className={mode === "sign-up" ? "text-brass" : "text-paper-dim"}
        >
          {dict.account.createAccount}
        </button>
      </div>

      {notice && <p className="mt-6 text-sm text-brass">{notice}</p>}

      <form onSubmit={mode === "sign-in" ? handleSignIn : handleSignUp} className="mt-8 space-y-5">
        {mode === "sign-up" && (
          <div>
            <label className="block text-sm text-paper-dim">{dict.common.fullName}</label>
            <input
              required
              value={fullName}
              onChange={(e) => setFullName(e.target.value)}
              className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
            />
          </div>
        )}
        <div>
          <label className="block text-sm text-paper-dim">
            {mode === "sign-in" ? dict.account.emailOrUsername : dict.common.email}
          </label>
          <input
            required
            type={mode === "sign-up" ? "email" : "text"}
            autoCapitalize="none"
            autoCorrect="off"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">{dict.account.password}</label>
          <input
            required
            type="password"
            minLength={6}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>

        <Turnstile key={captchaKey} onToken={setCaptchaToken} />

        {error && <p className="text-sm text-red-400">{error}</p>}

        <button
          type="submit"
          disabled={submitting || (captchaEnabled && !captchaToken)}
          className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
        >
          {submitting ? dict.account.pleaseWait : mode === "sign-in" ? dict.account.signIn : dict.account.createAccount}
        </button>
      </form>
    </div>
  );
}
