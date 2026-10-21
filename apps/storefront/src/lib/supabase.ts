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
  parent_id?: string | null; // a subcategory's parent (0026_subcategories.sql)
};

export type ProductImage = {
  storage_path: string;
  is_primary: boolean;
  thumb_path: string | null; // ~400px copy for cards (0037_search_and_thumbnails.sql)
  variant_id?: string | null; // a photo of one variant (0028_storefront_variants.sql)
};

export type ProductVariant = {
  id: string;
  name: string;
  inStock?: boolean; // only filled in by fetchProduct
};

export type Product = {
  id: string;
  name: string;
  description: string | null;
  sell_price: number;
  category_id: string | null;
  categories: Category | null;
  product_images: ProductImage[];
  product_variants: ProductVariant[];
  inStock: boolean; // summed over every shop; filled in by fetchProducts/fetchProduct
};

export async function fetchCategories(): Promise<Category[]> {
  // Only categories the admin has toggled visible — every caller here is
  // customer-facing (nav, /products filter chips), so filter at the source.
  const { data } = await getSupabaseClient()
    .from("categories")
    .select("id, name, visible_on_storefront, parent_id")
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
  "id, name, description, sell_price, category_id, categories(id, name, visible_on_storefront), product_images(storage_path, thumb_path, is_primary, variant_id), product_variants(id, name)";

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

export const PRODUCTS_PER_PAGE = 48;

export type ProductPage = { products: Product[]; total: number };

// One page of the catalog, in-stock first then newest. Paging, ordering and the
// hidden-category filter run in SQL (0036_storefront_product_pages.sql), so a
// request costs the same however big the catalog gets.
export async function fetchProducts({
  categoryId,
  search,
  page = 1,
  pageSize = PRODUCTS_PER_PAGE,
}: { categoryId?: string; search?: string; page?: number; pageSize?: number } = {}): Promise<ProductPage> {
  let categoryIds: string[] | null = null;
  if (categoryId) {
    // A parent category also lists everything in its subcategories.
    const { data: subcategories } = await getSupabaseClient().from("categories").select("id").eq("parent_id", categoryId);
    categoryIds = [categoryId, ...(subcategories ?? []).map((c) => c.id as string)];
  }

  const { data: rows } = await getSupabaseClient().rpc("storefront_product_page", {
    category_ids: categoryIds,
    page_size: pageSize,
    page_offset: (page - 1) * pageSize,
    search: search || null,
  });
  const pageRows = (rows as { product_id: string; in_stock: boolean; total: number }[] | null) ?? [];
  if (pageRows.length === 0) return { products: [], total: 0 };

  const { data } = await getSupabaseClient()
    .from("products")
    .select(PRODUCT_COLUMNS)
    .in(
      "id",
      pageRows.map((row) => row.product_id),
    );
  const byId = new Map(((data as unknown as Product[]) ?? []).map((product) => [product.id, product]));
  return {
    products: pageRows.flatMap((row) => {
      const product = byId.get(row.product_id);
      return product ? [{ ...product, inStock: row.in_stock }] : [];
    }),
    total: Number(pageRows[0].total),
  };
}

export async function fetchProduct(id: string): Promise<Product | null> {
  const [{ data }, inStock, variantsInStock] = await Promise.all([
    getSupabaseClient().from("products").select(PRODUCT_COLUMNS).eq("id", id).maybeSingle(),
    fetchInStockIds(),
    getSupabaseClient().rpc("in_stock_variant_ids"),
  ]);
  const product = data as unknown as Product | null;
  if (!product || !onStorefront(product)) return null;
  const variantStock = new Set((variantsInStock.data as string[] | null) ?? []);
  return {
    ...product,
    inStock: inStock.has(product.id),
    product_variants: [...product.product_variants]
      .sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true }))
      .map((variant) => ({ ...variant, inStock: variantStock.has(variant.id) })),
  };
}

export function primaryImage(product: Pick<Product, "product_images">): ProductImage | null {
  if (product.product_images.length === 0) return null;
  return product.product_images.find((img) => img.is_primary) ?? product.product_images[0];
}
