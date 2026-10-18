import LegalPage, { legalMetadata } from "@/components/LegalPage";

export async function generateMetadata() {
  return legalMetadata("delivery");
}

export default function DeliveryPage() {
  return <LegalPage doc="delivery" />;
}
