"use client";

import {
  createContext,
  useContext,
  useSyncExternalStore,
  type ReactNode,
} from "react";

export type CartLine = {
  productId: string;
  variantId?: string | null; // absent in carts saved before variants could be bought online
  variantName?: string | null;
  name: string;
  unitPrice: number;
  quantity: number;
  imagePath: string | null;
};

// One line per product, or per variant for products that have them.
export function lineKey(line: Pick<CartLine, "productId" | "variantId">) {
  return line.variantId ? `${line.productId}:${line.variantId}` : line.productId;
}

type CartContextValue = {
  lines: CartLine[];
  addItem: (item: Omit<CartLine, "quantity">, quantity?: number) => void;
  removeItem: (key: string) => void;
  setQuantity: (key: string, quantity: number) => void;
  clear: () => void;
  itemCount: number;
  subtotal: number;
  // False until the saved cart has been read from localStorage.
  hydrated: boolean;
};

const CartContext = createContext<CartContextValue | null>(null);
const STORAGE_KEY = "shelby-cart";

// The saved cart lives in localStorage, read through useSyncExternalStore: the server
// render (and the first client render) see an empty cart, then the saved one.
const EMPTY: CartLine[] = [];
let saved: CartLine[] | null = null; // parsed once, so every read returns the same array
const listeners = new Set<() => void>();

// Wrapped in try/catch since localStorage can throw (private browsing, disabled storage, etc.).
function readCart(): CartLine[] {
  if (saved) return saved;
  try {
    const raw = window.localStorage.getItem(STORAGE_KEY);
    saved = raw ? JSON.parse(raw) : EMPTY;
  } catch {
    saved = EMPTY; // just start with an empty cart
  }
  return saved!;
}

function writeCart(update: (lines: CartLine[]) => CartLine[]) {
  saved = update(readCart());
  try {
    window.localStorage.setItem(STORAGE_KEY, JSON.stringify(saved));
  } catch {
    // ignore
  }
  listeners.forEach((listener) => listener());
}

// Also follows changes made in another tab.
function subscribe(listener: () => void) {
  listeners.add(listener);
  const onStorage = (event: StorageEvent) => {
    if (event.key !== STORAGE_KEY) return;
    saved = null;
    listener();
  };
  window.addEventListener("storage", onStorage);
  return () => {
    listeners.delete(listener);
    window.removeEventListener("storage", onStorage);
  };
}

const subscribeNothing = () => () => {};

export function CartProvider({ children }: { children: ReactNode }) {
  const lines = useSyncExternalStore(subscribe, readCart, () => EMPTY);
  // False until the saved cart has been read (i.e. during the server render and hydration).
  const hydrated = useSyncExternalStore(subscribeNothing, () => true, () => false);
  const setLines = writeCart;

  function addItem(item: Omit<CartLine, "quantity">, quantity = 1) {
    const key = lineKey(item);
    setLines((prev) => {
      const existing = prev.find((line) => lineKey(line) === key);
      if (existing) {
        return prev.map((line) =>
          lineKey(line) === key
            ? { ...line, quantity: line.quantity + quantity }
            : line,
        );
      }
      return [...prev, { ...item, quantity }];
    });
  }

  function removeItem(key: string) {
    setLines((prev) => prev.filter((line) => lineKey(line) !== key));
  }

  function setQuantity(key: string, quantity: number) {
    if (quantity <= 0) {
      removeItem(key);
      return;
    }
    setLines((prev) =>
      prev.map((line) => (lineKey(line) === key ? { ...line, quantity } : line)),
    );
  }

  function clear() {
    setLines(() => EMPTY);
  }

  const itemCount = lines.reduce((sum, line) => sum + line.quantity, 0);
  const subtotal = lines.reduce((sum, line) => sum + line.quantity * line.unitPrice, 0);

  return (
    <CartContext.Provider
      value={{ lines, addItem, removeItem, setQuantity, clear, itemCount, subtotal, hydrated }}
    >
      {children}
    </CartContext.Provider>
  );
}

export function useCart() {
  const ctx = useContext(CartContext);
  if (!ctx) throw new Error("useCart must be used within a CartProvider");
  return ctx;
}
