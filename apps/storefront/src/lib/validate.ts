// Shared request-body validation for the storefront's API routes — replaces
// bare truthiness checks (`!body.x`) with actual type/length/format bounds,
// since these values go straight into Supabase inserts otherwise.

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
const UUID_RE = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

export function isValidEmail(value: unknown): value is string {
  return typeof value === "string" && value.length <= 254 && EMAIL_RE.test(value);
}

export function isNonEmptyString(value: unknown, maxLen: number): value is string {
  return typeof value === "string" && value.trim().length > 0 && value.length <= maxLen;
}

export function isOptionalString(value: unknown, maxLen: number): boolean {
  return value === undefined || value === null || (typeof value === "string" && value.length <= maxLen);
}

export function isUuid(value: unknown): value is string {
  return typeof value === "string" && UUID_RE.test(value);
}
