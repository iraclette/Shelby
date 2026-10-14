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

// CAPTCHA for the storefront's own login forms. Renders nothing until a site key is set.
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
