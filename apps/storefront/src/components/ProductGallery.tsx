"use client";

import { useRef, useState } from "react";
import Image from "next/image";
import { productImageUrl, type ProductImage } from "@/lib/supabase";
import { useI18n } from "@/i18n/client";
import { fmt } from "@/i18n/format";

const SWIPE_THRESHOLD_PX = 40;

// Main image + clickable thumbnails; swipe left/right on the main image on phones.
export default function ProductGallery({ images, alt }: { images: ProductImage[]; alt: string }) {
  const [index, setIndex] = useState(0);
  const touchStartX = useRef<number | null>(null);
  const { dict } = useI18n();

  if (images.length === 0) {
    return (
      <div className="relative aspect-square overflow-hidden border border-line bg-ink-elevated">
        <div className="flex h-full items-center justify-center text-paper-dim/80">
          <span className="font-display text-4xl italic">Shelby</span>
        </div>
      </div>
    );
  }

  const current = images[index];

  function step(delta: number) {
    setIndex((i) => (i + delta + images.length) % images.length);
  }

  function handleTouchEnd(event: React.TouchEvent) {
    if (touchStartX.current === null) return;
    const dx = event.changedTouches[0].clientX - touchStartX.current;
    touchStartX.current = null;
    if (Math.abs(dx) >= SWIPE_THRESHOLD_PX) step(dx < 0 ? 1 : -1);
  }

  return (
    <div>
      <div
        className="relative aspect-square overflow-hidden border border-line bg-ink-elevated"
        onTouchStart={(e) => (touchStartX.current = e.touches[0].clientX)}
        onTouchEnd={handleTouchEnd}
      >
        <Image
          key={current.storage_path}
          src={productImageUrl(current.storage_path)}
          alt={alt}
          fill
          sizes="(min-width: 768px) 50vw, 100vw"
          className="object-cover"
          priority={index === 0}
        />
        {images.length > 1 && (
          <p className="absolute right-3 bottom-3 rounded-sm bg-ink/80 px-2 py-1 text-xs text-paper">
            {index + 1} / {images.length}
          </p>
        )}
      </div>

      {images.length > 1 && (
        <div className="mt-4 grid grid-cols-5 gap-3">
          {images.map((image, i) => (
            <button
              key={image.storage_path}
              type="button"
              onClick={() => setIndex(i)}
              aria-label={fmt(dict.product.showPhoto, { n: i + 1, total: images.length })}
              aria-current={i === index}
              className={`relative aspect-square cursor-pointer overflow-hidden border bg-ink-elevated transition-colors ${
                i === index ? "border-brass" : "border-line hover:border-paper-dim"
              }`}
            >
              <Image
                src={productImageUrl(image.storage_path)}
                alt=""
                fill
                sizes="20vw"
                className={`object-cover transition-opacity ${i === index ? "" : "opacity-70"}`}
              />
            </button>
          ))}
        </div>
      )}
    </div>
  );
}
