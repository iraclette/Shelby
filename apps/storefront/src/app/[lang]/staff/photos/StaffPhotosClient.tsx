"use client";

import { useRef, useState } from "react";
import Image from "next/image";
import { getSupabaseClient } from "@/lib/supabase";
import StaffAuthGate from "@/components/StaffAuthGate";
import Skeleton from "@/components/Skeleton";
import { removeProductPhoto, uploadProductPhoto } from "@/lib/productPhotos";
import { useI18n } from "@/i18n/client";
import { fmt } from "@/i18n/format";

type Product = { id: string; name: string; sku: string };
type ProductImage = {
  id: string;
  storage_path: string;
  thumb_path: string | null;
  is_primary: boolean;
  variant_id: string | null;
};
type Variant = { id: string; name: string };

function publicUrl(storagePath: string) {
  return `${process.env.NEXT_PUBLIC_SUPABASE_URL}/storage/v1/object/public/product-images/${storagePath}`;
}

export default function StaffPhotosClient() {
  return <StaffAuthGate>{({ signOut }) => <PhotosManager onSignOut={signOut} />}</StaffAuthGate>;
}

function PhotosManager({ onSignOut }: { onSignOut: () => void }) {
  const supabase = getSupabaseClient();
  const { dict } = useI18n();

  const [query, setQuery] = useState("");
  const [results, setResults] = useState<Product[]>([]);
  const [searching, setSearching] = useState(false);

  const [selected, setSelected] = useState<Product | null>(null);
  const [images, setImages] = useState<ProductImage[]>([]);
  const [variants, setVariants] = useState<Variant[]>([]);
  const [imagesLoading, setImagesLoading] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [statusMessage, setStatusMessage] = useState("");

  const fileInputRef = useRef<HTMLInputElement>(null);
  const galleryInputRef = useRef<HTMLInputElement>(null);

  async function runSearch(text: string) {
    setQuery(text);
    if (text.trim().length < 2) {
      setResults([]);
      return;
    }
    setSearching(true);
    const { data } = await supabase
      .from("products")
      .select("id, name, sku")
      .ilike("name", `%${text.trim()}%`)
      .order("name")
      .limit(20);
    setResults((data as Product[]) ?? []);
    setSearching(false);
  }

  async function loadImages(productId: string) {
    setImagesLoading(true);
    const [{ data }, { data: variantRows }] = await Promise.all([
      supabase
        .from("product_images")
        .select("id, storage_path, thumb_path, is_primary, variant_id")
        .eq("product_id", productId)
        .order("is_primary", { ascending: false }),
      supabase.from("product_variants").select("id, name").eq("product_id", productId),
    ]);
    setImages((data as ProductImage[]) ?? []);
    setVariants(
      ((variantRows as Variant[]) ?? []).sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true })),
    );
    setImagesLoading(false);
  }

  function selectProduct(product: Product) {
    setSelected(product);
    setStatusMessage("");
    loadImages(product.id);
  }

  async function handleFiles(fileList: FileList | null) {
    if (!fileList || fileList.length === 0 || !selected) return;
    setUploading(true);
    setStatusMessage("");

    let hadExistingImage = images.length > 0;
    let failures = 0;

    for (const file of Array.from(fileList)) {
      const paths = await uploadProductPhoto(supabase, selected.id, file);
      if (!paths) {
        failures += 1;
        continue;
      }

      const { error: insertError } = await supabase
        .from("product_images")
        .insert({ product_id: selected.id, ...paths, is_primary: !hadExistingImage });

      if (insertError) {
        failures += 1;
        continue;
      }
      hadExistingImage = true;
    }

    setUploading(false);
    setStatusMessage(failures > 0 ? fmt(dict.staff.uploadFailed, { n: failures }) : dict.staff.uploaded);
    if (fileInputRef.current) fileInputRef.current.value = "";
    if (galleryInputRef.current) galleryInputRef.current.value = "";
    loadImages(selected.id);
  }

  async function setImageVariant(image: ProductImage, variantId: string | null) {
    setImages((current) => current.map((i) => (i.id === image.id ? { ...i, variant_id: variantId } : i)));
    const { error } = await supabase.from("product_images").update({ variant_id: variantId }).eq("id", image.id);
    if (error && selected) {
      setStatusMessage(dict.staff.photoVariantFailed);
      loadImages(selected.id);
    }
  }

  async function removeImage(image: ProductImage) {
    if (!selected) return;
    if (!confirm(dict.staff.confirmRemove)) return;
    await removeProductPhoto(supabase, image);
    loadImages(selected.id);
  }

  if (selected) {
    return (
      <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
        <button
          onClick={() => setSelected(null)}
          className="mb-4 min-h-11 text-sm text-paper-dim active:text-brass"
        >
          {dict.staff.backToSearch}
        </button>
        <h1 className="font-display text-2xl text-paper">{selected.name}</h1>
        <p className="text-sm text-paper-dim">{selected.sku}</p>

        <input
          ref={fileInputRef}
          type="file"
          accept="image/*"
          capture="environment"
          className="hidden"
          onChange={(e) => handleFiles(e.target.files)}
        />
        <input
          ref={galleryInputRef}
          type="file"
          accept="image/*"
          multiple
          className="hidden"
          onChange={(e) => handleFiles(e.target.files)}
        />
        {uploading ? (
          <p className="mt-6 flex min-h-14 items-center justify-center rounded-md bg-brass/60 text-base font-medium text-ink">
            {dict.staff.uploading}
          </p>
        ) : (
          <div className="mt-6 grid grid-cols-2 gap-3">
            <button
              onClick={() => fileInputRef.current?.click()}
              className="min-h-14 rounded-md bg-brass text-base font-medium text-ink"
            >
              {dict.staff.takePhoto}
            </button>
            <button
              onClick={() => galleryInputRef.current?.click()}
              className="min-h-14 rounded-md border border-line text-base text-paper active:border-brass"
            >
              {dict.staff.fromGallery}
            </button>
          </div>
        )}
        {statusMessage && <p className="mt-2 text-sm text-paper-dim">{statusMessage}</p>}

        <div className="mt-6 grid grid-cols-3 gap-2">
          {imagesLoading &&
            images.length === 0 &&
            [0, 1, 2].map((i) => <Skeleton key={i} className="aspect-square rounded-md" />)}
          {!imagesLoading && images.length === 0 && (
            <p className="col-span-3 text-sm text-paper-dim">{dict.staff.noPhotos}</p>
          )}
          {images.map((image) => (
            <div key={image.id}>
              <button
                onClick={() => removeImage(image)}
                className="relative block aspect-square w-full overflow-hidden rounded-md border border-line"
              >
                <Image src={publicUrl(image.thumb_path ?? image.storage_path)} alt="" fill sizes="33vw" className="object-cover" />
                {image.is_primary && (
                  <span className="absolute left-1 top-1 rounded bg-ink/80 px-1.5 py-0.5 text-[10px] text-brass">
                    {dict.staff.primary}
                  </span>
                )}
              </button>
              {variants.length > 0 && (
                <select
                  value={image.variant_id ?? ""}
                  onChange={(e) => setImageVariant(image, e.target.value || null)}
                  aria-label={dict.staff.photoOf}
                  className="mt-1 min-h-9 w-full rounded-md border border-line bg-ink-soft px-1 text-xs text-paper outline-none focus:border-brass"
                >
                  <option value="">{dict.staff.allVariants}</option>
                  {variants.map((variant) => (
                    <option key={variant.id} value={variant.id}>
                      {variant.name}
                    </option>
                  ))}
                </select>
              )}
            </div>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper">{dict.staff.productPhotos}</h1>
        <button onClick={onSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          {dict.common.signOut}
        </button>
      </div>
      <input
        value={query}
        onChange={(e) => runSearch(e.target.value)}
        placeholder={dict.common.searchProducts}
        autoCapitalize="none"
        className="mt-4 w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
      />
      <div className="mt-4 space-y-2">
        {searching &&
          results.length === 0 &&
          [0, 1, 2].map((i) => <Skeleton key={i} className="h-14 w-full rounded-md" />)}
        {!searching && query.trim().length >= 2 && results.length === 0 && (
          <p className="text-sm text-paper-dim">{fmt(dict.common.noMatch, { query })}</p>
        )}
        {results.map((product) => (
          <button
            key={product.id}
            onClick={() => selectProduct(product)}
            className="flex min-h-14 w-full items-center justify-between rounded-md border border-line px-4 text-left active:border-brass"
          >
            <span className="text-paper">{product.name}</span>
            <span className="text-xs text-paper-dim">{product.sku}</span>
          </button>
        ))}
      </div>
    </div>
  );
}
