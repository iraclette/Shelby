import LegalPage, { legalMetadata } from "@/components/LegalPage";

export async function generateMetadata() {
  return legalMetadata("privacy");
}

export default function PrivacyPage() {
  return <LegalPage doc="privacy" />;
}
