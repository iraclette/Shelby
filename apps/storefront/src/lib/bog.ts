import "server-only";
import crypto from "node:crypto";

// Bank of Georgia Online Payment API: https://api.bog.ge/docs/en/payments/introduction

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

// TODO: signature encoding assumed base64; confirm against a real BOG callback.
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

// The callback isn't trusted; this fetches the real status from BOG.
export async function getBogOrderStatus(orderId: string): Promise<string> {
  const token = await getAccessToken();
  const res = await fetch(`${API_BASE}/receipt/${orderId}`, {
    headers: { Authorization: `Bearer ${token}` },
    cache: "no-store",
  });

  if (!res.ok) {
    throw new Error(`BOG order lookup failed: ${res.status} ${await res.text()}`);
  }

  // Shape per BOG's Get Payment Details docs: order_status: { key: "completed", value: "<Georgian label>" }.
  // Keys: created, processing, completed, rejected, refund_requested, refunded, ...
  const data = (await res.json()) as { order_status?: { key?: string } };
  return data.order_status?.key ?? "";
}
