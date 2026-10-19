// How the home page and the /products filter arrange categories. Matched by lowercased
// category name; a category that isn't listed (added later in the Shop Console) lands in
// "other" at the end, so it still shows up. A category with subcategories (Perfumes) gets
// a section of its own instead, ahead of these.

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

// Subcategories in the order the shops list them; ones not here follow, alphabetically.
const SUBCATEGORIES = [
  "mini perfumes 8-12 gel",
  "special offer 25 gel",
  "dubai perfumes",
  "original tester quality",
  "high quality replicas",
  "niche perfumes",
  "originals & replicas",
];

const position = new Map<string, { group: number; index: number }>();
GROUPS.forEach((group, g) => group.names.forEach((name, i) => position.set(name, { group: g, index: i })));
SUBCATEGORIES.forEach((name, i) => position.set(name, { group: -1, index: i }));

function placeOf(name: string) {
  return position.get(name.trim().toLowerCase()) ?? { group: GROUPS.length, index: 0 };
}

type Categorized = { id: string; name: string; parent_id?: string | null };

export function subcategoriesOf<T extends Categorized>(categories: T[], parentId: string) {
  return sortCategories(categories.filter((c) => c.parent_id === parentId));
}

export function topLevelCategories<T extends Categorized>(categories: T[]) {
  return sortCategories(categories.filter((c) => !c.parent_id));
}

// Each parent with subcategories is its own section (`parent` set, tiles are its
// subcategories); the remaining top-level categories fill the named groups.
export function groupCategories<T extends Categorized>(categories: T[]) {
  const parents = topLevelCategories(categories).filter((c) => subcategoriesOf(categories, c.id).length > 0);
  const parentSections = parents.map((parent) => ({
    key: parent.id,
    parent: parent as T | undefined,
    categories: subcategoriesOf(categories, parent.id),
  }));

  const groups = [...GROUPS.map((g) => ({ key: g.key as CategoryGroupKey, categories: [] as T[] })), {
    key: "other" as CategoryGroupKey,
    categories: [] as T[],
  }];
  for (const category of topLevelCategories(categories)) {
    if (parents.includes(category)) continue;
    groups[placeOf(category.name).group].categories.push(category);
  }
  return [
    ...parentSections,
    ...groups.filter((group) => group.categories.length > 0).map((group) => ({ ...group, parent: undefined })),
  ];
}

// Group order, then the order listed above; unknown categories keep their database order
// (alphabetical) at the end.
export function sortCategories<T extends { name: string }>(categories: T[]) {
  return [...categories].sort((a, b) => {
    const pa = placeOf(a.name);
    const pb = placeOf(b.name);
    return pa.group - pb.group || pa.index - pb.index;
  });
}
