import type { Metadata } from "next";
import { getDictionary } from "@/i18n/server";
import StaffPhotosClient from "./StaffPhotosClient";

// Admin only; enforced by is_admin() RLS.
export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.photos,
    robots: { index: false, follow: false },
  };
}

export default function StaffPhotosPage() {
  return <StaffPhotosClient />;
}
