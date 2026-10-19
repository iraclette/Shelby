"use client";

import AddToCartButton from "@/components/AddToCartButton";
import StockRequestForm from "@/components/StockRequestForm";
import { VariantPicker, useVariantSelection } from "@/components/VariantSelection";

// The buy area of a product with variants: the picker, then "Add to cart" for a variant
// in stock or "let me know when it's back" for one that's sold out.
export default function VariantPurchase({
  productId,
  ...cartProps
}: Omit<React.ComponentProps<typeof AddToCartButton>, "productId"> & { productId: string }) {
  const selected = useVariantSelection()?.selected;
  return (
    <>
      <VariantPicker />
      {!selected || selected.inStock ? (
        <AddToCartButton productId={productId} {...cartProps} />
      ) : (
        <StockRequestForm key={selected.id} productId={productId} variantId={selected.id} />
      )}
    </>
  );
}
