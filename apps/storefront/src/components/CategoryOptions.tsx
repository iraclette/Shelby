import { categoryName } from "@/i18n/format";
import type { Dictionary } from "@/i18n/dictionaries/en";
import { subcategoriesOf, topLevelCategories } from "@/lib/categoryGroups";

export type StaffCategory = { id: string; name: string; parent_id: string | null };

// The <option>s of a staff category picker, starting with "No category" (value "").
export default function CategoryOptions({ categories, dict }: { categories: StaffCategory[]; dict: Dictionary }) {
  return (
    <>
      <option value="">{dict.staff.noCategory}</option>
      {/* A category with subcategories is a group: products go in one of its subcategories. */}
      {topLevelCategories(categories).map((category) => {
        const subcategories = subcategoriesOf(categories, category.id);
        return subcategories.length > 0 ? (
          <optgroup key={category.id} label={categoryName(dict, category.name)}>
            {subcategories.map((sub) => (
              <option key={sub.id} value={sub.id}>
                {categoryName(dict, sub.name)}
              </option>
            ))}
          </optgroup>
        ) : (
          <option key={category.id} value={category.id}>
            {categoryName(dict, category.name)}
          </option>
        );
      })}
    </>
  );
}
