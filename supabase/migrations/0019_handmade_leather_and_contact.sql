-- Handmade Leather custom order requests + a Contact Us inbox, both surfaced
-- only in the Shop Console admin — no email dependency for leather orders
-- (a lost custom-order spec is a lost sale), best-effort email on top of the
-- DB write for contact messages (see apps/storefront/src/lib/email.ts).

-- General mechanism, not hardcoded to leather: lets admin hide any category
-- (and its storefront surface) before it's ready.
alter table categories add column visible_on_storefront boolean not null default true;

create type custom_order_status as enum ('new', 'reviewed', 'quoted', 'in_progress', 'completed', 'cancelled');

create table custom_leather_orders (
  id uuid primary key default gen_random_uuid(),
  customer_name text not null,
  customer_email text not null,
  customer_phone text,
  preferred_shop_id uuid references shops(id),
  item_description text not null,
  status custom_order_status not null default 'new',
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create type contact_message_status as enum ('unread', 'read');

create table contact_messages (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  email text not null,
  message text not null,
  status contact_message_status not null default 'unread',
  email_sent boolean not null default false,
  created_at timestamptz not null default now()
);

alter table custom_leather_orders enable row level security;
alter table contact_messages enable row level security;

-- Mirrors salary_payments (0014): admin-only, no anon/authenticated-staff
-- policy — inserts happen exclusively through the storefront's service-role
-- API routes, same pattern as `orders` (0007).
create policy "admins read custom_leather_orders" on custom_leather_orders for select to authenticated using (is_admin());
create policy "admins update custom_leather_orders" on custom_leather_orders for update to authenticated using (is_admin()) with check (is_admin());

create policy "admins read contact_messages" on contact_messages for select to authenticated using (is_admin());
create policy "admins update contact_messages" on contact_messages for update to authenticated using (is_admin()) with check (is_admin());
