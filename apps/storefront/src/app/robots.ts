import type { MetadataRoute } from "next";
import { LOCALES } from "@/i18n/config";
import { SITE_URL } from "@/lib/seo";

// Staff tools, accounts and checkout have nothing for search engines (staff pages
// also say noindex themselves); everything else is open.
const PRIVATE = ["/staff", "/account", "/cart", "/checkout", "/api"];

export default function robots(): MetadataRoute.Robots {
  return {
    rules: {
      userAgent: "*",
      allow: "/",
      disallow: [...PRIVATE, ...LOCALES.flatMap((locale) => PRIVATE.map((path) => `/${locale}${path}`))],
    },
    sitemap: `${SITE_URL}/sitemap.xml`,
  };
}
