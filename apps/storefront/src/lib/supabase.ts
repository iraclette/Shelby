import { createClient } from "@supabase/supabase-js";

// Public storefront: read-only, anonymous. No auth session to manage, so a
// single plain client (no cookies/SSR helper needed) is enough here —
// unlike the Shop Console, this app never signs anyone in.
export const supabase = createClient(
  process.env.NEXT_PUBLIC_SUPABASE_URL!,
  process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!,
);

export function productImageUrl(storagePath: string) {
  return `${process.env.NEXT_PUBLIC_SUPABASE_URL}/storage/v1/object/public/product-images/${storagePath}`;
}

export type Category = {
  id: string;
  name: string;
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
};

export async function fetchCategories(): Promise<Category[]> {
  const { data } = await supabase.from("categories").select("id, name").order("name");
  return data ?? [];
}

export async function fetchProducts(categoryId?: string): Promise<Product[]> {
  let query = supabase
    .from("products")
    .select("id, name, description, sell_price, category_id, categories(id, name), product_images(storage_path, is_primary)")
    .order("created_at", { ascending: false });

  if (categoryId) {
    query = query.eq("category_id", categoryId);
  }

  const { data } = await query;
  return (data as unknown as Product[]) ?? [];
}

export async function fetchProduct(id: string): Promise<Product | null> {
  const { data } = await supabase
    .from("products")
    .select("id, name, description, sell_price, category_id, categories(id, name), product_images(storage_path, is_primary)")
    .eq("id", id)
    .maybeSingle();
  return data as unknown as Product | null;
}

export function primaryImage(product: Product): ProductImage | null {
  if (product.product_images.length === 0) return null;
  return product.product_images.find((img) => img.is_primary) ?? product.product_images[0];
}
