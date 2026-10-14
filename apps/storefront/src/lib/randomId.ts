// crypto.randomUUID() is undefined over plain http (e.g. testing from a phone on LAN).
export function randomId() {
  if (typeof crypto !== "undefined" && crypto.randomUUID) return crypto.randomUUID();
  return `${Date.now()}-${Math.random().toString(16).slice(2)}`;
}
