import "server-only";
import crypto from "node:crypto";

// Bank of Georgia's "Online Payment API" (the current one — its predecessor,
// "iPay", is deprecated). Docs: https://api.bog.ge/docs/en/payments/introduction
// Stripe isn't usable here at all — Georgia isn't on Stripe's supported
// country list — so this is the real integration, not a placeholder.

const AUTH_URL = "https://account.bog.ge/auth/realms/bog/protocol/openid-connect/token";
const API_BASE = "https://api.bog.ge/payments/v1";

async function getAccessToken(): Promise<string> {
  const clientId = process.env.BOG_CLIENT_ID;
  const clientSecret = process.env.BOG_CLIENT_SECRET;
  if (!clientId || !clientSecret) {
    throw new Error("BOG_CLIENT_ID / BOG_CLIENT_SECRET are not configured.");
  }

  const credentials = Buffer.from(`${clientId}:${clientSecret}`).toString("base64");
  const res = await fetch(AUTH_URL, {
    method: "POST",
    headers: {
      Authorization: `Basic ${credentials}`,
      "Content-Type": "application/x-www-form-urlencoded",
    },
    body: "grant_type=client_credentials",
    cache: "no-store",
  });

  if (!res.ok) {
    throw new Error(`BOG auth failed: ${res.status} ${await res.text()}`);
  }

  const data = (await res.json()) as { access_token: string };
  return data.access_token;
}

export type BogBasketItem = {
  productId: string;
  quantity: number;
  unitPrice: number;
};

export async function createBogOrder(params: {
  externalOrderId: string;
  totalAmount: number;
  basket: BogBasketItem[];
  callbackUrl: string;
  successUrl: string;
  failUrl: string;
}): Promise<{ orderId: string; redirectUrl: string }> {
  const token = await getAccessToken();

  const res = await fetch(`${API_BASE}/ecommerce/orders`, {
    method: "POST",
    headers: {
      Authorization: `Bearer ${token}`,
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      callback_url: params.callbackUrl,
      external_order_id: params.externalOrderId,
      purchase_units: {
        currency: "GEL",
        total_amount: params.totalAmount,
        basket: params.basket.map((item) => ({
          product_id: item.productId,
          quantity: item.quantity,
          unit_price: item.unitPrice,
        })),
      },
      redirect_urls: {
        success: params.successUrl,
        fail: params.failUrl,
      },
    }),
    cache: "no-store",
  });

  if (!res.ok) {
    throw new Error(`BOG order creation failed: ${res.status} ${await res.text()}`);
  }

  const data = (await res.json()) as { id: string; _links: { redirect: { href: string } } };
  return { orderId: data.id, redirectUrl: data._links.redirect.href };
}

function getPublicKey(): string {
  const raw = process.env.BOG_PUBLIC_KEY ?? "";
  // Most env-var UIs (including Render's) don't preserve literal newlines
  // well in multi-line PEM values, so also accept an escaped `\n` form.
  return raw.includes("\\n") ? raw.replace(/\\n/g, "\n") : raw;
}

// NOTE: BOG's docs confirm SHA256withRSA over the raw request body, signed
// with their private key and checked here against their public key — but
// don't spell out the signature's text encoding. base64 is assumed (the
// common convention for this kind of header); confirm against a real
// callback once sandbox/live credentials are available, and adjust if BOG
// actually sends hex instead.
export function verifyBogCallbackSignature(rawBody: string, signatureHeader: string | null): boolean {
  if (!signatureHeader) return false;
  const publicKey = getPublicKey();
  if (!publicKey) return false;

  try {
    const verifier = crypto.createVerify("RSA-SHA256");
    verifier.update(rawBody, "utf8");
    verifier.end();
    return verifier.verify(publicKey, signatureHeader, "base64");
  } catch {
    return false;
  }
}

// The callback body only confirms *that something happened* for an
// order_id, not the actual payment outcome — so the callback is treated as
// "go check", not as the source of truth. This re-fetches the order's
// authoritative status directly from BOG before confirming anything.
export async function getBogOrderStatus(orderId: string): Promise<string> {
  const token = await getAccessToken();
  const res = await fetch(`${API_BASE}/receipt/${orderId}`, {
    headers: { Authorization: `Bearer ${token}` },
    cache: "no-store",
  });

  if (!res.ok) {
    throw new Error(`BOG order lookup failed: ${res.status} ${await res.text()}`);
  }

  // TODO: confirm the exact field name/path once testing against a real
  // order — docs show "completed" as one possible status value.
  const data = (await res.json()) as Record<string, unknown>;
  return (data.status as string) ?? (data.order_status as string) ?? "";
}
