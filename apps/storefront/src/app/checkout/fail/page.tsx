import Link from "next/link";

export default function CheckoutFailPage() {
  return (
    <div className="mx-auto max-w-xl px-6 py-24 text-center">
      <h1 className="font-display text-3xl text-paper">Payment didn&apos;t go through</h1>
      <p className="mt-4 text-paper-dim">Nothing was charged. You can try again.</p>
      <Link href="/cart" className="mt-6 inline-block text-brass hover:underline">
        Back to cart
      </Link>
    </div>
  );
}
