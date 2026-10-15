import Link from "next/link";
import { getDictionary } from "@/i18n/server";

export default async function CheckoutFailPage() {
  const dict = await getDictionary();

  return (
    <div className="mx-auto max-w-xl px-6 py-24 text-center">
      <h1 className="font-display text-3xl text-paper">{dict.checkout.failTitle}</h1>
      <p className="mt-4 text-paper-dim">{dict.checkout.failText}</p>
      <Link href="/cart" className="mt-6 inline-block text-brass hover:underline">
        {dict.checkout.backToCart}
      </Link>
    </div>
  );
}
