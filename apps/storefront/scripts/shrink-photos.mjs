// One-off: shrink product photos that were uploaded before resize-on-upload
// (lib/resizeImage.ts, shop-console v2.3.1). Paths and formats stay the same,
// so product_images rows and existing links keep working.
//
//   node scripts/shrink-photos.mjs           # dry run, prints savings
//   node scripts/shrink-photos.mjs --apply   # overwrite in storage
//
// The CLI can't overwrite (409 Duplicate), so --apply removes each object and
// uploads the shrunk copy, putting the original back if that upload fails.
// Originals are kept in the printed backup folder.
//
// Uses the Supabase CLI linked at the repo root, and sharp
// from Next's dependencies.
import { execFileSync } from "node:child_process";
import { cpSync, mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { dirname, extname, join, relative } from "node:path";
import { fileURLToPath } from "node:url";
import sharp from "sharp";

const REPO_ROOT = join(dirname(fileURLToPath(import.meta.url)), "../../..");
const BUCKET = "product-images";
const MAX_SIDE = 1600;
const apply = process.argv.includes("--apply");

function storage(...args) {
  try {
    execFileSync("supabase", ["storage", ...args, "--experimental", "--linked"], {
      cwd: REPO_ROOT,
      stdio: ["ignore", "pipe", "pipe"],
    });
  } catch (error) {
    // The CLI reports failures as JSON on stdout.
    throw new Error(`supabase storage ${args[0]} failed: ${error.stdout || error.stderr || error.message}`);
  }
}

function* walk(dir) {
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const path = join(dir, entry.name);
    if (entry.isDirectory()) yield* walk(path);
    else yield path;
  }
}

const work = mkdtempSync(join(tmpdir(), "shrink-photos-"));
try {
  console.log(`Downloading ${BUCKET}...`);
  storage("cp", "-r", `ss:///${BUCKET}`, work);
  const root = join(work, BUCKET);
  const backup = join(work, "originals");
  if (apply) {
    cpSync(root, backup, { recursive: true });
    console.log(`Originals backed up to ${backup}`);
  }

  let before = 0;
  let after = 0;
  let changed = 0;
  for (const file of walk(root)) {
    const original = readFileSync(file);
    before += original.length;

    const ext = extname(file).toLowerCase();
    let image = sharp(original).rotate().resize(MAX_SIDE, MAX_SIDE, { fit: "inside", withoutEnlargement: true });
    if (ext === ".jpg" || ext === ".jpeg") image = image.jpeg({ quality: 82, mozjpeg: true });
    else if (ext === ".png") image = image.png({ compressionLevel: 9 });
    else if (ext === ".webp") image = image.webp({ quality: 82 });
    else {
      after += original.length;
      continue;
    }

    const shrunk = await image.toBuffer();
    // Not worth a re-upload unless it saves at least 10%.
    if (shrunk.length > original.length * 0.9) {
      after += original.length;
      continue;
    }

    const path = relative(root, file);
    after += shrunk.length;
    changed += 1;
    console.log(`${path}: ${(original.length / 1e6).toFixed(2)} MB -> ${(shrunk.length / 1e6).toFixed(2)} MB`);
    if (apply) {
      writeFileSync(file, shrunk);
      storage("rm", `ss:///${BUCKET}/${path}`);
      try {
        storage("cp", file, `ss:///${BUCKET}/${path}`);
      } catch (error) {
        storage("cp", join(backup, path), `ss:///${BUCKET}/${path}`);
        throw error;
      }
    }
  }

  console.log(
    `\n${changed} photo(s), ${(before / 1e6).toFixed(1)} MB -> ${(after / 1e6).toFixed(1)} MB` +
      (apply ? " (uploaded)" : " (dry run, pass --apply to upload)"),
  );
} finally {
  if (!apply) rmSync(work, { recursive: true, force: true });
}
