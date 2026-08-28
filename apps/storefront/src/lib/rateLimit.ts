import "server-only";
import type { NextRequest } from "next/server";

// Fixed-window counter, in-process. Render runs this app as one persistent
// `next start` process (not per-request serverless functions), so this
// actually persists across requests here — it just wouldn't be shared if
// this ever scaled to multiple instances, and resets on every deploy/restart.
// Fine for this app's traffic; swap for a shared store (e.g. Redis) if that
// ever changes.
type Bucket = { count: number; resetAt: number };
const buckets = new Map<string, Bucket>();

export function checkRateLimit(key: string, limit: number, windowMs: number): boolean {
  const now = Date.now();
  const bucket = buckets.get(key);

  if (!bucket || now > bucket.resetAt) {
    buckets.set(key, { count: 1, resetAt: now + windowMs });
    return true;
  }

  if (bucket.count >= limit) return false;
  bucket.count += 1;
  return true;
}

export function getClientIp(request: NextRequest): string {
  // Render terminates TLS in front of this process and forwards the real
  // client IP as the first entry in X-Forwarded-For.
  const forwarded = request.headers.get("x-forwarded-for");
  if (forwarded) return forwarded.split(",")[0].trim();
  return "unknown";
}
