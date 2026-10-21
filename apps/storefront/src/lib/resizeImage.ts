// Product photos are shrunk in the browser before upload so the storefront
// can serve them straight from Supabase (images.unoptimized) instead of
// resizing full-size phone photos on the server. Same sizes as shop-console.
const MAX_SIDE = 1600;
const QUALITY = 0.82;
const THUMB_SIDE = 400;
const THUMB_QUALITY = 0.78;

async function toJpeg(file: File, maxSide: number, quality: number): Promise<Blob | null> {
  try {
    const bitmap = await createImageBitmap(file, { imageOrientation: "from-image" });
    const scale = Math.min(1, maxSide / Math.max(bitmap.width, bitmap.height));
    const width = Math.round(bitmap.width * scale);
    const height = Math.round(bitmap.height * scale);

    const canvas = document.createElement("canvas");
    canvas.width = width;
    canvas.height = height;
    const context = canvas.getContext("2d")!;
    // JPEG has no alpha; flatten transparent PNGs onto white.
    context.fillStyle = "#fff";
    context.fillRect(0, 0, width, height);
    context.drawImage(bitmap, 0, 0, width, height);
    bitmap.close();

    return await new Promise<Blob | null>((resolve) => canvas.toBlob(resolve, "image/jpeg", quality));
  } catch {
    // Formats the browser can't decode (e.g. HEIC outside Safari).
    return null;
  }
}

function baseName(file: File) {
  return file.name.replace(/\.[^.]*$/, "");
}

export async function resizeImage(file: File): Promise<File> {
  const blob = await toJpeg(file, MAX_SIDE, QUALITY);
  if (!blob || blob.size >= file.size) return file;
  return new File([blob], baseName(file) + ".jpg", { type: "image/jpeg" });
}

// Null when the browser can't decode the photo; the site then shows the original.
export async function makeThumbnail(file: File): Promise<File | null> {
  const blob = await toJpeg(file, THUMB_SIDE, THUMB_QUALITY);
  return blob && new File([blob], baseName(file) + ".thumb.jpg", { type: "image/jpeg" });
}
