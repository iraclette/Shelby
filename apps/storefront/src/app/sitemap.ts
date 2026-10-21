import type { MetadataRoute } from "next";
import { LOCALES } from "@/i18n/config";
import { fetchCategories, getSupabaseClient } from "@/lib/supabase";
import { SITE_URL } from "@/lib/seo";

// Rebuilt at most hourly; new products show up in search engines within a day or so anyway.
export const revalidate = 3600;

const localized = (locale: string, path: string) => `${SITE_URL}/${locale}${path === "/" ? "" : path}`;

// Every language version of a page, each listing the others (hreflang).
function entries(path: string, lastModified?: string): MetadataRoute.Sitemap {
  const languages = Object.fromEntries(LOCALES.map((locale) => [locale, localized(locale, path)]));
  return LOCALES.map((locale) => ({ url: localized(locale, path), lastModified, alternates: { languages } }));
}

// Products the storefront shows: hidden categories are left out (same rule as onStorefront).
async function storefrontProducts() {
  const products: { id: string; updated_at: string }[] = [];
  for (let from = 0; ; from += 1000) {
    const { data } = await getSupabaseClient()
      .from("products")
      .select("id, updated_at, categories(visible_on_storefront)")
      .order("id")
      .range(from, from + 999);
    const rows = (data ?? []) as unknown as {
      id: string;
      updated_at: string;
      categories: { visible_on_storefront: boolean } | null;
    }[];
    products.push(...rows.filter((row) => row.categories?.visible_on_storefront !== false));
    if (rows.length < 1000) return products;
  }
}

export default async function sitemap(): Promise<MetadataRoute.Sitemap> {
  const [products, categories] = await Promise.all([storefrontProducts(), fetchCategories()]);
  const pages = ["/", "/products", "/contact", "/delivery", "/jobs", "/terms", "/privacy"];
  if (categories.some((c) => c.name === "Handmade Leather")) pages.push("/handmade-leather");

  return [
    ...pages.flatMap((path) => entries(path)),
    ...products.flatMap((product) => entries(`/products/${product.id}`, product.updated_at)),
  ];
}
