import type { Metadata } from "next";
import StaffPhotosClient from "./StaffPhotosClient";

// Admin only; enforced by is_admin() RLS.
export const metadata: Metadata = {
  title: "Shelby — Product Photos",
  robots: { index: false, follow: false },
};

export default function StaffPhotosPage() {
  return <StaffPhotosClient />;
}
