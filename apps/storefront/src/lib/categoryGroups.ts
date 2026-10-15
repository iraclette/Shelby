// How the home page and the /products filter arrange categories. Matched by lowercased
// category name; a category that isn't listed (added later in the Shop Console) lands in
// "other" at the end, so it still shows up.

export type CategoryGroupKey = "beauty" | "brands" | "blades" | "forHim" | "gifts" | "other";

const GROUPS: { key: Exclude<CategoryGroupKey, "other">; names: string[] }[] = [
  {
    key: "beauty",
    names: [
      "decorative cosmetics",
      "skincare & body care",
      "hair care",
      "hair clips & ties",
      "nail care",
      "perfume",
      "original oil perfumes",
    ],
  },
  // Shown as name-only tiles rather than icons.
  { key: "brands", names: ["golden rose", "pastel", "staleks"] },
  {
    key: "blades",
    names: ["knives", "hunting knives & axes", "rosaries, knuckles & batons", "flashlights", "binoculars"],
  },
  { key: "forHim", names: ["lighters", "hookah", "flasks & flask sets", "hair & beard care", "car fragrances"] },
  { key: "gifts", names: ["jewelry", "watches", "gift boxes", "gift packages", "board games", "souvenirs"] },
];

const position = new Map<string, { group: number; index: number }>();
GROUPS.forEach((group, g) => group.names.forEach((name, i) => position.set(name, { group: g, index: i })));

function placeOf(name: string) {
  return position.get(name.trim().toLowerCase()) ?? { group: GROUPS.length, index: 0 };
}

export function groupCategories<T extends { name: string }>(categories: T[]) {
  const groups = [...GROUPS.map((g) => ({ key: g.key as CategoryGroupKey, categories: [] as T[] })), {
    key: "other" as CategoryGroupKey,
    categories: [] as T[],
  }];
  for (const category of sortCategories(categories)) groups[placeOf(category.name).group].categories.push(category);
  return groups.filter((group) => group.categories.length > 0);
}

// Group order, then the order listed above; unknown categories keep their database order at the end.
export function sortCategories<T extends { name: string }>(categories: T[]) {
  return [...categories].sort((a, b) => {
    const pa = placeOf(a.name);
    const pb = placeOf(b.name);
    return pa.group - pb.group || pa.index - pb.index;
  });
}
