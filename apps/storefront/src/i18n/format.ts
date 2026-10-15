import type { Dictionary } from "./dictionaries/en";

// fmt("Order #{id}", { id: 5 }) -> "Order #5"
export function fmt(template: string, vars: Record<string, string | number>) {
  return template.replace(/\{(\w+)\}/g, (match, key: string) => (key in vars ? String(vars[key]) : match));
}

export function formatPrice(dict: Dictionary, amount: number) {
  return fmt(dict.common.price, { amount: amount.toFixed(2) });
}

// Category names live in the database in English; a category that isn't in the
// dictionary (added later in the Shop Console) shows its stored name.
export function categoryName(dict: Dictionary, name: string) {
  return dict.categories[name.trim().toLowerCase()] ?? name;
}
