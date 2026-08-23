import Link from "next/link";
import { fetchCategories, fetchProducts } from "@/lib/supabase";
import ProductCard from "@/components/ProductCard";

export const revalidate = 60;

export default async function Home() {
  const [categories, products] = await Promise.all([
    fetchCategories(),
    fetchProducts(),
  ]);

  const recentProducts = products.slice(0, 8);

  return (
    <div>
      <section className="border-b border-line">
        <div className="mx-auto max-w-6xl px-6 py-28 text-center">
          <p className="text-xs tracking-[0.3em] text-brass uppercase">
            Black Eye Beauty · Shelby · End
          </p>
          <h1 className="mt-6 font-display text-5xl italic text-paper sm:text-6xl">
            Fine Blades, Rich Leather
            <br />
            &amp; Everyday Beauty
          </h1>
          <p className="mx-auto mt-6 max-w-xl text-paper-dim">
            A curated shop of hunting knives, leather goods, cosmetics, and
            souvenirs. Browse the collection online, find us in store.
          </p>
          <Link
            href="/products"
            className="mt-10 inline-block border border-brass px-8 py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink"
          >
            Browse the Collection
          </Link>
        </div>
      </section>

      {categories.length > 0 && (
        <section className="mx-auto max-w-6xl px-6 py-20">
          <h2 className="font-display text-2xl text-paper">Shop by Category</h2>
          <div className="mt-8 grid grid-cols-2 gap-4 sm:grid-cols-3 md:grid-cols-5">
            {categories.map((category) => (
              <Link
                key={category.id}
                href={`/products?category=${category.id}`}
                className="border border-line px-4 py-6 text-center transition-colors hover:border-brass"
              >
                <span className="text-sm tracking-[0.1em] text-paper uppercase">
                  {category.name}
                </span>
              </Link>
            ))}
          </div>
        </section>
      )}

      {recentProducts.length > 0 && (
        <section className="mx-auto max-w-6xl px-6 pb-24">
          <h2 className="font-display text-2xl text-paper">New Arrivals</h2>
          <div className="mt-8 grid grid-cols-2 gap-6 sm:grid-cols-3 md:grid-cols-4">
            {recentProducts.map((product) => (
              <ProductCard key={product.id} product={product} />
            ))}
          </div>
        </section>
      )}
    </div>
  );
}
