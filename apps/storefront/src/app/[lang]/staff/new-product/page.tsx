import type { Metadata } from "next";
import { getDictionary } from "@/i18n/server";
import NewProductClient from "./NewProductClient";

// Admin only; enforced by is_admin() RLS.
export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.addProduct,
    robots: { index: false, follow: false },
  };
}

export default function NewProductPage() {
  return <NewProductClient />;
}
