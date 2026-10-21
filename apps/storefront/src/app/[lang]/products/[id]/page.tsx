import { cache } from "react";
import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Store, Truck } from "lucide-react";
import { fetchProduct, primaryImage, productImageUrl } from "@/lib/supabase";
import { languageAlternates, OG_LOCALES, snippet, SITE_URL } from "@/lib/seo";
import AddToCartButton from "@/components/AddToCartButton";
import StockRequestForm from "@/components/StockRequestForm";
import ProductGallery from "@/components/ProductGallery";
import { VariantProvider } from "@/components/VariantSelection";
import VariantPurchase from "@/components/VariantPurchase";
import { getDictionary, getLocale } from "@/i18n/server";
import { descriptionFor } from "@/lib/descriptions";
import { categoryName, fmt, formatPrice } from "@/i18n/format";
import { DELIVERY } from "@/lib/business";

export const revalidate = 60;

// generateMetadata and the page both need it; one query per request.
const getProduct = cache(fetchProduct);

export async function generateMetadata({ params }: PageProps<"/[lang]/products/[id]">): Promise<Metadata> {
  const { id } = await params;
  const [product, locale] = await Promise.all([getProduct(id), getLocale()]);
  if (!product) return {};
  const image = primaryImage(product);
  const description = snippet(descriptionFor(product, locale));
  return {
    title: `${product.name} — Shelby`,
    description,
    alternates: languageAlternates(`/products/${id}`, locale),
    // Replaces the layout's openGraph rather than merging with it, so repeat those fields.
    openGraph: {
      siteName: "Shelby",
      type: "website",
      locale: OG_LOCALES[locale],
      title: product.name,
      description,
      images: image ? [productImageUrl(image.storage_path)] : undefined,
    },
  };
}

export default async function ProductPage({
  params,
}: PageProps<"/[lang]/products/[id]">) {
  const { id } = await params;
  const [product, dict, locale] = await Promise.all([getProduct(id), getDictionary(), getLocale()]);
  if (!product) notFound();
  const description = descriptionFor(product, locale);

  const featured = primaryImage(product);
  const galleryImages = featured
    ? [featured, ...product.product_images.filter((image) => image !== featured)]
    : [];
  // First photo of each variant, so the cart shows the shade that was picked.
  const variantImagePaths: Record<string, string> = {};
  for (const image of galleryImages) {
    if (image.variant_id && !variantImagePaths[image.variant_id]) variantImagePaths[image.variant_id] = image.thumb_path ?? image.storage_path;
  }

  // Lets search engines show the price, stock and photo with the result.
  const structuredData = {
    "@context": "https://schema.org",
    "@type": "Product",
    name: product.name,
    description: description ?? undefined,
    image: galleryImages.map((image) => productImageUrl(image.storage_path)),
    offers: {
      "@type": "Offer",
      price: product.sell_price.toFixed(2),
      priceCurrency: "GEL",
      availability: product.inStock ? "https://schema.org/InStock" : "https://schema.org/OutOfStock",
      url: `${SITE_URL}/${locale}/products/${id}`,
    },
  };

  return (
    <VariantProvider variants={product.product_variants}>
    <div className="mx-auto max-w-6xl px-6 py-16">
      <script
        type="application/ld+json"
        // "<" escaped so a description can never close the script tag.
        dangerouslySetInnerHTML={{ __html: JSON.stringify(structuredData).replace(/</g, "\\u003c") }}
      />
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

          {description && (
            <p className="mt-6 max-w-prose whitespace-pre-line leading-relaxed text-paper-dim">{description}</p>
          )}

          {product.product_variants.length > 0 ? (
            <VariantPurchase
              productId={product.id}
              name={product.name}
              unitPrice={product.sell_price}
              imagePath={featured ? (featured.thumb_path ?? featured.storage_path) : null}
              variantImagePaths={variantImagePaths}
            />
          ) : product.inStock ? (
            <AddToCartButton
              productId={product.id}
              name={product.name}
              unitPrice={product.sell_price}
              imagePath={featured ? (featured.thumb_path ?? featured.storage_path) : null}
            />
          ) : (
            <StockRequestForm productId={product.id} />
          )}

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
    </VariantProvider>
  );
}
