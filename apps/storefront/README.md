# Shelby Storefront

The customer-facing website for Shelby's shops, at [shelby.ge](https://shelby.ge), plus a few phone-friendly pages for staff. Built with Next.js (App Router), React and Tailwind CSS, on the same Supabase backend as the Shop Console.

## Running it locally

Requires Node.js 20 or later.

```bash
npm install
# create .env.local with the values below
npm run dev
```

The site runs at [http://localhost:3000](http://localhost:3000). Other scripts:

| Command | What it does |
|---|---|
| `npm run build` | Production build |
| `npm start` | Serves the production build |
| `npm run lint` | ESLint |

## Configuration

All settings are environment variables, set in `.env.local` locally and in the host's settings in production. Only the two Supabase public values are needed to browse the site locally. Everything else switches on one feature and is skipped while unset.

| Variables | Used for |
|---|---|
| `NEXT_PUBLIC_SUPABASE_URL`, `NEXT_PUBLIC_SUPABASE_ANON_KEY` | Reading the catalog, customer and staff sign-in |
| `SUPABASE_SECRET_KEY` | Server-only API routes that write orders and form submissions. Never expose it to the browser |
| `BOG_CLIENT_ID`, `BOG_CLIENT_SECRET`, `BOG_PUBLIC_KEY` | Bank of Georgia card payments and their callback |
| `RESEND_API_KEY`, `CONTACT_EMAIL` | Emailing the business about contact form messages |
| `NEXT_PUBLIC_TURNSTILE_SITE_KEY`, `TURNSTILE_SECRET_KEY` | Cloudflare Turnstile CAPTCHA on sign-in and sign-up |
| `TELEGRAM_BOT_TOKEN`, `NEXT_PUBLIC_TELEGRAM_BOT_USERNAME`, `TELEGRAM_WEBHOOK_SECRET` | Telegram alerts for owners |

Business details shown on the policy pages, delivery prices and the online ordering switch live in `src/lib/business.ts`. With `ONLINE_ORDERING` off, the site works as a catalogue: there's no cart, and product pages point visitors to the shops to order.

## Languages

Every page is served in English, Georgian and Russian under `/en`, `/ka` and `/ru`. Visitors get their browser's language when the site has it, English otherwise, and the language switcher remembers their choice. The text lives in `src/i18n/dictionaries/`. Georgian wording follows `src/i18n/GLOSSARY.md`.

## Layout

| Path | Contents |
|---|---|
| `src/app/[lang]/` | Pages: catalog, product pages, cart and checkout, account, contact, handmade leather, jobs, delivery and policy pages |
| `src/app/[lang]/staff/` | Staff pages: product photos, new product, stock lookup, Telegram notifications |
| `src/app/api/` | Server routes: checkout and the Bank of Georgia webhook, form submissions (contact, custom orders, jobs, stock requests), the Telegram webhook, CAPTCHA verification |
| `src/lib/` | Supabase clients, payments, email, Telegram, validation and rate limiting |
| `src/components/` | Shared UI |

The database schema and access rules are in `supabase/migrations/` at the repository root. Every table is protected by row-level security, and the secret key is used only in server routes that need to write past it.
