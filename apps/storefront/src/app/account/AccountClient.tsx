"use client";

import { useEffect, useState, type FormEvent } from "react";
import type { Session } from "@supabase/supabase-js";
import { getSupabaseClient } from "@/lib/supabase";

type Customer = { full_name: string | null; email: string };

export default function AccountClient() {
  const supabase = getSupabaseClient();

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

  useEffect(() => {
    supabase.auth.getSession().then(({ data }) => {
      setSession(data.session);
      setCheckingSession(false);
    });
    const { data: sub } = supabase.auth.onAuthStateChange((_event, newSession) => {
      setSession(newSession);
      if (!newSession) setCustomer(null);
    });
    return () => sub.subscription.unsubscribe();
  }, [supabase]);

  useEffect(() => {
    if (!session) return;
    supabase
      .from("customers")
      .select("full_name, email")
      .eq("id", session.user.id)
      .maybeSingle()
      .then(({ data }) => setCustomer(data));
  }, [session, supabase]);

  async function handleSignIn(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setSubmitting(true);
    const { error } = await supabase.auth.signInWithPassword({ email, password });
    setSubmitting(false);
    if (error) setError(error.message);
  }

  async function handleSignUp(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setNotice(null);
    setSubmitting(true);

    const { data, error } = await supabase.auth.signUp({
      email,
      password,
      options: { data: { full_name: fullName, account_type: "customer" } },
    });

    setSubmitting(false);
    if (error) {
      setError(error.message);
      return;
    }
    if (!data.session) {
      setNotice("Check your email to confirm your account, then sign in.");
      setMode("sign-in");
    }
  }

  async function handleSignOut() {
    await supabase.auth.signOut();
  }

  if (checkingSession) {
    return null;
  }

  if (session) {
    return (
      <div className="mx-auto max-w-xl px-6 py-16">
        <h1 className="font-display text-3xl text-paper">Your Account</h1>
        <p className="mt-4 text-paper-dim">
          Signed in as <span className="text-paper">{customer?.email ?? session.user.email}</span>
        </p>
        {customer?.full_name && <p className="text-paper-dim">{customer.full_name}</p>}
        <button
          onClick={handleSignOut}
          className="mt-8 border border-line px-6 py-2 text-sm tracking-[0.1em] text-paper-dim uppercase transition-colors hover:border-brass hover:text-brass"
        >
          Sign out
        </button>
      </div>
    );
  }

  return (
    <div className="mx-auto max-w-xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">
        {mode === "sign-in" ? "Sign In" : "Create Account"}
      </h1>

      <div className="mt-4 flex gap-6 text-sm tracking-[0.1em] uppercase">
        <button
          onClick={() => { setMode("sign-in"); setError(null); }}
          className={mode === "sign-in" ? "text-brass" : "text-paper-dim"}
        >
          Sign In
        </button>
        <button
          onClick={() => { setMode("sign-up"); setError(null); }}
          className={mode === "sign-up" ? "text-brass" : "text-paper-dim"}
        >
          Create Account
        </button>
      </div>

      {notice && <p className="mt-6 text-sm text-brass">{notice}</p>}

      <form onSubmit={mode === "sign-in" ? handleSignIn : handleSignUp} className="mt-8 space-y-5">
        {mode === "sign-up" && (
          <div>
            <label className="block text-sm text-paper-dim">Full name</label>
            <input
              required
              value={fullName}
              onChange={(e) => setFullName(e.target.value)}
              className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
            />
          </div>
        )}
        <div>
          <label className="block text-sm text-paper-dim">Email</label>
          <input
            required
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>
        <div>
          <label className="block text-sm text-paper-dim">Password</label>
          <input
            required
            type="password"
            minLength={6}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            className="mt-1 w-full border border-line bg-ink-soft px-3 py-2 text-paper"
          />
        </div>

        {error && <p className="text-sm text-red-400">{error}</p>}

        <button
          type="submit"
          disabled={submitting}
          className="w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
        >
          {submitting ? "Please wait…" : mode === "sign-in" ? "Sign In" : "Create Account"}
        </button>
      </form>
    </div>
  );
}
