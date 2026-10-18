import Link from "next/link";
import Image from "next/image";
import { primaryImage, productImageUrl, type Product } from "@/lib/supabase";
import { getDictionary } from "@/i18n/server";
import { categoryName, formatPrice } from "@/i18n/format";

export default async function ProductCard({ product }: { product: Product }) {
  const dict = await getDictionary();
  const image = primaryImage(product);

  return (
    <Link
      href={`/products/${product.id}`}
      className="group block overflow-hidden rounded-sm border border-line bg-ink-soft transition-colors hover:border-brass"
    >
      <div className="relative aspect-square overflow-hidden bg-ink-elevated">
        {image ? (
          <Image
            src={productImageUrl(image.storage_path)}
            alt={product.name}
            fill
            sizes="(min-width: 768px) 25vw, 50vw"
            className={`object-cover transition-transform duration-300 motion-safe:group-hover:scale-105 ${product.inStock ? "" : "opacity-50"}`}
          />
        ) : (
          <div className="flex h-full items-center justify-center text-paper-dim/80">
            <span className="font-display text-3xl italic">Shelby</span>
          </div>
        )}
        {!product.inStock && (
          <span className="absolute top-2 left-2 bg-ink/85 px-2 py-1 text-xs tracking-[0.15em] text-paper uppercase">
            {dict.product.outOfStock}
          </span>
        )}
      </div>
      <div className="p-4">
        {product.categories && (
          <p className="text-xs tracking-[0.15em] text-brass uppercase">
            {categoryName(dict, product.categories.name)}
          </p>
        )}
        <h3 className="mt-1 font-display text-lg text-paper">{product.name}</h3>
        <p className="mt-1 text-sm text-paper-dim tabular-nums">{formatPrice(dict, product.sell_price)}</p>
      </div>
    </Link>
  );
}
