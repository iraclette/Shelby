import LegalPage, { legalMetadata } from "@/components/LegalPage";

export async function generateMetadata() {
  return legalMetadata("terms");
}

export default function TermsPage() {
  return <LegalPage doc="terms" />;
}
