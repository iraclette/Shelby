import type { Metadata } from "next";
import { getDictionary } from "@/i18n/server";
import InventoryClient from "./InventoryClient";

// Admin only; enforced by is_admin() RLS.
export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.inventory,
    robots: { index: false, follow: false },
  };
}

export default function InventoryPage() {
  return <InventoryClient />;
}
