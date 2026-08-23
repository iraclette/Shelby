import Image from "next/image";
import { notFound } from "next/navigation";
import { fetchProduct, primaryImage, productImageUrl } from "@/lib/supabase";

export const revalidate = 60;

export default async function ProductPage({
  params,
}: {
  params: Promise<{ id: string }>;
}) {
  const { id } = await params;
  const product = await fetchProduct(id);
  if (!product) notFound();

  const images = product.product_images.length > 0 ? product.product_images : null;
  const featured = primaryImage(product);

  return (
    <div className="mx-auto max-w-6xl px-6 py-16">
      <div className="grid gap-12 md:grid-cols-2">
        <div>
          <div className="relative aspect-square overflow-hidden border border-line bg-ink-elevated">
            {featured ? (
              <Image
                src={productImageUrl(featured.storage_path)}
                alt={product.name}
                fill
                sizes="(min-width: 768px) 50vw, 100vw"
                className="object-cover"
                priority
              />
            ) : (
              <div className="flex h-full items-center justify-center text-paper-dim/50">
                <span className="font-display text-4xl italic">Shelby</span>
              </div>
            )}
          </div>
          {images && images.length > 1 && (
            <div className="mt-4 grid grid-cols-5 gap-3">
              {images.map((image) => (
                <div
                  key={image.storage_path}
                  className="relative aspect-square overflow-hidden border border-line bg-ink-elevated"
                >
                  <Image
                    src={productImageUrl(image.storage_path)}
                    alt={product.name}
                    fill
                    sizes="20vw"
                    className="object-cover"
                  />
                </div>
              ))}
            </div>
          )}
        </div>

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

          <div className="mt-10 border border-line px-6 py-4 text-sm text-paper-dim">
            Available in store — visit Black Eye Beauty, Shelby, or End to purchase.
          </div>
        </div>
      </div>
    </div>
  );
}
