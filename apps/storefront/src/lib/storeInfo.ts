// Copy about the three shops that isn't in the database yet. Keyed by shop name as it
// appears in the shops table; a shop that isn't listed here still shows, just without a blurb.

export const TOWN = "Gori";

export const SHOP_BLURBS: Record<string, string> = {
  "Black Eye Beauty": "Cosmetics, skincare, nail and hair care, and perfume.",
  Shelby: "Leather goods, hunting knives, decorative katanas, hookahs, oil perfumes, gift boxes and souvenirs.",
  End: "A little of both: beauty from Black Eye and the pick of Shelby, under one roof.",
};

export const PROMISES = [
  { title: "Pick up in store", text: "Order online, collect from any of our three shops." },
  { title: "Secure payment", text: "Card payments handled by Bank of Georgia." },
  { title: "Ready to gift", text: "Boxed sets with chess, flasks and shot cups." },
  { title: "Real people", text: `Three shops in ${TOWN}, run by one family.` },
];
