"use client";

import Script from "next/script";
import { useRef } from "react";

declare global {
  interface Window {
    turnstile?: {
      render: (
        container: HTMLElement,
        options: { sitekey: string; callback: (token: string) => void },
      ) => string;
    };
  }
}

// App-level CAPTCHA gate for the storefront's own login/signup forms (not
// Supabase's project-wide CAPTCHA-required setting — see the security plan
// notes: that setting is all-or-nothing and would also require the Shop
// Console's native login to solve a challenge, which needs Qt WebEngine
// (MSVC-only, this project builds with MinGW)). Renders nothing if no site
// key is configured yet, so login keeps working before Cloudflare is set up
// — see api/verify-captcha's matching fail-open behavior.
export default function Turnstile({ onToken }: { onToken: (token: string) => void }) {
  const containerRef = useRef<HTMLDivElement>(null);
  const siteKey = process.env.NEXT_PUBLIC_TURNSTILE_SITE_KEY;

  if (!siteKey) return null;

  return (
    <>
      {/* onReady (unlike onLoad) fires both on first load AND on every
          remount — needed since the parent remounts this component via a
          `key` change to get a fresh challenge after a failed attempt. */}
      <Script
        src="https://challenges.cloudflare.com/turnstile/v0/api.js"
        strategy="afterInteractive"
        onReady={() => {
          if (containerRef.current && window.turnstile) {
            window.turnstile.render(containerRef.current, { sitekey: siteKey, callback: onToken });
          }
        }}
      />
      <div ref={containerRef} />
    </>
  );
}
