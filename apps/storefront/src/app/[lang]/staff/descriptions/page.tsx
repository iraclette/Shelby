import type { Metadata } from "next";
import { getDictionary } from "@/i18n/server";
import StaffDescriptionsClient from "./StaffDescriptionsClient";

// Admin only; enforced by is_admin() RLS and the translate-description function.
export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.descriptions,
    robots: { index: false, follow: false },
  };
}

export default function StaffDescriptionsPage() {
  return <StaffDescriptionsClient />;
}
