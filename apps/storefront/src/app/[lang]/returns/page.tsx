import LegalPage, { legalMetadata } from "@/components/LegalPage";

export async function generateMetadata() {
  return legalMetadata("returns");
}

export default function ReturnsPage() {
  return <LegalPage doc="returns" />;
}
