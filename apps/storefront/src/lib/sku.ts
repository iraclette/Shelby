// Same format as the Shop Console's generateInternalSku() (ProductDialog.cpp).
export function generateInternalSku() {
  const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // no O/0/I/1 to avoid label confusion
  let code = "";
  for (let i = 0; i < 6; i++) code += alphabet[Math.floor(Math.random() * alphabet.length)];
  return `INT-${code}`;
}
