import type { Metadata } from "next";
import InventoryClient from "./InventoryClient";

// Admin-only, reachable by direct link (or the header's admin-only link) —
// not part of the public nav for a signed-out visitor. Reads are gated by
// the same is_admin() RLS the desktop Shop Console relies on.
export const metadata: Metadata = {
  title: "Shelby — Inventory",
  robots: { index: false, follow: false },
};

export default function InventoryPage() {
  return <InventoryClient />;
}
