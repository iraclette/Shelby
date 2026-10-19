"use client";

import { useEffect, useRef, useState } from "react";
import Link from "next/link";
import type { Session } from "@supabase/supabase-js";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";
import Skeleton from "@/components/Skeleton";
import { randomId } from "@/lib/randomId";
import { useI18n } from "@/i18n/client";
import { categoryName, fmt } from "@/i18n/format";
import { subcategoriesOf, topLevelCategories } from "@/lib/categoryGroups";

type Shop = { id: string; name: string };
type Category = { id: string; name: string; parent_id: string | null };
type PickedPhoto = { id: string; file: File; previewUrl: string };

const inputClass =
  "w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass";
const labelClass = "block text-sm text-paper-dim";

// Same format as the Shop Console's generateInternalSku() (ProductDialog.cpp).
function generateInternalSku() {
  const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // no O/0/I/1 to avoid label confusion
  let code = "";
  for (let i = 0; i < 6; i++) code += alphabet[Math.floor(Math.random() * alphabet.length)];
  return `INT-${code}`;
}

// Phones in Georgian locale type "12,50".
function parsePrice(text: string): number | null {
  const value = Number(text.trim().replace(",", "."));
  return text.trim() !== "" && Number.isFinite(value) && value >= 0 ? value : null;
}

export default function NewProductClient() {
  return (
    <StaffAuthGate>{({ session, signOut }) => <NewProductForm session={session} onSignOut={signOut} />}</StaffAuthGate>
  );
}

function NewProductForm({ session, onSignOut }: { session: Session; onSignOut: () => void }) {
  const supabase = getSupabaseClient();
  const { dict } = useI18n();

  const [shops, setShops] = useState<Shop[]>([]);
  const [categories, setCategories] = useState<Category[]>([]);
  const [listsLoaded, setListsLoaded] = useState(false);

  const [name, setName] = useState("");
  const [categoryId, setCategoryId] = useState("");
  const [sellPrice, setSellPrice] = useState("");
  const [costPrice, setCostPrice] = useState("");
  const [description, setDescription] = useState("");
  const [stock, setStock] = useState<Record<string, string>>({});
  const [photos, setPhotos] = useState<PickedPhoto[]>([]);

  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");
  const [created, setCreated] = useState<{ id: string; name: string; warning: string } | null>(null);

  const cameraInputRef = useRef<HTMLInputElement>(null);
  const galleryInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    Promise.all([
      supabase.from("shops").select("id, name").order("name"),
      supabase.from("categories").select("id, name, parent_id").order("name"),
    ]).then(([shopsRes, categoriesRes]) => {
      setShops((shopsRes.data as Shop[]) ?? []);
      setCategories((categoriesRes.data as Category[]) ?? []);
      setListsLoaded(true);
    });
  }, [supabase]);

  function addPhotos(fileList: FileList | null) {
    if (!fileList) return;
    const picked = Array.from(fileList).map((file) => ({
      id: randomId(),
      file,
      previewUrl: URL.createObjectURL(file),
    }));
    setPhotos((current) => [...current, ...picked]);
    if (cameraInputRef.current) cameraInputRef.current.value = "";
    if (galleryInputRef.current) galleryInputRef.current.value = "";
  }

  function removePhoto(photo: PickedPhoto) {
    URL.revokeObjectURL(photo.previewUrl);
    setPhotos((current) => current.filter((p) => p.id !== photo.id));
  }

  function resetForm() {
    photos.forEach((photo) => URL.revokeObjectURL(photo.previewUrl));
    setName("");
    setSellPrice("");
    setCostPrice("");
    setDescription("");
    setStock({});
    setPhotos([]);
    setError("");
    setCreated(null);
  }

  async function insertProduct(body: Record<string, unknown>) {
    // Retry on the rare random-SKU collision (unique_violation).
    for (let attempt = 0; attempt < 3; attempt++) {
      const result = await supabase
        .from("products")
        .insert({ ...body, sku: generateInternalSku() })
        .select("id")
        .single();
      if (result.error?.code !== "23505") return result;
    }
    return { data: null, error: { message: dict.staff.skuCollision } };
  }

  async function handleSubmit(event: React.FormEvent) {
    event.preventDefault();
    setError("");

    const sell = parsePrice(sellPrice);
    const cost = costPrice.trim() === "" ? 0 : parsePrice(costPrice);
    if (!name.trim()) return setError(dict.staff.enterName);
    if (sell === null) return setError(dict.staff.enterPrice);
    if (cost === null) return setError(dict.staff.costNumber);

    setSaving(true);

    const { data: product, error: productError } = await insertProduct({
      name: name.trim(),
      description: description.trim() || null,
      category_id: categoryId || null,
      sell_price: sell,
      cost_price: cost,
    });

    if (productError || !product) {
      setSaving(false);
      setError(productError?.message ?? dict.staff.saveFailed);
      return;
    }

    const problems: string[] = [];

    const levels = shops
      .map((shop) => ({ product_id: product.id, shop_id: shop.id, quantity: parseInt(stock[shop.id] ?? "", 10) }))
      .filter((level) => Number.isInteger(level.quantity) && level.quantity > 0);
    if (levels.length > 0) {
      const { error: stockError } = await supabase.from("inventory_levels").insert(levels);
      if (stockError) problems.push(dict.staff.stockFailed);
    }

    let failedPhotos = 0;
    for (const [i, photo] of photos.entries()) {
      const extension = photo.file.name.includes(".") ? photo.file.name.split(".").pop() : "jpg";
      const storagePath = `${product.id}/${randomId()}.${extension}`;
      const { error: uploadError } = await supabase.storage
        .from("product-images")
        .upload(storagePath, photo.file, { contentType: photo.file.type || "image/jpeg" });
      const { error: rowError } = uploadError
        ? { error: uploadError }
        : await supabase
            .from("product_images")
            .insert({ product_id: product.id, storage_path: storagePath, is_primary: i === 0 });
      if (rowError) failedPhotos += 1;
    }
    if (failedPhotos > 0) problems.push(fmt(dict.staff.photosFailed, { n: failedPhotos }));

    // Fire and forget, same as the Shop Console's logAuditEvent().
    supabase
      .from("audit_log")
      .insert({
        actor_id: session.user.id,
        action: "product_created",
        entity_type: "product",
        entity_id: product.id,
        detail: `created from phone: ${name.trim()} @ GEL ${sell.toFixed(2)}`,
      })
      .then(() => {});

    setSaving(false);
    setCreated({
      id: product.id,
      name: name.trim(),
      warning: problems.length > 0 ? fmt(dict.staff.savedBut, { problems: problems.join(dict.staff.and) }) : "",
    });
  }

  if (created) {
    return (
      <div className="mx-auto min-h-dvh max-w-lg px-4 py-10 text-center">
        <p className="text-xs tracking-[0.2em] text-brass uppercase">{dict.staff.added}</p>
        <h1 className="mt-2 font-display text-3xl text-paper">{created.name}</h1>
        {created.warning && <p className="mt-4 text-sm text-red-400">{created.warning}</p>}
        <p className="mt-4 text-sm text-paper-dim">{dict.staff.appearDelay}</p>
        <button
          onClick={resetForm}
          className="mt-8 flex min-h-14 w-full items-center justify-center rounded-md bg-brass text-base font-medium text-ink"
        >
          {dict.staff.addAnother}
        </button>
        <Link
          href={`/products/${created.id}`}
          className="mt-3 flex min-h-14 w-full items-center justify-center rounded-md border border-line text-base text-paper"
        >
          {dict.staff.viewOnSite}
        </Link>
      </div>
    );
  }

  return (
    <form onSubmit={handleSubmit} className="mx-auto min-h-dvh max-w-lg space-y-5 px-4 py-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper">{dict.nav.addProduct}</h1>
        <button type="button" onClick={onSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          {dict.common.signOut}
        </button>
      </div>

      <div>
        <p className={labelClass}>{dict.staff.photos}</p>
        <input
          ref={cameraInputRef}
          type="file"
          accept="image/*"
          capture="environment"
          className="hidden"
          onChange={(e) => addPhotos(e.target.files)}
        />
        <input
          ref={galleryInputRef}
          type="file"
          accept="image/*"
          multiple
          className="hidden"
          onChange={(e) => addPhotos(e.target.files)}
        />
        <div className="mt-2 grid grid-cols-2 gap-3">
          <button
            type="button"
            onClick={() => cameraInputRef.current?.click()}
            className="min-h-14 rounded-md border border-brass text-base text-brass active:bg-brass active:text-ink"
          >
            {dict.staff.takePhoto}
          </button>
          <button
            type="button"
            onClick={() => galleryInputRef.current?.click()}
            className="min-h-14 rounded-md border border-line text-base text-paper active:border-brass"
          >
            {dict.staff.fromGallery}
          </button>
        </div>
        {photos.length > 0 && (
          <div className="mt-3 grid grid-cols-3 gap-2">
            {photos.map((photo, i) => (
              <div key={photo.id} className="relative aspect-square overflow-hidden rounded-md border border-line">
                {/* eslint-disable-next-line @next/next/no-img-element -- local blob preview */}
                <img src={photo.previewUrl} alt="" className="h-full w-full object-cover" />
                {i === 0 && (
                  <span className="absolute top-1 left-1 rounded bg-ink/80 px-1.5 py-0.5 text-[11px] text-brass">
                    {dict.staff.main}
                  </span>
                )}
                <button
                  type="button"
                  onClick={() => removePhoto(photo)}
                  aria-label={dict.staff.removePhoto}
                  className="absolute top-0 right-0 flex h-11 w-11 items-start justify-end p-1"
                >
                  <span className="flex h-6 w-6 items-center justify-center rounded-full bg-ink/80 text-paper">×</span>
                </button>
              </div>
            ))}
          </div>
        )}
      </div>

      <label className="block">
        <span className={labelClass}>{dict.staff.name}</span>
        <input value={name} onChange={(e) => setName(e.target.value)} className={`mt-1 ${inputClass}`} />
      </label>

      <label className="block">
        <span className={labelClass}>{dict.staff.category}</span>
        <select value={categoryId} onChange={(e) => setCategoryId(e.target.value)} className={`mt-1 ${inputClass}`}>
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
        </select>
      </label>

      <div className="grid grid-cols-2 gap-3">
        <label className="block">
          <span className={labelClass}>{dict.staff.price}</span>
          <input
            value={sellPrice}
            onChange={(e) => setSellPrice(e.target.value)}
            inputMode="decimal"
            placeholder="0.00"
            className={`mt-1 ${inputClass}`}
          />
        </label>
        <label className="block">
          <span className={labelClass}>{dict.staff.cost}</span>
          <input
            value={costPrice}
            onChange={(e) => setCostPrice(e.target.value)}
            inputMode="decimal"
            placeholder={dict.staff.optional}
            className={`mt-1 ${inputClass}`}
          />
        </label>
      </div>

      <label className="block">
        <span className={labelClass}>{dict.staff.description}</span>
        <textarea
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          rows={3}
          className={`mt-1 ${inputClass}`}
        />
      </label>

      {!listsLoaded && (
        <div className="space-y-2" aria-busy="true">
          <Skeleton className="h-4 w-44" />
          {[0, 1, 2].map((i) => (
            <Skeleton key={i} className="h-12.5 w-full rounded-md" />
          ))}
        </div>
      )}

      {shops.length > 0 && (
        <fieldset>
          <legend className={labelClass}>{dict.staff.howMany}</legend>
          <div className="mt-1 space-y-2">
            {shops.map((shop) => (
              <label key={shop.id} className="flex items-center justify-between gap-3">
                <span className="text-paper">{shop.name}</span>
                <input
                  value={stock[shop.id] ?? ""}
                  onChange={(e) => setStock((s) => ({ ...s, [shop.id]: e.target.value.replace(/\D/g, "") }))}
                  inputMode="numeric"
                  placeholder="0"
                  className="w-24 rounded-md border border-line bg-ink-soft px-4 py-3 text-right text-base text-paper outline-none focus:border-brass"
                />
              </label>
            ))}
          </div>
        </fieldset>
      )}

      {error && <p className="text-sm text-red-400">{error}</p>}

      <button
        type="submit"
        disabled={saving}
        className="flex min-h-14 w-full items-center justify-center rounded-md bg-brass text-base font-medium text-ink disabled:opacity-60"
      >
        {saving ? dict.staff.saving : dict.staff.save}
      </button>
    </form>
  );
}
