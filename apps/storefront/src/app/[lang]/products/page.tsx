import { Suspense } from "react";
import Link from "next/link";
import { fetchCategories, fetchProducts, PRODUCTS_PER_PAGE } from "@/lib/supabase";
import ProductCard from "@/components/ProductCard";
import { ProductGridSkeleton } from "@/components/Skeleton";
import { getDictionary } from "@/i18n/server";
import { categoryName, fmt } from "@/i18n/format";
import { subcategoriesOf, topLevelCategories } from "@/lib/categoryGroups";

export const revalidate = 60;

export default async function ProductsPage({
  searchParams,
}: PageProps<"/[lang]/products">) {
  const { category: categoryParam, page: pageParam, q } = await searchParams;
  const category = typeof categoryParam === "string" ? categoryParam : undefined;
  const search = typeof q === "string" ? q.trim().slice(0, 100) : "";
  const page = Math.max(1, parseInt(typeof pageParam === "string" ? pageParam : "", 10) || 1);
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

      {/* Plain GET form: works without JS, and the query lands in the URL. */}
      <form action="/products" className="mt-6 flex max-w-md gap-3">
        {category && <input type="hidden" name="category" value={category} />}
        <input
          type="search"
          name="q"
          defaultValue={search}
          placeholder={dict.products.searchPlaceholder}
          aria-label={dict.products.searchPlaceholder}
          className="min-w-0 flex-1 border border-line bg-transparent px-4 py-2 text-sm text-paper placeholder:text-paper-dim/70 focus:border-brass focus:outline-none"
        />
        <button
          type="submit"
          className="border border-brass px-4 py-2 text-xs tracking-[0.1em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink"
        >
          {dict.products.search}
        </button>
      </form>

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
      <Suspense key={`${category ?? "all"}-${search}-${page}`} fallback={<ProductGridSkeleton />}>
        <ProductGrid category={category} search={search} page={page} />
      </Suspense>
    </div>
  );
}

async function ProductGrid({ category, search, page }: { category?: string; search: string; page: number }) {
  const [{ products, total }, dict] = await Promise.all([
    fetchProducts({ categoryId: category, search, page }),
    getDictionary(),
  ]);
  const pages = Math.ceil(total / PRODUCTS_PER_PAGE);
  const pageHref = (n: number) => {
    const params = new URLSearchParams();
    if (category) params.set("category", category);
    if (search) params.set("q", search);
    if (n > 1) params.set("page", String(n));
    const query = params.toString();
    return query ? `/products?${query}` : "/products";
  };
  const pageLink = "text-sm tracking-[0.15em] text-brass uppercase hover:text-brass-soft";

  return products.length === 0 ? (
    <div className="relative mt-12 overflow-hidden border border-line px-6 py-14 text-center">
      <div className="atmosphere-subtle" />
      <p className="relative font-display text-2xl italic text-paper">
        {search
          ? fmt(dict.products.noResults, { query: search })
          : category
            ? dict.products.emptyCategory
            : dict.products.emptyAll}
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
    <>
      <div className="mt-10 grid grid-cols-2 gap-6 sm:grid-cols-3 md:grid-cols-4">
        {products.map((product) => (
          <ProductCard key={product.id} product={product} />
        ))}
      </div>

      {pages > 1 && (
        <nav className="mt-12 flex items-center justify-between border-t border-line pt-6">
          {page > 1 ? (
            <Link href={pageHref(page - 1)} className={pageLink}>
              {dict.products.previous}
            </Link>
          ) : (
            <span />
          )}
          <span className="text-xs tracking-[0.1em] text-paper-dim uppercase">
            {fmt(dict.products.pageOf, { page, pages })}
          </span>
          {page < pages ? (
            <Link href={pageHref(page + 1)} className={pageLink}>
              {dict.products.next}
            </Link>
          ) : (
            <span />
          )}
        </nav>
      )}
    </>
  );
}
