import type { ReactNode } from "react";
import Link from "next/link";
import { fetchShops } from "@/lib/supabase";
import { getDictionary } from "@/i18n/server";
import { fmt } from "@/i18n/format";

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
            <p className="text-xs tracking-[0.2em] text-brass uppercase">{dict.footer.getInTouch}</p>
            <ul className="mt-3 space-y-1 text-sm text-paper-dim">
              <li>
                <Link href="/contact" className="transition-colors hover:text-brass">
                  {dict.footer.contactUs}
                </Link>
              </li>
            </ul>
          </div>
        </div>
        <p className="mt-10 border-t border-line pt-6 text-xs text-paper-dim">
          {fmt(dict.footer.rights, { year: new Date().getFullYear() })}
        </p>
      </div>
    </footer>
  );
}
