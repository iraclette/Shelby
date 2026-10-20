import type { SupabaseClient } from "@supabase/supabase-js";

// The business's prefix (Shop Console → Settings → Checkout). Staff pages that make codes call
// loadSkuPrefix() when they open; until it answers, codes use the original INT.
let prefix = "INT";

export async function loadSkuPrefix(supabase: SupabaseClient) {
  const { data } = await supabase.from("business_settings").select("sku_prefix").maybeSingle();
  if (data?.sku_prefix) prefix = data.sku_prefix as string;
}

// Same format as the Shop Console's generateInternalSku() (ProductDialog.cpp).
export function generateInternalSku() {
  const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // no O/0/I/1 to avoid label confusion
  let code = "";
  for (let i = 0; i < 6; i++) code += alphabet[Math.floor(Math.random() * alphabet.length)];
  return `${prefix}-${code}`;
}
