import { createClient, type SupabaseClient } from "@supabase/supabase-js";

// Read-only anon client. Created lazily so `next build` doesn't crash without env vars.
let cached: SupabaseClient | null = null;

export function getSupabaseClient(): SupabaseClient {
  if (cached) return cached;
  cached = createClient(
    process.env.NEXT_PUBLIC_SUPABASE_URL!,
    process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!,
  );
  return cached;
}

export function productImageUrl(storagePath: string) {
  return `${process.env.NEXT_PUBLIC_SUPABASE_URL}/storage/v1/object/public/product-images/${storagePath}`;
}

export type Category = {
  id: string;
  name: string;
  visible_on_storefront: boolean;
};

export type ProductImage = {
  storage_path: string;
  is_primary: boolean;
};

export type Product = {
  id: string;
  name: string;
  description: string | null;
  sell_price: number;
  category_id: string | null;
  categories: Category | null;
  product_images: ProductImage[];
  inStock: boolean; // summed over every shop; filled in by fetchProducts/fetchProduct
};

export async function fetchCategories(): Promise<Category[]> {
  // Only categories the admin has toggled visible — every caller here is
  // customer-facing (nav, /products filter chips), so filter at the source.
  const { data } = await getSupabaseClient()
    .from("categories")
    .select("id, name, visible_on_storefront")
    .eq("visible_on_storefront", true)
    .order("name");
  return data ?? [];
}

export type Shop = {
  id: string;
  name: string;
  maps_url: string | null;
};

export async function fetchShops(): Promise<Shop[]> {
  const { data } = await getSupabaseClient().from("shops").select("id, name, maps_url").order("name");
  return data ?? [];
}

const PRODUCT_COLUMNS =
  "id, name, description, sell_price, category_id, categories(id, name, visible_on_storefront), product_images(storage_path, is_primary)";

// A product in a category hidden from the website (the Shop Console's "show on
// website" switch) is hidden too; uncategorised products always show.
function onStorefront(product: Product) {
  return product.categories?.visible_on_storefront !== false;
}

// Ids of products with stock in any shop. Counts stay private: anon can't read
// inventory_levels, only this function (0025_stock_requests.sql).
async function fetchInStockIds(): Promise<Set<string>> {
  const { data } = await getSupabaseClient().rpc("in_stock_product_ids");
  return new Set((data as string[] | null) ?? []);
}

export async function fetchProducts(categoryId?: string): Promise<Product[]> {
  let query = getSupabaseClient()
    .from("products")
    .select(PRODUCT_COLUMNS)
    .order("created_at", { ascending: false });

  if (categoryId) {
    query = query.eq("category_id", categoryId);
  }

  const [{ data }, inStock] = await Promise.all([query, fetchInStockIds()]);
  return ((data as unknown as Product[]) ?? [])
    .filter(onStorefront)
    .map((product) => ({ ...product, inStock: inStock.has(product.id) }))
    // Newest first still, but what can be bought comes before what can't.
    .sort((a, b) => Number(b.inStock) - Number(a.inStock));
}

export async function fetchProduct(id: string): Promise<Product | null> {
  const [{ data }, inStock] = await Promise.all([
    getSupabaseClient().from("products").select(PRODUCT_COLUMNS).eq("id", id).maybeSingle(),
    fetchInStockIds(),
  ]);
  const product = data as unknown as Product | null;
  return product && onStorefront(product) ? { ...product, inStock: inStock.has(product.id) } : null;
}

export function primaryImage(product: Product): ProductImage | null {
  if (product.product_images.length === 0) return null;
  return product.product_images.find((img) => img.is_primary) ?? product.product_images[0];
}
