"use client";

import { useEffect, useRef, useState } from "react";
import Link from "next/link";
import type { Session } from "@supabase/supabase-js";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";
import CategoryOptions, { type StaffCategory } from "@/components/CategoryOptions";
import Skeleton from "@/components/Skeleton";
import { randomId } from "@/lib/randomId";
import { uploadProductPhoto } from "@/lib/productPhotos";
import { translateDescription, withStaffEdits } from "@/lib/descriptions";
import { LOCALE_LABELS, LOCALES, type Locale } from "@/i18n/config";
import { generateInternalSku, loadSkuPrefix } from "@/lib/sku";
import { useI18n } from "@/i18n/client";
import { fmt } from "@/i18n/format";

type Shop = { id: string; name: string };
// variantKey: which NewVariant the photo shows ("" = the product in general).
type PickedPhoto = { id: string; file: File; previewUrl: string; variantKey: string };
// key is local to the form; the database id only exists once it's saved.
type NewVariant = { key: string; name: string; stock: Record<string, string> };

const inputClass =
  "w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass";
const labelClass = "block text-sm text-paper-dim";

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
  const { dict, lang } = useI18n();

  const [shops, setShops] = useState<Shop[]>([]);
  const [categories, setCategories] = useState<StaffCategory[]>([]);
  const [listsLoaded, setListsLoaded] = useState(false);

  const [name, setName] = useState("");
  const [categoryId, setCategoryId] = useState("");
  const [sellPrice, setSellPrice] = useState("");
  const [costPrice, setCostPrice] = useState("");
  const [description, setDescription] = useState("");
  const [descriptionLang, setDescriptionLang] = useState<Locale>(lang);
  const [stock, setStock] = useState<Record<string, string>>({});
  const [photos, setPhotos] = useState<PickedPhoto[]>([]);
  const [variants, setVariants] = useState<NewVariant[]>([]);

  const [saving, setSaving] = useState(false);
  const [error, setError] = useState("");
  const [created, setCreated] = useState<{ id: string; name: string; warning: string } | null>(null);

  const cameraInputRef = useRef<HTMLInputElement>(null);
  const galleryInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    loadSkuPrefix(supabase);
    Promise.all([
      supabase.from("shops").select("id, name").order("name"),
      supabase.from("categories").select("id, name, parent_id").order("name"),
    ]).then(([shopsRes, categoriesRes]) => {
      setShops((shopsRes.data as Shop[]) ?? []);
      setCategories((categoriesRes.data as StaffCategory[]) ?? []);
      setListsLoaded(true);
    });
  }, [supabase]);

  function addPhotos(fileList: FileList | null) {
    if (!fileList) return;
    const picked = Array.from(fileList).map((file) => ({
      id: randomId(),
      file,
      previewUrl: URL.createObjectURL(file),
      variantKey: "",
    }));
    setPhotos((current) => [...current, ...picked]);
    if (cameraInputRef.current) cameraInputRef.current.value = "";
    if (galleryInputRef.current) galleryInputRef.current.value = "";
  }

  function removePhoto(photo: PickedPhoto) {
    URL.revokeObjectURL(photo.previewUrl);
    setPhotos((current) => current.filter((p) => p.id !== photo.id));
  }

  function updateVariant(key: string, change: Partial<NewVariant>) {
    setVariants((current) => current.map((v) => (v.key === key ? { ...v, ...change } : v)));
  }

  function removeVariant(key: string) {
    setVariants((current) => current.filter((v) => v.key !== key));
    setPhotos((current) => current.map((p) => (p.variantKey === key ? { ...p, variantKey: "" } : p)));
  }

  function resetForm() {
    photos.forEach((photo) => URL.revokeObjectURL(photo.previewUrl));
    setName("");
    setSellPrice("");
    setCostPrice("");
    setDescription("");
    setStock({});
    setPhotos([]);
    setVariants([]);
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
    const namedVariants = variants.filter((v) => v.name.trim());
    const variantNames = namedVariants.map((v) => v.name.trim().toLowerCase());
    const duplicate = namedVariants.find((v, i) => variantNames.indexOf(v.name.trim().toLowerCase()) !== i);
    if (duplicate) return setError(fmt(dict.staff.variantExists, { name: duplicate.name.trim() }));

    setSaving(true);

    const { data: product, error: productError } = await insertProduct({
      name: name.trim(),
      descriptions: description.trim()
        ? withStaffEdits({}, { [descriptionLang]: description }, descriptionLang)
        : {},
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
    // The other languages fill in on the server within a few seconds.
    if (description.trim()) void translateDescription(supabase, product.id);

    // Form key -> saved variant id.
    const variantIds = new Map<string, string>();
    for (const variant of namedVariants) {
      let result = await supabase
        .from("product_variants")
        .insert({ product_id: product.id, name: variant.name.trim(), sku: generateInternalSku() })
        .select("id")
        .single();
      // Retry on the rare random-SKU collision.
      for (let attempt = 0; attempt < 2 && result.error?.code === "23505"; attempt++) {
        result = await supabase
          .from("product_variants")
          .insert({ product_id: product.id, name: variant.name.trim(), sku: generateInternalSku() })
          .select("id")
          .single();
      }
      if (result.data) variantIds.set(variant.key, result.data.id);
    }
    if (variantIds.size < namedVariants.length) problems.push(dict.staff.variantsFailed);

    // A product with variants only has stock per variant.
    const stockRows =
      namedVariants.length > 0
        ? namedVariants.flatMap((variant) =>
            variantIds.has(variant.key)
              ? shops.map((shop) => ({ shop, variantId: variantIds.get(variant.key)!, text: variant.stock[shop.id] }))
              : [],
          )
        : shops.map((shop) => ({ shop, variantId: null as string | null, text: stock[shop.id] }));
    const levels = stockRows
      .map((row) => ({
        product_id: product.id,
        shop_id: row.shop.id,
        variant_id: row.variantId,
        quantity: parseInt(row.text ?? "", 10),
      }))
      .filter((level) => Number.isInteger(level.quantity) && level.quantity > 0);
    if (levels.length > 0) {
      const { error: stockError } = await supabase.from("inventory_levels").insert(levels);
      if (stockError) problems.push(dict.staff.stockFailed);
    }

    let failedPhotos = 0;
    for (const [i, photo] of photos.entries()) {
      const paths = await uploadProductPhoto(supabase, product.id, photo.file);
      const { error: rowError } = !paths
        ? { error: true }
        : await supabase
            .from("product_images")
            .insert({
              product_id: product.id,
              variant_id: variantIds.get(photo.variantKey) ?? null,
              ...paths,
              is_primary: i === 0,
            });
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
        detail:
          `created from phone: ${name.trim()} @ GEL ${sell.toFixed(2)}` +
          (namedVariants.length > 0 ? `, variants: ${namedVariants.map((v) => v.name.trim()).join(", ")}` : ""),
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
                {variants.some((v) => v.name.trim()) && (
                  <select
                    value={photo.variantKey}
                    onChange={(e) =>
                      setPhotos((current) =>
                        current.map((p) => (p.id === photo.id ? { ...p, variantKey: e.target.value } : p)),
                      )
                    }
                    aria-label={dict.staff.photoOf}
                    className="absolute inset-x-0 bottom-0 bg-ink/85 px-1 py-1.5 text-xs text-paper outline-none"
                  >
                    <option value="">{dict.staff.allVariants}</option>
                    {variants
                      .filter((v) => v.name.trim())
                      .map((v) => (
                        <option key={v.key} value={v.key}>
                          {v.name.trim()}
                        </option>
                      ))}
                  </select>
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
          <CategoryOptions categories={categories} dict={dict} />
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

      <div>
        <div className="flex items-center justify-between">
          <label htmlFor="description" className={labelClass}>
            {dict.staff.description}
          </label>
          <select
            value={descriptionLang}
            onChange={(e) => setDescriptionLang(e.target.value as Locale)}
            aria-label={dict.staff.writtenIn}
            className="rounded-md border border-line bg-ink-soft px-2 py-1 text-xs text-paper-dim"
          >
            {LOCALES.map((locale) => (
              <option key={locale} value={locale}>
                {LOCALE_LABELS[locale].name}
              </option>
            ))}
          </select>
        </div>
        <textarea
          id="description"
          value={description}
          onChange={(e) => setDescription(e.target.value)}
          rows={3}
          className={`mt-1 ${inputClass}`}
        />
      </div>

      {!listsLoaded && (
        <div className="space-y-2" aria-busy="true">
          <Skeleton className="h-4 w-44" />
          {[0, 1, 2].map((i) => (
            <Skeleton key={i} className="h-12.5 w-full rounded-md" />
          ))}
        </div>
      )}

      <fieldset>
        <legend className={labelClass}>{dict.staff.variants}</legend>
        {variants.length === 0 && <p className="mt-1 text-xs text-paper-dim">{dict.staff.variantsHint}</p>}
        {variants.length > 0 && <p className="mt-1 text-xs text-paper-dim">{dict.staff.variantStock}</p>}
        <div className="mt-2 space-y-3">
          {variants.map((variant) => (
            <div key={variant.key} className="rounded-md border border-line p-3">
              <div className="flex items-center gap-2">
                <input
                  value={variant.name}
                  onChange={(e) => updateVariant(variant.key, { name: e.target.value })}
                  placeholder={dict.staff.variantPlaceholder}
                  className={inputClass}
                />
                <button
                  type="button"
                  onClick={() => removeVariant(variant.key)}
                  aria-label={dict.staff.removeVariant}
                  className="flex h-11 w-11 flex-shrink-0 items-center justify-center text-xl text-paper-dim active:text-brass"
                >
                  ×
                </button>
              </div>
              {shops.map((shop) => (
                <label key={shop.id} className="mt-2 flex items-center justify-between gap-3">
                  <span className="text-sm text-paper-dim">{shop.name}</span>
                  <input
                    value={variant.stock[shop.id] ?? ""}
                    onChange={(e) =>
                      updateVariant(variant.key, {
                        stock: { ...variant.stock, [shop.id]: e.target.value.replace(/\D/g, "") },
                      })
                    }
                    inputMode="numeric"
                    placeholder="0"
                    className="w-24 rounded-md border border-line bg-ink-soft px-4 py-2 text-right text-base text-paper outline-none focus:border-brass"
                  />
                </label>
              ))}
            </div>
          ))}
        </div>
        <button
          type="button"
          onClick={() => setVariants((current) => [...current, { key: randomId(), name: "", stock: {} }])}
          className="mt-2 min-h-11 text-sm text-brass active:text-paper"
        >
          {dict.staff.addVariant}
        </button>
      </fieldset>

      {shops.length > 0 && variants.length === 0 && (
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
