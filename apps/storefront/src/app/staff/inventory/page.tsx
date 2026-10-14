import type { Metadata } from "next";
import InventoryClient from "./InventoryClient";

// Admin only; enforced by is_admin() RLS.
export const metadata: Metadata = {
  title: "Shelby — Inventory",
  robots: { index: false, follow: false },
};

export default function InventoryPage() {
  return <InventoryClient />;
}
