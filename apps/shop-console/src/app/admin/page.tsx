import { createClient } from "@/lib/supabase/server";

export default async function AdminDashboard() {
  const supabase = await createClient();
  const { data: products, error } = await supabase
    .from("products")
    .select("id, name, sku, sell_price, cost_price")
    .order("created_at", { ascending: false })
    .limit(20);

  return (
    <div className="space-y-4">
      <p className="text-sm text-neutral-600">
        Connected to Supabase. {products?.length ?? 0} product(s) found.
      </p>
      {error && <p className="text-sm text-red-600">{error.message}</p>}
      <ul className="divide-y divide-neutral-200 rounded border border-neutral-200 bg-white">
        {products?.map((p) => (
          <li
            key={p.id}
            className="flex justify-between px-4 py-2 text-sm text-neutral-900"
          >
            <span>{p.name}</span>
            <span className="text-neutral-500">{p.sku}</span>
          </li>
        ))}
      </ul>
    </div>
  );
}
