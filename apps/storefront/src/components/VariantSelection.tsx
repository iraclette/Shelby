"use client";

import { createContext, useContext, useState, type ReactNode } from "react";
import type { ProductVariant } from "@/lib/supabase";
import { useI18n } from "@/i18n/client";

type VariantSelection = {
  variants: ProductVariant[];
  selected: ProductVariant | null;
  select: (variant: ProductVariant) => void;
};

const VariantContext = createContext<VariantSelection | null>(null);

// The shade/size a visitor picked on a product page. The gallery shows its photo and
// the add-to-cart button buys it. Starts on the first one that's in stock.
export function VariantProvider({ variants, children }: { variants: ProductVariant[]; children: ReactNode }) {
  const [selected, setSelected] = useState(() => variants.find((v) => v.inStock) ?? variants[0] ?? null);
  return (
    <VariantContext.Provider value={{ variants, selected, select: setSelected }}>{children}</VariantContext.Provider>
  );
}

// Null on products without variants.
export function useVariantSelection() {
  const ctx = useContext(VariantContext);
  return ctx && ctx.variants.length > 0 ? ctx : null;
}

export function VariantPicker() {
  const selection = useVariantSelection();
  const { dict } = useI18n();
  if (!selection) return null;

  return (
    <fieldset className="mt-6">
      <legend className="text-sm text-paper-dim">{dict.product.chooseVariant}</legend>
      <div className="mt-2 flex flex-wrap gap-2">
        {selection.variants.map((variant) => {
          const active = selection.selected?.id === variant.id;
          return (
            // Sold-out ones can still be picked, to ask to be told when they're back.
            <button
              key={variant.id}
              type="button"
              onClick={() => selection.select(variant)}
              aria-pressed={active}
              title={variant.inStock ? undefined : dict.product.outOfStock}
              className={`min-h-11 min-w-11 border px-4 text-sm transition-colors ${
                active ? "border-brass text-brass" : "border-line text-paper hover:border-paper-dim"
              } ${variant.inStock ? "" : "line-through opacity-60"}`}
            >
              {variant.name}
            </button>
          );
        })}
      </div>
    </fieldset>
  );
}
