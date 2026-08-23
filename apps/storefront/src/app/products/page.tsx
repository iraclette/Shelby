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
        <p className="mt-16 text-paper-dim">No products here yet.</p>
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
