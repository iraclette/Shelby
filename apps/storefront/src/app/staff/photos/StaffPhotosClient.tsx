"use client";

import { useEffect, useRef, useState } from "react";
import type { Session } from "@supabase/supabase-js";
import Image from "next/image";
import { getSupabaseClient } from "@/lib/supabase";

type Product = { id: string; name: string; sku: string };
type ProductImage = { id: string; storage_path: string; is_primary: boolean };

// crypto.randomUUID() needs a secure context (HTTPS, or localhost) — but
// testing this from a phone against `next dev` usually means hitting the
// dev machine's plain-http LAN address, where it's undefined. This works
// everywhere.
function randomId() {
  if (typeof crypto !== "undefined" && crypto.randomUUID) return crypto.randomUUID();
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}

function publicUrl(storagePath: string) {
  return `${process.env.NEXT_PUBLIC_SUPABASE_URL}/storage/v1/object/public/product-images/${storagePath}`;
}

// Mirrors the Shop Console's login convention: staff sign in with a short
// username, not a full email — Supabase Auth still needs an email shape
// under the hood, so anything without "@" gets this fixed fake domain.
function toEmail(identifier: string) {
  const trimmed = identifier.trim();
  return trimmed.includes("@") ? trimmed : `${trimmed}@shop.local`;
}

export default function StaffPhotosClient() {
  const supabase = getSupabaseClient();

  const [checkingSession, setCheckingSession] = useState(true);
  const [session, setSession] = useState<Session | null>(null);
  const [isAdmin, setIsAdmin] = useState<boolean | null>(null);

  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [loginError, setLoginError] = useState("");
  const [signingIn, setSigningIn] = useState(false);

  const [query, setQuery] = useState("");
  const [results, setResults] = useState<Product[]>([]);
  const [searching, setSearching] = useState(false);

  const [selected, setSelected] = useState<Product | null>(null);
  const [images, setImages] = useState<ProductImage[]>([]);
  const [imagesLoading, setImagesLoading] = useState(false);
  const [uploading, setUploading] = useState(false);
  const [statusMessage, setStatusMessage] = useState("");

  const fileInputRef = useRef<HTMLInputElement>(null);

  // Restore an existing session (this is a bookmarked page — sign in once,
  // stays signed in via Supabase's own localStorage persistence).
  useEffect(() => {
    supabase.auth.getSession().then(({ data }) => {
      setSession(data.session);
      setCheckingSession(false);
    });
    const { data: sub } = supabase.auth.onAuthStateChange((_event, newSession) => {
      setSession(newSession);
      if (!newSession) setIsAdmin(null);
    });
    return () => sub.subscription.unsubscribe();
  }, [supabase]);

  useEffect(() => {
    if (!session) return;
    supabase
      .from("profiles")
      .select("role")
      .eq("id", session.user.id)
      .maybeSingle()
      .then(({ data }) => {
        setIsAdmin(data?.role === "owner" || data?.role === "admin");
      });
  }, [session, supabase]);

  async function handleSignIn(event: React.FormEvent) {
    event.preventDefault();
    setLoginError("");
    setSigningIn(true);
    const { error } = await supabase.auth.signInWithPassword({
      email: toEmail(username),
      password,
    });
    setSigningIn(false);
    if (error) setLoginError(error.message);
  }

  async function handleSignOut() {
    await supabase.auth.signOut();
    setSelected(null);
  }

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
    const { data } = await supabase
      .from("product_images")
      .select("id, storage_path, is_primary")
      .eq("product_id", productId)
      .order("is_primary", { ascending: false });
    setImages((data as ProductImage[]) ?? []);
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
      const extension = file.name.includes(".") ? file.name.split(".").pop() : "jpg";
      const storagePath = `${selected.id}/${randomId()}.${extension}`;

      const { error: uploadError } = await supabase.storage
        .from("product-images")
        .upload(storagePath, file, { contentType: file.type || "image/jpeg" });

      if (uploadError) {
        failures += 1;
        continue;
      }

      const { error: insertError } = await supabase
        .from("product_images")
        .insert({ product_id: selected.id, storage_path: storagePath, is_primary: !hadExistingImage });

      if (insertError) {
        failures += 1;
        continue;
      }
      hadExistingImage = true;
    }

    setUploading(false);
    setStatusMessage(failures > 0 ? `${failures} photo(s) failed to upload.` : "Uploaded.");
    if (fileInputRef.current) fileInputRef.current.value = "";
    loadImages(selected.id);
  }

  async function removeImage(image: ProductImage) {
    if (!selected) return;
    if (!confirm("Remove this photo?")) return;
    await supabase.storage.from("product-images").remove([image.storage_path]);
    await supabase.from("product_images").delete().eq("id", image.id);
    loadImages(selected.id);
  }

  if (checkingSession) {
    return <CenteredMessage>Loading…</CenteredMessage>;
  }

  if (!session) {
    return (
      <CenteredMessage>
        <form onSubmit={handleSignIn} className="w-full max-w-sm space-y-4">
          <h1 className="font-display text-2xl text-paper">Shelby Staff</h1>
          {loginError && <p className="text-sm text-red-400">{loginError}</p>}
          <input
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            placeholder="Username"
            autoCapitalize="none"
            autoCorrect="off"
            className="w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
          />
          <input
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            type="password"
            placeholder="Password"
            className="w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
          />
          <button
            type="submit"
            disabled={signingIn}
            className="w-full rounded-md bg-brass px-4 py-3 text-base font-medium text-ink disabled:opacity-60"
          >
            {signingIn ? "Signing in…" : "Sign in"}
          </button>
        </form>
      </CenteredMessage>
    );
  }

  if (isAdmin === null) {
    return <CenteredMessage>Loading…</CenteredMessage>;
  }

  if (!isAdmin) {
    return (
      <CenteredMessage>
        <p className="text-paper-dim">This account isn&apos;t an admin, so it can&apos;t manage photos.</p>
        <button onClick={handleSignOut} className="mt-4 text-sm text-brass underline">
          Sign out
        </button>
      </CenteredMessage>
    );
  }

  if (selected) {
    return (
      <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
        <button
          onClick={() => setSelected(null)}
          className="mb-4 min-h-11 text-sm text-paper-dim active:text-brass"
        >
          ← Back to search
        </button>
        <h1 className="font-display text-2xl text-paper">{selected.name}</h1>
        <p className="text-sm text-paper-dim">{selected.sku}</p>

        <input
          ref={fileInputRef}
          type="file"
          accept="image/*"
          capture="environment"
          multiple
          className="hidden"
          onChange={(e) => handleFiles(e.target.files)}
        />
        <button
          onClick={() => fileInputRef.current?.click()}
          disabled={uploading}
          className="mt-6 flex min-h-14 w-full items-center justify-center gap-2 rounded-md bg-brass text-base font-medium text-ink disabled:opacity-60"
        >
          {uploading ? "Uploading…" : "Take Photo"}
        </button>
        {statusMessage && <p className="mt-2 text-sm text-paper-dim">{statusMessage}</p>}

        <div className="mt-6 grid grid-cols-3 gap-2">
          {imagesLoading && <p className="col-span-3 text-sm text-paper-dim">Loading photos…</p>}
          {!imagesLoading && images.length === 0 && (
            <p className="col-span-3 text-sm text-paper-dim">No photos yet.</p>
          )}
          {images.map((image) => (
            <button
              key={image.id}
              onClick={() => removeImage(image)}
              className="relative aspect-square overflow-hidden rounded-md border border-line"
            >
              <Image src={publicUrl(image.storage_path)} alt="" fill sizes="33vw" className="object-cover" />
              {image.is_primary && (
                <span className="absolute left-1 top-1 rounded bg-ink/80 px-1.5 py-0.5 text-[10px] text-brass">
                  primary
                </span>
              )}
            </button>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="mx-auto min-h-dvh max-w-lg px-4 py-6">
      <div className="flex items-center justify-between">
        <h1 className="font-display text-2xl text-paper">Product Photos</h1>
        <button onClick={handleSignOut} className="min-h-11 text-sm text-paper-dim active:text-brass">
          Sign out
        </button>
      </div>
      <input
        value={query}
        onChange={(e) => runSearch(e.target.value)}
        placeholder="Search products…"
        autoCapitalize="none"
        className="mt-4 w-full rounded-md border border-line bg-ink-soft px-4 py-3 text-base text-paper outline-none focus:border-brass"
      />
      <div className="mt-4 space-y-2">
        {searching && <p className="text-sm text-paper-dim">Searching…</p>}
        {!searching && query.trim().length >= 2 && results.length === 0 && (
          <p className="text-sm text-paper-dim">No products match &quot;{query}&quot;.</p>
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

function CenteredMessage({ children }: { children: React.ReactNode }) {
  return (
    <div className="flex min-h-dvh items-center justify-center px-4 text-center">
      <div>{children}</div>
    </div>
  );
}
