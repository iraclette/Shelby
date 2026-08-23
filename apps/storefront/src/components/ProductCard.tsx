import Link from "next/link";
import Image from "next/image";
import { primaryImage, productImageUrl, type Product } from "@/lib/supabase";

export default function ProductCard({ product }: { product: Product }) {
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
            className="object-cover transition-transform duration-300 group-hover:scale-105"
          />
        ) : (
          <div className="flex h-full items-center justify-center text-paper-dim/50">
            <span className="font-display text-3xl italic">Shelby</span>
          </div>
        )}
      </div>
      <div className="p-4">
        {product.categories && (
          <p className="text-xs tracking-[0.15em] text-brass uppercase">
            {product.categories.name}
          </p>
        )}
        <h3 className="mt-1 font-display text-lg text-paper">{product.name}</h3>
        <p className="mt-1 text-sm text-paper-dim">GEL {product.sell_price.toFixed(2)}</p>
      </div>
    </Link>
  );
}
