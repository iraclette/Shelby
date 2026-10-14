import Link from "next/link";
import { fetchCategories, fetchProducts } from "@/lib/supabase";
import ProductCard from "@/components/ProductCard";

export const revalidate = 60;

export default async function ProductsPage({
  searchParams,
}: {
  searchParams: Promise<{ category?: string }>;
}) {
  const { category } = await searchParams;
  const [categories, products] = await Promise.all([
    fetchCategories(),
    fetchProducts(category),
  ]);

  return (
    <div className="mx-auto max-w-6xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">Shop</h1>

      <div className="mt-6 flex flex-wrap gap-3">
        <Link
          href="/products"
          className={`border px-4 py-2 text-xs tracking-[0.1em] uppercase transition-colors ${
            !category
              ? "border-brass text-brass"
              : "border-line text-paper-dim hover:border-brass hover:text-brass"
          }`}
        >
          All
        </Link>
        {categories.map((c) => (
          <Link
            key={c.id}
            href={`/products?category=${c.id}`}
            className={`border px-4 py-2 text-xs tracking-[0.1em] uppercase transition-colors ${
              category === c.id
                ? "border-brass text-brass"
                : "border-line text-paper-dim hover:border-brass hover:text-brass"
            }`}
          >
            {c.name}
          </Link>
        ))}
      </div>

      {products.length === 0 ? (
        <div className="relative mt-12 overflow-hidden border border-line px-6 py-14 text-center">
          <div className="atmosphere-subtle" />
          <p className="relative font-display text-2xl italic text-paper">
            {category ? "Nothing online in this category yet." : "The collection is coming online."}
          </p>
          <p className="relative mx-auto mt-3 max-w-md text-sm text-paper-dim">
            Our shops in Gori have it on the shelves. Drop by, or check back soon.
          </p>
          <Link
            href="/#shops"
            className="relative mt-6 inline-block text-sm tracking-[0.15em] text-brass uppercase hover:text-brass-soft"
          >
            Find a shop →
          </Link>
        </div>
      ) : (
        <div className="mt-10 grid grid-cols-2 gap-6 sm:grid-cols-3 md:grid-cols-4">
          {products.map((product) => (
            <ProductCard key={product.id} product={product} />
          ))}
        </div>
      )}
    </div>
  );
}
