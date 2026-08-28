import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  images: {
    remotePatterns: [new URL("https://xlrvofdiqnuuafzwtefe.supabase.co/storage/v1/object/public/**")],
  },

  // Baseline hardening only — no CSP here yet. A strict CSP risks breaking
  // the Turnstile widget/Supabase/Google Fonts and can't be verified without
  // live browser testing, so it's deliberately deferred rather than shipped
  // half-tested.
  async headers() {
    return [
      {
        source: "/:path*",
        headers: [
          { key: "X-Frame-Options", value: "DENY" },
          { key: "X-Content-Type-Options", value: "nosniff" },
          { key: "Referrer-Policy", value: "strict-origin-when-cross-origin" },
          { key: "Permissions-Policy", value: "camera=(), microphone=(), geolocation=()" },
        ],
      },
    ];
  },
};

export default nextConfig;
