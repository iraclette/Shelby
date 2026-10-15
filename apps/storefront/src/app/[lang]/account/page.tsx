import type { Metadata } from "next";
import { getDictionary } from "@/i18n/server";
import AccountClient from "./AccountClient";

export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return {
    title: dict.meta.account,
  };
}

export default function AccountPage() {
  return <AccountClient />;
}
