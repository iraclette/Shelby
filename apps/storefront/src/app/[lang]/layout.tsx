import type { Metadata } from "next";
import { Suspense } from "react";
import { Fraunces, Geist, Lora, Noto_Sans_Georgian, Noto_Serif_Georgian } from "next/font/google";
import Header from "@/components/Header";
import Footer, { FooterShops } from "@/components/Footer";
import Skeleton from "@/components/Skeleton";
import { CartProvider } from "@/lib/cart";
import { fetchCategories } from "@/lib/supabase";
import { LOCALES } from "@/i18n/config";
import { I18nProvider } from "@/i18n/client";
import { getDictionary, getLocale } from "@/i18n/server";
import "../globals.css";

const geistSans = Geist({
  variable: "--font-geist-sans",
  subsets: ["latin", "cyrillic"],
});

// Fraunces has no Cyrillic or Georgian, so those glyphs fall through to the fonts below
// (see --font-display in globals.css). Its metric-matched fallback would otherwise be
// Times New Roman, which has Cyrillic and would win over Lora, hence adjustFontFallback.
const fraunces = Fraunces({
  variable: "--font-fraunces",
  subsets: ["latin"],
  weight: ["400", "500", "600"],
  style: ["normal", "italic"],
  adjustFontFallback: false,
});

const lora = Lora({
  variable: "--font-lora",
  subsets: ["cyrillic"],
  weight: ["400", "500", "600"],
  style: ["normal", "italic"],
});

const notoSansGeorgian = Noto_Sans_Georgian({
  variable: "--font-noto-sans-georgian",
  subsets: ["georgian"],
});

const notoSerifGeorgian = Noto_Serif_Georgian({
  variable: "--font-noto-serif-georgian",
  subsets: ["georgian"],
  weight: ["400", "500", "600"],
});

const fontVariables = [geistSans, fraunces, lora, notoSansGeorgian, notoSerifGeorgian]
  .map((font) => font.variable)
  .join(" ");

export async function generateStaticParams() {
  return LOCALES.map((lang) => ({ lang }));
}

export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.title,
    description: dict.meta.description,
    icons: {
      icon: [
        { url: "/favicon.ico", sizes: "48x48" },
        { url: "/icon.png", type: "image/png", sizes: "512x512" },
      ],
      apple: "/apple-icon.png",
    },
  };
}

// Layout-level fetch runs on every request otherwise; revalidate on the same
// interval as /products so an admin's visibility toggle shows up promptly.
export const revalidate = 60;

export default async function RootLayout({ children }: LayoutProps<"/[lang]">) {
  const [lang, dict] = await Promise.all([getLocale(), getDictionary()]);
  // Not awaited: the header renders straight away and the link appears once Supabase answers.
  const showHandmadeLeather = fetchCategories().then((categories) =>
    categories.some((c) => c.name === "Handmade Leather"),
  );

  return (
    <html lang={lang} className={`${fontVariables} h-full antialiased`}>
      <body className="flex min-h-full flex-col bg-ink text-paper font-sans">
        <I18nProvider lang={lang} dict={dict}>
          <CartProvider>
            <Header showHandmadeLeather={showHandmadeLeather} />
            <main className="flex-1">{children}</main>
            <Footer>
              <Suspense
                fallback={
                  <div className="space-y-2 pt-1">
                    <Skeleton className="h-4 w-32" />
                    <Skeleton className="h-4 w-24" />
                    <Skeleton className="h-4 w-28" />
                  </div>
                }
              >
                <FooterShops />
              </Suspense>
            </Footer>
          </CartProvider>
        </I18nProvider>
      </body>
    </html>
  );
}
