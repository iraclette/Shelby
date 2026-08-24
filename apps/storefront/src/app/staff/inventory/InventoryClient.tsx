"use client";

import { useEffect, useState } from "react";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";

type Shop = { id: string; name: string };
type Product = {
  id: string;
  name: string;
  sku: string;
  inventory_levels: { shop_id: string; quantity: number }[];
};

export default function InventoryClient() {
  return <StaffAuthGate>{({ signOut }) => <Inventory onSignOut={signOut} />}</StaffAuthGate>;
}

function Inventory({ onSignOut }: { onSignOut: () => void }) {
  const supabase = getSupabaseClient();

  const [shops, setShops] = useState<Shop[]>([]);
  const [products, setProducts] = useState<Product[]>([]);
  const [loading, setLoading] = useState(true);
  const [query, setQuery] = useState("");

  useEffect(() => {
    Promise.all([
      supabase.from("shops").select("id, name").order("name"),
      supabase
        .from("products")
        .select("id, name, sku, inventory_levels(shop_id, quantity)")
        .order("name"),
    ]).then(([shopsRes, productsRes]) => {
      setShops((shopsRes.data as Shop[]) ?? []);
      setProducts((productsRes.data as unknown as Product[]) ?? []);
      setLoading(false);
    });
  }, [supabase]);

  const filtered = products.filter((product) => {
    const text = query.trim().toLowerCase();
    if (!text) return true;
    return product.name.toLowerCase().includes(text) || product.sku.toLowerCase().includes(text);
  });

  function quantityFor(product: Product, shopId: string) {
    return product.inventory_levels.find((level) => level.shop_id === shopId)?.quantity ?? 0;
  }

  function totalFor(product: Product) {
    return product.inventory_levels.reduce((sum, level) => sum + level.quantity, 0);
  }

  return (
    <div className="mx-auto max-w-4xl px-4 py-6 sm:px-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper sm:text-3xl">Current Stock</h1>
        <button onClick={onSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          Sign out
        </button>
      </div>

      <input
        value={query}
        onChange={(e) => setQuery(e.target.value)}
        placeholder="Search products…"
        autoCapitalize="none"
        className="mt-4 w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
      />

      {loading && <p className="mt-6 text-sm text-paper-dim">Loading…</p>}

      {!loading && (
        <div className="mt-6 overflow-x-auto">
          <table className="w-full min-w-[480px] border-collapse text-sm">
            <thead>
              <tr className="border-b border-line text-left text-paper-dim">
                <th className="py-2 pr-4 font-normal">Product</th>
                <th className="py-2 pr-4 font-normal">SKU</th>
                {shops.map((shop) => (
                  <th key={shop.id} className="py-2 pr-4 text-right font-normal">
                    {shop.name}
                  </th>
                ))}
                <th className="py-2 text-right font-normal">Total</th>
              </tr>
            </thead>
            <tbody>
              {filtered.map((product) => (
                <tr key={product.id} className="border-b border-line/50">
                  <td className="py-2 pr-4 text-paper">{product.name}</td>
                  <td className="py-2 pr-4 text-paper-dim">{product.sku}</td>
                  {shops.map((shop) => (
                    <td key={shop.id} className="py-2 pr-4 text-right text-paper-dim">
                      {quantityFor(product, shop.id)}
                    </td>
                  ))}
                  <td className="py-2 text-right font-medium text-paper">{totalFor(product)}</td>
                </tr>
              ))}
            </tbody>
          </table>
          {filtered.length === 0 && (
            <p className="mt-4 text-sm text-paper-dim">No products match &quot;{query}&quot;.</p>
          )}
        </div>
      )}
    </div>
  );
}
