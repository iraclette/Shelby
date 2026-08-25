import type { Metadata } from "next";
import { Fraunces, Geist } from "next/font/google";
import Header from "@/components/Header";
import Footer from "@/components/Footer";
import { CartProvider } from "@/lib/cart";
import { fetchCategories } from "@/lib/supabase";
import "./globals.css";

const geistSans = Geist({
  variable: "--font-geist-sans",
  subsets: ["latin"],
});

const fraunces = Fraunces({
  variable: "--font-fraunces",
  subsets: ["latin"],
  weight: ["400", "500", "600"],
  style: ["normal", "italic"],
});

export const metadata: Metadata = {
  title: "Shelby",
  description: "Knives, leather, cosmetics, and souvenirs.",
};

// Layout-level fetch runs on every request otherwise; revalidate on the same
// interval as /products so an admin's visibility toggle shows up promptly.
export const revalidate = 60;

export default async function RootLayout({ children }: LayoutProps<"/">) {
  const categories = await fetchCategories();
  const showHandmadeLeather = categories.some((c) => c.name === "Handmade Leather");

  return (
    <html lang="en" className={`${geistSans.variable} ${fraunces.variable} h-full antialiased`}>
      <body className="flex min-h-full flex-col bg-ink text-paper font-sans">
        <CartProvider>
          <Header showHandmadeLeather={showHandmadeLeather} />
          <main className="flex-1">{children}</main>
          <Footer />
        </CartProvider>
      </body>
    </html>
  );
}
