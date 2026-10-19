import Link from "next/link";
import type { Dictionary } from "@/i18n/dictionaries/en";

// Takes the place of "Add to cart", the cart and checkout while ONLINE_ORDERING is off.
export default function OrderByContact({ dict }: { dict: Dictionary }) {
  return (
    <div className="mt-6 border border-line bg-ink-soft p-5">
      <p className="text-sm tracking-[0.15em] text-paper uppercase">{dict.ordering.pausedTitle}</p>
      <p className="mt-2 text-sm text-paper-dim">{dict.ordering.pausedText}</p>
      <Link
        href="/contact"
        className="mt-4 flex w-full justify-center border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink"
      >
        {dict.ordering.contactUs}
      </Link>
    </div>
  );
}
