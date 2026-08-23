"use client";

import {
  createContext,
  useContext,
  useEffect,
  useState,
  type ReactNode,
} from "react";

export type CartLine = {
  productId: string;
  name: string;
  unitPrice: number;
  quantity: number;
  imagePath: string | null;
};

type CartContextValue = {
  lines: CartLine[];
  addItem: (item: Omit<CartLine, "quantity">, quantity?: number) => void;
  removeItem: (productId: string) => void;
  setQuantity: (productId: string, quantity: number) => void;
  clear: () => void;
  itemCount: number;
  subtotal: number;
};

const CartContext = createContext<CartContextValue | null>(null);
const STORAGE_KEY = "shelby-cart";

export function CartProvider({ children }: { children: ReactNode }) {
  const [lines, setLines] = useState<CartLine[]>([]);
  const [hydrated, setHydrated] = useState(false);

  // Read the saved cart once on mount. Wrapped in try/catch since
  // localStorage can throw (private browsing, disabled storage, etc.).
  useEffect(() => {
    try {
      const raw = window.localStorage.getItem(STORAGE_KEY);
      if (raw) setLines(JSON.parse(raw));
    } catch {
      // ignore — just start with an empty cart
    }
    setHydrated(true);
  }, []);

  useEffect(() => {
    if (!hydrated) return; // don't overwrite saved data with the initial empty state
    try {
      window.localStorage.setItem(STORAGE_KEY, JSON.stringify(lines));
    } catch {
      // ignore
    }
  }, [lines, hydrated]);

  function addItem(item: Omit<CartLine, "quantity">, quantity = 1) {
    setLines((prev) => {
      const existing = prev.find((line) => line.productId === item.productId);
      if (existing) {
        return prev.map((line) =>
          line.productId === item.productId
            ? { ...line, quantity: line.quantity + quantity }
            : line,
        );
      }
      return [...prev, { ...item, quantity }];
    });
  }

  function removeItem(productId: string) {
    setLines((prev) => prev.filter((line) => line.productId !== productId));
  }

  function setQuantity(productId: string, quantity: number) {
    if (quantity <= 0) {
      removeItem(productId);
      return;
    }
    setLines((prev) =>
      prev.map((line) => (line.productId === productId ? { ...line, quantity } : line)),
    );
  }

  function clear() {
    setLines([]);
  }

  const itemCount = lines.reduce((sum, line) => sum + line.quantity, 0);
  const subtotal = lines.reduce((sum, line) => sum + line.quantity * line.unitPrice, 0);

  return (
    <CartContext.Provider
      value={{ lines, addItem, removeItem, setQuantity, clear, itemCount, subtotal }}
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
