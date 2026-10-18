import Link from "next/link";
import { notFound } from "next/navigation";
import { Store, Truck } from "lucide-react";
import { fetchProduct, primaryImage } from "@/lib/supabase";
import AddToCartButton from "@/components/AddToCartButton";
import ProductGallery from "@/components/ProductGallery";
import { getDictionary } from "@/i18n/server";
import { categoryName, fmt, formatPrice } from "@/i18n/format";
import { DELIVERY } from "@/lib/business";

export const revalidate = 60;

export default async function ProductPage({
  params,
}: PageProps<"/[lang]/products/[id]">) {
  const { id } = await params;
  const [product, dict] = await Promise.all([fetchProduct(id), getDictionary()]);
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
              {categoryName(dict, product.categories.name)}
            </p>
          )}
          <h1 className="mt-2 font-display text-4xl text-paper">{product.name}</h1>
          <p className="mt-4 text-2xl text-paper tabular-nums">{formatPrice(dict, product.sell_price)}</p>

          {product.description && (
            <p className="mt-6 max-w-prose whitespace-pre-line leading-relaxed text-paper-dim">{product.description}</p>
          )}

          <AddToCartButton
            productId={product.id}
            name={product.name}
            unitPrice={product.sell_price}
            imagePath={featured?.storage_path ?? null}
          />

          <ul className="mt-6 space-y-3 border-t border-line pt-6 text-sm text-paper-dim">
            <li className="flex gap-3">
              <Store className="mt-0.5 h-4 w-4 flex-shrink-0 text-brass" strokeWidth={1.5} aria-hidden="true" />
              {dict.product.pickupLine}
            </li>
            <li className="flex gap-3">
              <Truck className="mt-0.5 h-4 w-4 flex-shrink-0 text-brass" strokeWidth={1.5} aria-hidden="true" />
              <span>
                {fmt(dict.product.deliveryLine, {
                  branch: formatPrice(dict, DELIVERY.branchPrice),
                  home: formatPrice(dict, DELIVERY.homePrice),
                })}
                .{" "}
                <Link href="/delivery" className="text-brass underline-offset-4 hover:underline">
                  {dict.product.deliveryLink}
                </Link>
              </span>
            </li>
          </ul>
        </div>
      </div>
    </div>
  );
}
