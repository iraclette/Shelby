// Copy about the three shops that isn't in the database yet. Keyed by shop name as it
// appears in the shops table; a shop that isn't listed here still shows, just without a blurb.

export const TOWN = "Gori";

export const SHOP_BLURBS: Record<string, string> = {
  "Black Eye Beauty":
    "Your everyday ritual, start to finish: makeup and skincare, hair and nail care, and a fragrance to complete the look.",
  Shelby:
    "Blades for collectors and the outdoors, from hunting knives and karambits to display katanas, alongside fine leather, deep oil-based perfumes and gift boxes made to impress. Hookahs, lighters and souvenirs round out the shelves.",
  End:
    "The best of both: beauty essentials next to Shelby's blades and gifts, for when you're shopping for yourself and someone else in one stop.",
};

export const PROMISES = [
  { title: "Pick up or delivery", text: "Collect from any of our shops, or have it delivered at your cost." },
  { title: "Secure payment", text: "Card payments handled by Bank of Georgia." },
  { title: "Ready to gift", text: "Boxed sets with chess, flasks and shot cups." },
  { title: `Three shops in ${TOWN}`, text: "Drop in to see, smell and hold it before you buy." },
];
