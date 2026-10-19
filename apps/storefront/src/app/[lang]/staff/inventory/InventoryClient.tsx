"use client";

import { Fragment, useCallback, useEffect, useState } from "react";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";
import CategoryOptions, { type StaffCategory } from "@/components/CategoryOptions";
import Skeleton from "@/components/Skeleton";
import { useI18n } from "@/i18n/client";
import { fmt } from "@/i18n/format";
import { generateInternalSku } from "@/lib/sku";

type Shop = { id: string; name: string };
type Level = { shop_id: string; variant_id: string | null; quantity: number };
type Variant = { id: string; name: string; sku: string | null };
type Product = {
  id: string;
  name: string;
  sku: string;
  category_id: string | null;
  inventory_levels: Level[];
  product_variants: Variant[];
};

export default function InventoryClient() {
  return <StaffAuthGate>{({ signOut }) => <Inventory onSignOut={signOut} />}</StaffAuthGate>;
}

function Inventory({ onSignOut }: { onSignOut: () => void }) {
  const supabase = getSupabaseClient();
  const { dict } = useI18n();

  const [shops, setShops] = useState<Shop[]>([]);
  const [products, setProducts] = useState<Product[]>([]);
  const [categories, setCategories] = useState<StaffCategory[]>([]);
  const [loading, setLoading] = useState(true);
  const [query, setQuery] = useState("");
  const [error, setError] = useState("");
  // The product whose "add variant" row is open, and what's typed in it.
  const [addingTo, setAddingTo] = useState<string | null>(null);
  const [variantName, setVariantName] = useState("");
  const [savingVariant, setSavingVariant] = useState(false);

  const load = useCallback(async () => {
    const [shopsRes, productsRes, categoriesRes] = await Promise.all([
      supabase.from("shops").select("id, name").order("name"),
      supabase
        .from("products")
        .select(
          "id, name, sku, category_id, inventory_levels(shop_id, variant_id, quantity), product_variants(id, name, sku)",
        )
        .order("name"),
      supabase.from("categories").select("id, name, parent_id").order("name"),
    ]);
    setShops((shopsRes.data as Shop[]) ?? []);
    const loaded = (productsRes.data as unknown as Product[]) ?? [];
    for (const product of loaded) {
      product.product_variants.sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true }));
    }
    setProducts(loaded);
    setCategories((categoriesRes.data as StaffCategory[]) ?? []);
    setLoading(false);
  }, [supabase]);

  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect -- loads from Supabase once on mount
    load();
  }, [load]);

  const filtered = products.filter((product) => {
    const text = query.trim().toLowerCase();
    if (!text) return true;
    return [product.name, product.sku, ...product.product_variants.flatMap((v) => [v.name, v.sku ?? ""])].some(
      (value) => value.toLowerCase().includes(text),
    );
  });

  // The first variant takes over the product's own stock (add_product_variant in 0028).
  async function addVariant(product: Product) {
    const name = variantName.trim();
    if (!name) return;
    setError("");
    setSavingVariant(true);
    let result = await supabase.rpc("add_product_variant", { p_product_id: product.id, p_name: name, p_sku: generateInternalSku() });
    // Retry on the rare random-SKU collision.
    for (let attempt = 0; attempt < 2 && result.error?.code === "23505"; attempt++) {
      result = await supabase.rpc("add_product_variant", { p_product_id: product.id, p_name: name, p_sku: generateInternalSku() });
    }
    setSavingVariant(false);
    if (result.error) {
      setError(result.error.code === "23505" ? fmt(dict.staff.variantExists, { name }) : dict.staff.variantFailed);
      return;
    }
    setAddingTo(null);
    setVariantName("");
    load();
  }

  // Shown right away; put back if the save fails.
  async function changeCategory(product: Product, categoryId: string | null) {
    const setCategoryOf = (value: string | null) =>
      setProducts((current) => current.map((p) => (p.id === product.id ? { ...p, category_id: value } : p)));
    setError("");
    setCategoryOf(categoryId);
    const { error } = await supabase.from("products").update({ category_id: categoryId }).eq("id", product.id);
    if (error) {
      setCategoryOf(product.category_id);
      setError(dict.staff.categoryFailed);
    }
  }

  // variantId undefined = the whole product, every variant added up.
  function levelsFor(product: Product, variantId?: string) {
    return product.inventory_levels.filter((level) => variantId === undefined || level.variant_id === variantId);
  }

  function quantityFor(product: Product, shopId: string, variantId?: string) {
    return levelsFor(product, variantId)
      .filter((level) => level.shop_id === shopId)
      .reduce((sum, level) => sum + level.quantity, 0);
  }

  function totalFor(product: Product, variantId?: string) {
    return levelsFor(product, variantId).reduce((sum, level) => sum + level.quantity, 0);
  }

  return (
    <div className="mx-auto max-w-4xl px-4 py-6 sm:px-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper sm:text-3xl">{dict.staff.currentStock}</h1>
        <button onClick={onSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          {dict.common.signOut}
        </button>
      </div>

      <input
        value={query}
        onChange={(e) => setQuery(e.target.value)}
        placeholder={dict.common.searchProducts}
        autoCapitalize="none"
        className="mt-4 w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
      />

      {error && <p className="mt-3 text-sm text-red-400">{error}</p>}

      {loading && (
        <div className="mt-6 space-y-3" aria-busy="true">
          {Array.from({ length: 8 }, (_, i) => (
            <Skeleton key={i} className="h-6 w-full" />
          ))}
        </div>
      )}

      {!loading && (
        <div className="mt-6 overflow-x-auto">
          <table className="w-full min-w-[640px] border-collapse text-sm">
            <thead>
              <tr className="border-b border-line text-left text-paper-dim">
                <th className="py-2 pr-4 font-normal">{dict.staff.product}</th>
                <th className="py-2 pr-4 font-normal">{dict.staff.sku}</th>
                <th className="py-2 pr-4 font-normal">{dict.staff.category}</th>
                {shops.map((shop) => (
                  <th key={shop.id} className="py-2 pr-4 text-right font-normal">
                    {shop.name}
                  </th>
                ))}
                <th className="py-2 text-right font-normal">{dict.staff.total}</th>
              </tr>
            </thead>
            <tbody>
              {filtered.map((product) => (
                <Fragment key={product.id}>
                  <tr className="border-b border-line/50">
                    <td className="py-2 pr-4 text-paper">
                      {product.name}
                      <button
                        onClick={() => {
                          setAddingTo(addingTo === product.id ? null : product.id);
                          setVariantName("");
                        }}
                        className="block min-h-9 text-xs text-brass active:text-paper"
                      >
                        {dict.staff.addVariant}
                      </button>
                    </td>
                    <td className="py-2 pr-4 text-paper-dim">{product.sku}</td>
                    <td className="py-2 pr-4">
                      <select
                        value={product.category_id ?? ""}
                        onChange={(e) => changeCategory(product, e.target.value || null)}
                        aria-label={dict.staff.category}
                        className="min-h-9 max-w-48 rounded-md border border-line bg-ink-soft px-2 text-sm text-paper outline-none focus:border-brass"
                      >
                        <CategoryOptions categories={categories} dict={dict} />
                      </select>
                    </td>
                    {shops.map((shop) => (
                      <td key={shop.id} className="py-2 pr-4 text-right text-paper-dim">
                        {quantityFor(product, shop.id)}
                      </td>
                    ))}
                    <td className="py-2 text-right font-medium text-paper">{totalFor(product)}</td>
                  </tr>
                  {product.product_variants.map((variant) => (
                    <tr key={variant.id} className="border-b border-line/50 text-paper-dim">
                      <td className="py-2 pr-4 pl-4">↳ {variant.name}</td>
                      <td className="py-2 pr-4">{variant.sku}</td>
                      <td />
                      {shops.map((shop) => (
                        <td key={shop.id} className="py-2 pr-4 text-right">
                          {quantityFor(product, shop.id, variant.id)}
                        </td>
                      ))}
                      <td className="py-2 text-right">{totalFor(product, variant.id)}</td>
                    </tr>
                  ))}
                  {addingTo === product.id && (
                    <tr className="border-b border-line/50">
                      <td colSpan={shops.length + 4} className="py-2">
                        <form
                          onSubmit={(e) => {
                            e.preventDefault();
                            addVariant(product);
                          }}
                          className="flex flex-wrap items-center gap-2"
                        >
                          <input
                            autoFocus
                            value={variantName}
                            onChange={(e) => setVariantName(e.target.value)}
                            placeholder={dict.staff.variantPlaceholder}
                            className="min-h-11 flex-1 rounded-md border border-line bg-ink-soft px-3 text-base text-paper outline-none focus:border-brass"
                          />
                          <button
                            type="submit"
                            disabled={savingVariant || !variantName.trim()}
                            className="min-h-11 rounded-md bg-brass px-4 text-sm font-medium text-ink disabled:opacity-60"
                          >
                            {dict.staff.add}
                          </button>
                        </form>
                        {product.product_variants.length === 0 && totalFor(product) > 0 && (
                          <p className="mt-1 text-xs text-paper-dim">{dict.staff.firstVariantStock}</p>
                        )}
                      </td>
                    </tr>
                  )}
                </Fragment>
              ))}
            </tbody>
          </table>
          {filtered.length === 0 && (
            <p className="mt-4 text-sm text-paper-dim">{fmt(dict.common.noMatch, { query })}</p>
          )}
        </div>
      )}
    </div>
  );
}
