-- Phase 0 schema: shops, catalog, inventory, sales, staff profiles.
-- Run this in the Supabase SQL editor (or via `supabase db push`) on a fresh project.

create extension if not exists pgcrypto;

-- ---------------------------------------------------------------------------
-- Core tables
-- ---------------------------------------------------------------------------

create table shops (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  address text,
  created_at timestamptz not null default now()
);

create table categories (
  id uuid primary key default gen_random_uuid(),
  name text not null unique,
  created_at timestamptz not null default now()
);

create table products (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  description text,
  category_id uuid references categories(id) on delete set null,
  cost_price numeric(12, 2) not null default 0,
  sell_price numeric(12, 2) not null default 0,
  sku text not null unique,
  has_native_barcode boolean not null default false,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create table product_images (
  id uuid primary key default gen_random_uuid(),
  product_id uuid not null references products(id) on delete cascade,
  storage_path text not null,
  is_primary boolean not null default false,
  created_at timestamptz not null default now()
);

create table inventory_levels (
  id uuid primary key default gen_random_uuid(),
  product_id uuid not null references products(id) on delete cascade,
  shop_id uuid not null references shops(id) on delete cascade,
  quantity integer not null default 0,
  updated_at timestamptz not null default now(),
  unique (product_id, shop_id)
);

-- profiles.id is the same id as auth.users.id (one row per Supabase Auth user).
create table profiles (
  id uuid primary key references auth.users(id) on delete cascade,
  full_name text,
  role text not null default 'staff' check (role in ('owner', 'admin', 'staff')),
  shop_id uuid references shops(id) on delete set null,
  created_at timestamptz not null default now()
);

create table sales (
  id uuid primary key default gen_random_uuid(),
  -- set by the POS client when the sale is made offline, so a later sync
  -- can't insert the same sale twice.
  client_generated_id uuid not null unique,
  shop_id uuid not null references shops(id),
  staff_id uuid not null references profiles(id),
  sold_at timestamptz not null default now(),
  total numeric(12, 2) not null,
  created_at timestamptz not null default now()
);

create table sale_items (
  id uuid primary key default gen_random_uuid(),
  sale_id uuid not null references sales(id) on delete cascade,
  product_id uuid not null references products(id),
  quantity integer not null check (quantity > 0),
  unit_price numeric(12, 2) not null,
  unit_cost numeric(12, 2) not null -- snapshot of cost_price at sale time, so profit is stable even if cost changes later
);

create index on products (category_id);
create index on product_images (product_id);
create index on inventory_levels (shop_id);
create index on sales (shop_id, sold_at);
create index on sale_items (sale_id);

-- ---------------------------------------------------------------------------
-- Role helpers (used by RLS policies below)
-- ---------------------------------------------------------------------------

create function current_profile_role() returns text
  language sql stable security definer set search_path = public as $$
  select role from profiles where id = auth.uid();
$$;

create function current_profile_shop_id() returns uuid
  language sql stable security definer set search_path = public as $$
  select shop_id from profiles where id = auth.uid();
$$;

create function is_admin() returns boolean
  language sql stable security definer set search_path = public as $$
  select coalesce(current_profile_role() in ('owner', 'admin'), false);
$$;

-- ---------------------------------------------------------------------------
-- Row Level Security
-- ---------------------------------------------------------------------------

alter table shops enable row level security;
alter table categories enable row level security;
alter table products enable row level security;
alter table product_images enable row level security;
alter table inventory_levels enable row level security;
alter table profiles enable row level security;
alter table sales enable row level security;
alter table sale_items enable row level security;

-- shops / categories / products / product_images: every logged-in staff member
-- can read the catalog (needed for POS lookup); only admins/owners can write it.
create policy "read catalog" on shops for select to authenticated using (true);
create policy "write catalog" on shops for all to authenticated using (is_admin()) with check (is_admin());

create policy "read categories" on categories for select to authenticated using (true);
create policy "write categories" on categories for all to authenticated using (is_admin()) with check (is_admin());

create policy "read products" on products for select to authenticated using (true);
create policy "write products" on products for all to authenticated using (is_admin()) with check (is_admin());

create policy "read product_images" on product_images for select to authenticated using (true);
create policy "write product_images" on product_images for all to authenticated using (is_admin()) with check (is_admin());

-- inventory_levels: staff can see/update stock for their own shop; admins/owners see and edit all shops.
create policy "read own shop inventory" on inventory_levels for select to authenticated
  using (is_admin() or shop_id = current_profile_shop_id());
create policy "write own shop inventory" on inventory_levels for all to authenticated
  using (is_admin() or shop_id = current_profile_shop_id())
  with check (is_admin() or shop_id = current_profile_shop_id());

-- profiles: everyone can read their own profile; admins/owners can read and manage all profiles.
create policy "read own profile" on profiles for select to authenticated
  using (id = auth.uid() or is_admin());
create policy "admins manage profiles" on profiles for all to authenticated
  using (is_admin()) with check (is_admin());

-- sales / sale_items: staff can create and read sales for their own shop; admins/owners see everything.
create policy "read own shop sales" on sales for select to authenticated
  using (is_admin() or shop_id = current_profile_shop_id());
create policy "create own shop sales" on sales for insert to authenticated
  with check (is_admin() or shop_id = current_profile_shop_id());

create policy "read own shop sale_items" on sale_items for select to authenticated
  using (is_admin() or exists (
    select 1 from sales where sales.id = sale_items.sale_id and sales.shop_id = current_profile_shop_id()
  ));
create policy "create own shop sale_items" on sale_items for insert to authenticated
  with check (is_admin() or exists (
    select 1 from sales where sales.id = sale_items.sale_id and sales.shop_id = current_profile_shop_id()
  ));

-- New Supabase Auth users get a profile row automatically (role defaults to
-- 'staff' with no shop assigned; an admin promotes/assigns them afterward).
create function handle_new_user() returns trigger
  language plpgsql security definer set search_path = public as $$
begin
  insert into public.profiles (id, full_name) values (new.id, new.raw_user_meta_data ->> 'full_name');
  return new;
end;
$$;

create trigger on_auth_user_created
  after insert on auth.users
  for each row execute function handle_new_user();
