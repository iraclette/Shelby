import type { ReactNode } from "react";
import Link from "next/link";
import { fetchShops } from "@/lib/supabase";
import { getDictionary } from "@/i18n/server";
import { fmt } from "@/i18n/format";
import { BUSINESS } from "@/lib/business";

export async function FooterShops() {
  const shops = await fetchShops();
  return (
    <ul className="mt-3 space-y-1 text-sm text-paper-dim">
      {shops.map((shop) =>
        shop.maps_url ? (
          <li key={shop.id}>
            <a
              href={shop.maps_url}
              target="_blank"
              rel="noopener noreferrer"
              className="transition-colors hover:text-brass"
            >
              {shop.name}
            </a>
          </li>
        ) : (
          <li key={shop.id}>{shop.name}</li>
        ),
      )}
    </ul>
  );
}

// `children` is the shop list, wrapped in Suspense by the layout so the footer doesn't wait on it.
export default async function Footer({ children }: { children: ReactNode }) {
  const dict = await getDictionary();

  return (
    <footer className="border-t border-line bg-ink-soft">
      <div className="mx-auto max-w-6xl px-6 py-12">
        <div className="flex flex-col gap-8 sm:flex-row sm:justify-between">
          <div>
            <p className="font-display text-xl text-paper">Shelby</p>
            <p className="mt-2 max-w-xs text-sm text-paper-dim">
              {dict.footer.tagline}
            </p>
          </div>
          <div>
            <p className="text-xs tracking-[0.2em] text-brass uppercase">{dict.footer.visitUs}</p>
            {children}
          </div>
          <div>
            <p className="text-xs tracking-[0.2em] text-brass uppercase">{dict.footer.information}</p>
            <ul className="mt-3 space-y-1 text-sm text-paper-dim">
              {(
                [
                  ["/delivery", dict.footer.delivery],
                  ["/returns", dict.footer.returns],
                  ["/terms", dict.footer.terms],
                  ["/privacy", dict.footer.privacy],
                ] as const
              ).map(([href, label]) => (
                <li key={href}>
                  <Link href={href} className="transition-colors hover:text-brass">
                    {label}
                  </Link>
                </li>
              ))}
            </ul>
          </div>
          <div>
            <p className="text-xs tracking-[0.2em] text-brass uppercase">{dict.footer.getInTouch}</p>
            <ul className="mt-3 space-y-1 text-sm text-paper-dim">
              <li>
                <Link href="/contact" className="transition-colors hover:text-brass">
                  {dict.footer.contactUs}
                </Link>
              </li>
              <li>
                <Link href="/jobs" className="transition-colors hover:text-brass">
                  {dict.footer.jobs}
                </Link>
              </li>
            </ul>
          </div>
        </div>
        <div className="mt-10 flex flex-col gap-1 border-t border-line pt-6 text-xs text-paper-dim sm:flex-row sm:justify-between">
          <p>{fmt(dict.footer.rights, { year: new Date().getFullYear() })}</p>
          {/* Who the seller legally is: card schemes want this visible on every page. */}
          <p>
            {BUSINESS.legalName} · {BUSINESS.idNumber} · {BUSINESS.address}
          </p>
        </div>
      </div>
    </footer>
  );
}
