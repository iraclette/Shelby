import type { SupabaseClient } from "@supabase/supabase-js";
import { randomId } from "@/lib/randomId";
import { makeThumbnail, resizeImage } from "@/lib/resizeImage";

// Uploads a photo and its thumbnail (<name>.thumb.jpg, same as shop-console)
// to product-images. Returns the paths for the product_images row, or null if
// the photo itself failed; a failed thumbnail just leaves thumb_path null.
export async function uploadProductPhoto(
  supabase: SupabaseClient,
  productId: string,
  original: File,
): Promise<{ storage_path: string; thumb_path: string | null } | null> {
  const [file, thumb] = await Promise.all([resizeImage(original), makeThumbnail(original)]);
  const name = `${productId}/${randomId()}`;
  const extension = file.name.includes(".") ? file.name.split(".").pop() : "jpg";
  const storagePath = `${name}.${extension}`;

  const { error } = await supabase.storage
    .from("product-images")
    .upload(storagePath, file, { contentType: file.type || "image/jpeg" });
  if (error) return null;

  let thumbPath: string | null = null;
  if (thumb) {
    const { error: thumbError } = await supabase.storage
      .from("product-images")
      .upload(`${name}.thumb.jpg`, thumb, { contentType: "image/jpeg" });
    if (!thumbError) thumbPath = `${name}.thumb.jpg`;
  }
  return { storage_path: storagePath, thumb_path: thumbPath };
}

export async function removeProductPhoto(
  supabase: SupabaseClient,
  image: { id: string; storage_path: string; thumb_path: string | null },
) {
  await supabase.storage
    .from("product-images")
    .remove(image.thumb_path ? [image.storage_path, image.thumb_path] : [image.storage_path]);
  await supabase.from("product_images").delete().eq("id", image.id);
}
