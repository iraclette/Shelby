import { getDictionary } from "@/i18n/server";
import { fmt } from "@/i18n/format";

export default async function CheckoutSuccessPage({ searchParams }: PageProps<"/[lang]/checkout/success">) {
  const { order } = await searchParams;
  const dict = await getDictionary();

  return (
    <div className="mx-auto max-w-xl px-6 py-24 text-center">
      <h1 className="font-display text-3xl text-paper">{dict.checkout.successTitle}</h1>
      <p className="mt-4 text-paper-dim">
        {dict.checkout.successText}
      </p>
      {/* Same short code the Shop Console shows, so it can be read out at pickup. */}
      {typeof order === "string" && (
        <p className="mt-2 text-xs text-paper-dim">{fmt(dict.checkout.order, { id: order.slice(0, 8).toUpperCase() })}</p>
      )}
    </div>
  );
}
