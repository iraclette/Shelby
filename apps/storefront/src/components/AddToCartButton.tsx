"use client";

import { useState } from "react";
import { useCart } from "@/lib/cart";
import { useI18n } from "@/i18n/client";
import { useVariantSelection } from "@/components/VariantSelection";

export default function AddToCartButton({
  productId,
  name,
  unitPrice,
  imagePath,
  variantImagePaths = {},
}: {
  productId: string;
  name: string;
  unitPrice: number;
  imagePath: string | null;
  variantImagePaths?: Record<string, string>; // variant id -> its photo, shown in the cart
}) {
  const { addItem } = useCart();
  const { dict } = useI18n();
  const [added, setAdded] = useState(false);
  const selection = useVariantSelection();
  const variant = selection?.selected ?? null;

  return (
    <button
      disabled={selection !== null && variant === null}
      onClick={() => {
        addItem({
          productId,
          variantId: variant?.id ?? null,
          variantName: variant?.name ?? null,
          name,
          unitPrice,
          imagePath: (variant && variantImagePaths[variant.id]) || imagePath,
        });
        setAdded(true);
        setTimeout(() => setAdded(false), 1500);
      }}
      className="mt-6 w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink disabled:opacity-50"
    >
      {added ? dict.product.added : dict.product.addToCart}
    </button>
  );
}
