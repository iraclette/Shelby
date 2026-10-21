// One-off: make the ~400px <name>.thumb.jpg for product photos uploaded
// before thumbnails existed (0037_search_and_thumbnails.sql) and fill in
// product_images.thumb_path. Safe to re-run: only rows without one are touched.
//
//   node scripts/make-thumbnails.mjs           # dry run, lists photos
//   node scripts/make-thumbnails.mjs --apply   # upload thumbnails, set thumb_path
//
// Uses the Supabase CLI linked at the repo root, and sharp from Next's dependencies.
import { execFileSync } from "node:child_process";
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import sharp from "sharp";

const REPO_ROOT = join(dirname(fileURLToPath(import.meta.url)), "../../..");
const BUCKET = "product-images";
const THUMB_SIDE = 400; // same as lib/resizeImage.ts and shop-console
const apply = process.argv.includes("--apply");

function supabase(...args) {
  try {
    return execFileSync("supabase", [...args, "--linked"], {
      cwd: REPO_ROOT,
      encoding: "utf8",
      stdio: ["ignore", "pipe", "pipe"],
    });
  } catch (error) {
    // The CLI reports failures as JSON on stdout.
    throw new Error(`supabase ${args.slice(0, 2).join(" ")} failed: ${error.stdout || error.stderr || error.message}`);
  }
}

const sqlString = (value) => `'${value.replaceAll("'", "''")}'`;

const { rows } = JSON.parse(
  supabase("db", "query", "select id, storage_path from product_images where thumb_path is null order by created_at", "-o", "json"),
);
console.log(`${rows.length} photo(s) without a thumbnail`);

const work = mkdtempSync(join(tmpdir(), "make-thumbnails-"));
try {
  let made = 0;
  for (const { id, storage_path: storagePath } of rows) {
    const thumbPath = storagePath.replace(/\.[^./]*$/, "") + ".thumb.jpg";
    console.log(`${storagePath} -> ${thumbPath}`);
    if (!apply) continue;

    const original = join(work, "original");
    const thumb = join(work, "thumb.jpg");
    rmSync(original, { force: true });
    supabase("storage", "cp", `ss:///${BUCKET}/${storagePath}`, original, "--experimental");
    writeFileSync(
      thumb,
      await sharp(readFileSync(original))
        .rotate()
        .resize(THUMB_SIDE, THUMB_SIDE, { fit: "inside", withoutEnlargement: true })
        .flatten({ background: "#ffffff" })
        .jpeg({ quality: 78, mozjpeg: true })
        .toBuffer(),
    );
    // A thumbnail left over from an earlier, interrupted run is replaced.
    try {
      supabase("storage", "cp", thumb, `ss:///${BUCKET}/${thumbPath}`, "--experimental");
    } catch {
      supabase("storage", "rm", `ss:///${BUCKET}/${thumbPath}`, "--experimental");
      supabase("storage", "cp", thumb, `ss:///${BUCKET}/${thumbPath}`, "--experimental");
    }
    supabase("db", "query", `update product_images set thumb_path = ${sqlString(thumbPath)} where id = ${sqlString(id)}`);
    made += 1;
  }
  console.log(apply ? `\n${made} thumbnail(s) made` : "\nDry run, pass --apply to make them");
} finally {
  rmSync(work, { recursive: true, force: true });
}
