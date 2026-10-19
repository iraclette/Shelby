import { Suspense } from "react";
import Link from "next/link";
import { fetchCategories, fetchProducts } from "@/lib/supabase";
import ProductCard from "@/components/ProductCard";
import { ProductGridSkeleton } from "@/components/Skeleton";
import { getDictionary } from "@/i18n/server";
import { categoryName } from "@/i18n/format";
import { subcategoriesOf, topLevelCategories } from "@/lib/categoryGroups";

export const revalidate = 60;

export default async function ProductsPage({
  searchParams,
}: PageProps<"/[lang]/products">) {
  const { category: categoryParam } = await searchParams;
  const category = typeof categoryParam === "string" ? categoryParam : undefined;
  const [categories, dict] = await Promise.all([fetchCategories(), getDictionary()]);
  // The top-level chip stays lit while one of its subcategories is picked.
  const selected = categories.find((c) => c.id === category);
  const activeTop = selected?.parent_id ?? category;
  const subcategories = activeTop ? subcategoriesOf(categories, activeTop) : [];
  const chip = (active: boolean) =>
    `border px-4 py-2 text-xs tracking-[0.1em] uppercase transition-colors ${
      active ? "border-brass text-brass" : "border-line text-paper-dim hover:border-brass hover:text-brass"
    }`;

  return (
    <div className="mx-auto max-w-6xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">{dict.products.title}</h1>

      <div className="mt-6 flex flex-wrap gap-3">
        <Link href="/products" className={chip(!category)}>
          {dict.products.all}
        </Link>
        {topLevelCategories(categories).map((c) => (
          <Link key={c.id} href={`/products?category=${c.id}`} className={chip(activeTop === c.id)}>
            {categoryName(dict, c.name)}
          </Link>
        ))}
      </div>

      {subcategories.length > 0 && (
        <div className="mt-4 flex flex-wrap gap-3 border-l border-brass/40 pl-4">
          <Link href={`/products?category=${activeTop}`} className={chip(category === activeTop)}>
            {dict.products.all}
          </Link>
          {subcategories.map((c) => (
            <Link key={c.id} href={`/products?category=${c.id}`} className={chip(category === c.id)}>
              {categoryName(dict, c.name)}
            </Link>
          ))}
        </div>
      )}

      {/* Keyed on the category so switching category is new content: React shows the
          skeleton instead of keeping the previous category's grid until the new one arrives. */}
      <Suspense key={category ?? "all"} fallback={<ProductGridSkeleton />}>
        <ProductGrid category={category} />
      </Suspense>
    </div>
  );
}

async function ProductGrid({ category }: { category?: string }) {
  const [products, dict] = await Promise.all([fetchProducts(category), getDictionary()]);

  return products.length === 0 ? (
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
  );
}
