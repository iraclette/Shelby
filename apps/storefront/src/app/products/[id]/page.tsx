import { notFound } from "next/navigation";
import { fetchProduct, primaryImage } from "@/lib/supabase";
import AddToCartButton from "@/components/AddToCartButton";
import ProductGallery from "@/components/ProductGallery";

export const revalidate = 60;

export default async function ProductPage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const { id } = await params;
  const product = await fetchProduct(id);
  if (!product) notFound();

  const featured = primaryImage(product);
  const galleryImages = featured
    ? [featured, ...product.product_images.filter((image) => image !== featured)]
    : [];

  return (
    <div className="mx-auto max-w-6xl px-6 py-16">
      <div className="grid gap-12 md:grid-cols-2">
        <ProductGallery images={galleryImages} alt={product.name} />

        <div>
          {product.categories && (
            <p className="text-xs tracking-[0.2em] text-brass uppercase">
              {product.categories.name}
            </p>
          )}
          <h1 className="mt-2 font-display text-4xl text-paper">{product.name}</h1>
          <p className="mt-4 text-2xl text-paper">GEL {product.sell_price.toFixed(2)}</p>

          {product.description && (
            <p className="mt-6 leading-relaxed text-paper-dim">{product.description}</p>
          )}

          <AddToCartButton
            productId={product.id}
            name={product.name}
            unitPrice={product.sell_price}
            imagePath={featured?.storage_path ?? null}
          />

          <div className="mt-4 border border-line px-6 py-4 text-sm text-paper-dim">
            Buy online and pick up in store, or visit Black Eye Beauty, Shelby,
            or End directly.
          </div>
        </div>
      </div>
    </div>
  );
}
