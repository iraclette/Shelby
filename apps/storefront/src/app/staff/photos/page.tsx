import type { Metadata } from "next";
import StaffPhotosClient from "./StaffPhotosClient";

// Staff-only tool — only surfaced in the nav for signed-in admins
// (AdminLink), and excluded from search indexing regardless. Writes are
// still gated by the same is_admin() RLS the desktop Shop Console relies
// on, so a stray visitor with no admin account can look but can't touch
// anything.
export const metadata: Metadata = {
  title: "Shelby — Product Photos",
  robots: { index: false, follow: false },
};

export default function StaffPhotosPage() {
  return <StaffPhotosClient />;
}
