"use client";

import { useState } from "react";
import { useCart } from "@/lib/cart";

export default function AddToCartButton({
  productId,
  name,
  unitPrice,
  imagePath,
}: {
  productId: string;
  name: string;
  unitPrice: number;
  imagePath: string | null;
}) {
  const { addItem } = useCart();
  const [added, setAdded] = useState(false);

  return (
    <button
      onClick={() => {
        addItem({ productId, name, unitPrice, imagePath });
        setAdded(true);
        setTimeout(() => setAdded(false), 1500);
      }}
      className="mt-6 w-full border border-brass py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink"
    >
      {added ? "Added to cart" : "Add to cart"}
    </button>
  );
}
