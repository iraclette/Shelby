import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  images: {
    // Photos are resized on upload (lib/resizeImage, shop-console) and served
    // as-is; on-the-fly sharp resizing was OOM-killing the 512 MB Render box.
    unoptimized: true,
    remotePatterns: [new URL("https://xlrvofdiqnuuafzwtefe.supabase.co/storage/v1/object/public/**")],
  },

  // No CSP yet; needs live testing against Turnstile/Supabase/Google Fonts.
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
