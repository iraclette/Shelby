import { existsSync } from "node:fs";
import path from "node:path";
import Image from "next/image";
import Link from "next/link";
import { fetchCategories, fetchProducts, fetchShops } from "@/lib/supabase";
import { PROMISES, SHOP_BLURBS, TOWN } from "@/lib/storeInfo";
import ProductCard from "@/components/ProductCard";
import CategoryIcon from "@/components/CategoryIcon";
import { MapPin } from "lucide-react";

export const revalidate = 60;

// Optional; renders behind the overlay when present (see public/hero/README.md).
const HERO_PHOTO = "/hero/hero.jpg";
const hasHeroPhoto = existsSync(path.join(process.cwd(), "public", HERO_PHOTO));

const sectionHeading = "font-display text-2xl text-paper";
const eyebrow = "text-xs tracking-[0.3em] text-brass uppercase";

export default async function Home() {
  const [categories, products, shops] = await Promise.all([
    fetchCategories(),
    fetchProducts(),
    fetchShops(),
  ]);

  const recentProducts = products.slice(0, 8);

  return (
    <div>
      <section className="relative overflow-hidden border-b border-line">
        {hasHeroPhoto && (
          <Image src={HERO_PHOTO} alt="" fill priority className="object-cover" />
        )}
        <div className="atmosphere" />
        <div className="relative mx-auto max-w-6xl px-6 py-28 text-center">
          <p className={eyebrow}>{TOWN} · Black Eye Beauty · Shelby · End</p>
          <h1 className="mt-6 font-display text-5xl italic text-paper sm:text-6xl">
            Fine Blades, Rich Leather
            <br />
            &amp; Everyday Beauty
          </h1>
          <p className="mx-auto mt-6 max-w-xl text-paper-dim">
            Knives of every kind, katanas, leather goods and gift boxes, cosmetics, skincare
            and perfume. Browse online, pick up in any of our three shops.
          </p>
          <div className="mt-10 flex flex-wrap justify-center gap-4">
            <Link
              href="/products"
              className="inline-block border border-brass px-8 py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink"
            >
              Browse the Collection
            </Link>
            <a
              href="#shops"
              className="inline-block border border-line px-8 py-3 text-sm tracking-[0.15em] text-paper-dim uppercase transition-colors hover:border-paper-dim hover:text-paper"
            >
              Visit a Shop
            </a>
          </div>
        </div>
      </section>

      <section className="border-b border-line bg-ink-soft">
        <ul className="mx-auto grid max-w-6xl grid-cols-2 gap-px px-6 md:grid-cols-4">
          {PROMISES.map((promise) => (
            <li key={promise.title} className="py-6 pr-4">
              <p className="text-xs tracking-[0.2em] text-brass uppercase">{promise.title}</p>
              <p className="mt-1 text-sm text-paper-dim">{promise.text}</p>
            </li>
          ))}
        </ul>
      </section>

      {categories.length > 0 && (
        <section className="mx-auto max-w-6xl px-6 py-20">
          <h2 className={sectionHeading}>Shop by Category</h2>
          <div className="mt-8 grid grid-cols-2 gap-4 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-6">
            {categories.map((category) => (
              <Link
                key={category.id}
                href={`/products?category=${category.id}`}
                className="group relative flex flex-col items-center gap-3 overflow-hidden border border-line px-3 py-6 text-center transition-colors hover:border-brass"
              >
                <div className="atmosphere-subtle transition-opacity group-hover:opacity-80" />
                <CategoryIcon
                  name={category.name}
                  className="relative h-9 w-9 text-brass transition-colors group-hover:text-brass-soft"
                />
                <span className="relative text-xs tracking-[0.12em] text-paper uppercase">
                  {category.name}
                </span>
              </Link>
            ))}
          </div>
        </section>
      )}

      <section className="mx-auto max-w-6xl px-6 pb-20">
        <h2 className={sectionHeading}>New Arrivals</h2>
        {recentProducts.length > 0 ? (
          <div className="mt-8 grid grid-cols-2 gap-6 sm:grid-cols-3 md:grid-cols-4">
            {recentProducts.map((product) => (
              <ProductCard key={product.id} product={product} />
            ))}
          </div>
        ) : (
          <div className="relative mt-8 overflow-hidden border border-line px-6 py-14 text-center">
            <div className="atmosphere-subtle" />
            <p className="relative font-display text-2xl italic text-paper">
              New pieces are on their way.
            </p>
            <p className="relative mx-auto mt-3 max-w-md text-sm text-paper-dim">
              We&apos;re putting the collection online now. Until then, everything is waiting
              for you in store.
            </p>
            <a
              href="#shops"
              className="relative mt-6 inline-block text-sm tracking-[0.15em] text-brass uppercase hover:text-brass-soft"
            >
              Find a shop →
            </a>
          </div>
        )}
      </section>

      {shops.length > 0 && (
        <section id="shops" className="scroll-mt-24 border-t border-line bg-ink-soft">
          <div className="mx-auto max-w-6xl px-6 py-20">
            <p className={eyebrow}>{TOWN}</p>
            <h2 className={`mt-3 ${sectionHeading}`}>Our Shops</h2>
            <p className="mt-3 max-w-xl text-paper-dim">
              Three shops, one family. Each has its own character; all of them hold your online
              orders for pickup.
            </p>
            <div className="mt-10 grid gap-6 md:grid-cols-3">
              {shops.map((shop) => (
                <div key={shop.id} className="flex flex-col border border-line bg-ink p-6">
                  <h3 className="font-display text-2xl text-paper">{shop.name}</h3>
                  {SHOP_BLURBS[shop.name] && (
                    <p className="mt-3 flex-1 text-sm leading-relaxed text-paper-dim">
                      {SHOP_BLURBS[shop.name]}
                    </p>
                  )}
                  {shop.maps_url && (
                    <a
                      href={shop.maps_url}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="mt-6 inline-flex items-center gap-2 text-sm tracking-[0.12em] text-brass uppercase transition-colors hover:text-brass-soft"
                    >
                      <MapPin className="h-4 w-4" strokeWidth={1.5} aria-hidden="true" />
                      Directions
                    </a>
                  )}
                </div>
              ))}
            </div>
          </div>
        </section>
      )}
    </div>
  );
}
