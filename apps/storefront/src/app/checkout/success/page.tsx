export default async function CheckoutSuccessPage({
  searchParams,
}: {
  searchParams: Promise<{ order?: string }>;
}) {
  const { order } = await searchParams;

  return (
    <div className="mx-auto max-w-xl px-6 py-24 text-center">
      <h1 className="font-display text-3xl text-paper">Thank you!</h1>
      <p className="mt-4 text-paper-dim">
        Your payment was received. We&apos;ll have your order ready for pickup
        shortly.
      </p>
      {order && <p className="mt-2 text-xs text-paper-dim/60">Order #{order}</p>}
    </div>
  );
}
