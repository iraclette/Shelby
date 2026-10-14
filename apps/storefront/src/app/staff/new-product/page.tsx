import type { Metadata } from "next";
import NewProductClient from "./NewProductClient";

// Admin only; enforced by is_admin() RLS.
export const metadata: Metadata = {
  title: "Shelby — Add Product",
  robots: { index: false, follow: false },
};

export default function NewProductPage() {
  return <NewProductClient />;
}
