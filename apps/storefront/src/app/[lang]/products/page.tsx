import Link from "next/link";
import { fetchCategories, fetchProducts } from "@/lib/supabase";
import ProductCard from "@/components/ProductCard";
import { getDictionary } from "@/i18n/server";
import { categoryName } from "@/i18n/format";

export const revalidate = 60;

export default async function ProductsPage({
  searchParams,
}: PageProps<"/[lang]/products">) {
  const { category: categoryParam } = await searchParams;
  const category = typeof categoryParam === "string" ? categoryParam : undefined;
  const [categories, products, dict] = await Promise.all([
    fetchCategories(),
    fetchProducts(category),
    getDictionary(),
  ]);

  return (
    <div className="mx-auto max-w-6xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">{dict.products.title}</h1>

      <div className="mt-6 flex flex-wrap gap-3">
        <Link
          href="/products"
          className={`border px-4 py-2 text-xs tracking-[0.1em] uppercase transition-colors ${
            !category
              ? "border-brass text-brass"
              : "border-line text-paper-dim hover:border-brass hover:text-brass"
          }`}
        >
          {dict.products.all}
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
            {categoryName(dict, c.name)}
          </Link>
        ))}
      </div>

      {products.length === 0 ? (
        <div className="relative mt-12 overflow-hidden border border-line px-6 py-14 text-center">
          <div className="atmosphere-subtle" />
          <p className="relative font-display text-2xl italic text-paper">
            {category ? dict.products.emptyCategory : dict.products.emptyAll}
          </p>
          <p className="relative mx-auto mt-3 max-w-md text-sm text-paper-dim">
            {dict.products.emptyText}
          </p>
          <Link
            href="/#shops"
            className="relative mt-6 inline-block text-sm tracking-[0.15em] text-brass uppercase hover:text-brass-soft"
          >
            {dict.common.findShop}
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
