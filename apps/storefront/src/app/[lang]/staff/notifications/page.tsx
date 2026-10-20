import type { Metadata } from "next";
import { getDictionary } from "@/i18n/server";
import NotificationsClient from "./NotificationsClient";

// Admin only; enforced by RLS on telegram_links / telegram_link_codes (0031).
export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.notifications,
    robots: { index: false, follow: false },
  };
}

export default function StaffNotificationsPage() {
  return <NotificationsClient />;
}
