-- Per-shade/size stock tracking (cosmetics need this; knives/leather goods
-- generally won't have variants at all, so every change here is additive
-- and defaults to today's exact behavior when no variant is involved).

create table product_variants (
  id uuid primary key default gen_random_uuid(),
  product_id uuid not null references products(id) on delete cascade,
  name text not null, -- the shade/size label, e.g. "Red" — shown as "<product> (<name>)" downstream
  sku text unique,     -- nullable: cosmetics variants often have their own manufacturer barcode, knives/leather don't
  created_at timestamptz not null default now(),
  unique (product_id, name)
);

create index on product_variants (product_id);

alter table product_variants enable row level security;

create policy "read product_variants" on product_variants for select to authenticated using (true);
create policy "write product_variants" on product_variants for all to authenticated using (is_admin()) with check (is_admin());

-- inventory_levels needs to track stock per (product, variant, shop) instead
-- of just (product, shop) once a product has variants, while staying exactly
-- as it is today for the majority of products that never will.
alter table inventory_levels add column variant_id uuid references product_variants(id) on delete cascade;

-- Null-safe uniqueness key: Postgres treats NULL <> NULL, so a naive
-- unique(product_id, variant_id, shop_id) would silently stop enforcing
-- one-row-per-product-per-shop for every non-variant product. Coalescing to
-- a fixed nil UUID makes two "no variant" rows for the same product+shop
-- collide exactly like today, while two different variants' rows stay
-- distinct. Generated so it can't drift from variant_id or be set directly.
alter table inventory_levels add column variant_key uuid
  generated always as (coalesce(variant_id, '00000000-0000-0000-0000-000000000000'::uuid)) stored;

alter table inventory_levels drop constraint if exists inventory_levels_product_id_shop_id_key;
alter table inventory_levels add constraint inventory_levels_product_variant_shop_key
  unique (product_id, variant_key, shop_id);

create index if not exists idx_inventory_levels_variant on inventory_levels (variant_id) where variant_id is not null;

-- sale_items / return_items / stock_transfers: nullable, no cascade — these
-- are append-only ledgers (see 0011_returns.sql's comment), a variant
-- should never be deletable out from under sales/return/transfer history.
alter table sale_items add column variant_id uuid references product_variants(id);
alter table return_items add column variant_id uuid references product_variants(id);
alter table stock_transfers add column variant_id uuid references product_variants(id);

-- record_sale: same signature, p_items gains an optional variant_id key.
-- An old (pre-update) client's payload never has the key at all, which
-- nullif(...,'') treats identically to "key present but null" — a lagging
-- shop machine keeps selling unmodified until it self-updates. Same
-- technique 0013_sale_item_discounts.sql already used for list_price.
create or replace function record_sale(
  p_shop_id uuid,
  p_client_generated_id uuid,
  p_items jsonb, -- array of {product_id, variant_id, quantity, unit_price, unit_cost, list_price}
  p_total numeric
)
returns uuid
language plpgsql
as $$
declare
  v_sale_id uuid;
  v_item jsonb;
  v_variant_id uuid;
begin
  insert into sales (shop_id, staff_id, client_generated_id, total)
  values (p_shop_id, auth.uid(), p_client_generated_id, p_total)
  on conflict (client_generated_id) do nothing
  returning id into v_sale_id;

  if v_sale_id is null then
    select id into v_sale_id from sales where client_generated_id = p_client_generated_id;
    return v_sale_id;
  end if;

  for v_item in select * from jsonb_array_elements(p_items) loop
    v_variant_id := nullif(v_item ->> 'variant_id', '')::uuid;

    insert into sale_items (sale_id, product_id, variant_id, quantity, unit_price, unit_cost, list_price)
    values (
      v_sale_id,
      (v_item ->> 'product_id')::uuid,
      v_variant_id,
      (v_item ->> 'quantity')::int,
      (v_item ->> 'unit_price')::numeric,
      (v_item ->> 'unit_cost')::numeric,
      coalesce((v_item ->> 'list_price')::numeric, (v_item ->> 'unit_price')::numeric)
    );

    update inventory_levels
    set quantity = quantity - (v_item ->> 'quantity')::int, updated_at = now()
    where product_id = (v_item ->> 'product_id')::uuid
      and shop_id = p_shop_id
      and variant_key = coalesce(v_variant_id, '00000000-0000-0000-0000-000000000000'::uuid);
  end loop;

  return v_sale_id;
end;
$$;

-- record_return: variant is resolved server-side from the original
-- sale_items row (same trust model already used for unit_price) — p_items
-- shape ({sale_item_id, quantity}) is completely unchanged.
create or replace function record_return(
  p_shop_id uuid,
  p_client_generated_id uuid,
  p_sale_id uuid,
  p_reason text,
  p_items jsonb
)
returns uuid
language plpgsql
as $$
declare
  v_return_id uuid;
  v_item jsonb;
  v_sale_item_id uuid;
  v_quantity int;
  v_unit_price numeric;
  v_product_id uuid;
  v_variant_id uuid;
  v_already_returned int;
  v_original_quantity int;
  v_total numeric := 0;
begin
  insert into returns (sale_id, shop_id, staff_id, client_generated_id, reason, total)
  values (p_sale_id, p_shop_id, auth.uid(), p_client_generated_id, p_reason, 0)
  on conflict (client_generated_id) do nothing
  returning id into v_return_id;

  if v_return_id is null then
    select id into v_return_id from returns where client_generated_id = p_client_generated_id;
    return v_return_id;
  end if;

  for v_item in select * from jsonb_array_elements(p_items) loop
    v_sale_item_id := (v_item ->> 'sale_item_id')::uuid;
    v_quantity := (v_item ->> 'quantity')::int;

    select unit_price, product_id, variant_id, quantity
      into v_unit_price, v_product_id, v_variant_id, v_original_quantity
    from sale_items where id = v_sale_item_id and sale_id = p_sale_id;

    if v_unit_price is null then
      raise exception 'sale_item % does not belong to sale %', v_sale_item_id, p_sale_id;
    end if;

    select coalesce(sum(quantity), 0) into v_already_returned
    from return_items where sale_item_id = v_sale_item_id;

    if v_already_returned + v_quantity > v_original_quantity then
      raise exception 'Cannot return % of this item — only % left un-returned', v_quantity,
        v_original_quantity - v_already_returned;
    end if;

    insert into return_items (return_id, sale_item_id, product_id, variant_id, quantity, unit_price)
    values (v_return_id, v_sale_item_id, v_product_id, v_variant_id, v_quantity, v_unit_price);

    update inventory_levels
    set quantity = quantity + v_quantity, updated_at = now()
    where product_id = v_product_id
      and shop_id = p_shop_id
      and variant_key = coalesce(v_variant_id, '00000000-0000-0000-0000-000000000000'::uuid);

    v_total := v_total + (v_unit_price * v_quantity);
  end loop;

  update returns set total = v_total where id = v_return_id;
  return v_return_id;
end;
$$;

-- transfer_stock: new trailing, defaulted parameter — create or replace
-- allows appending a defaulted parameter to an existing function's argument
-- list without creating an ambiguous overload, so old and new clients both
-- keep working against the same function.
create or replace function transfer_stock(
  p_client_generated_id uuid,
  p_product_id uuid,
  p_from_shop_id uuid,
  p_to_shop_id uuid,
  p_quantity int,
  p_variant_id uuid default null
)
returns uuid
language plpgsql
as $$
declare
  v_transfer_id uuid;
  v_variant_key uuid := coalesce(p_variant_id, '00000000-0000-0000-0000-000000000000'::uuid);
begin
  if p_from_shop_id = p_to_shop_id then
    raise exception 'Cannot transfer stock to the same shop.';
  end if;
  if p_quantity <= 0 then
    raise exception 'Quantity must be greater than zero.';
  end if;

  insert into stock_transfers (client_generated_id, product_id, variant_id, from_shop_id, to_shop_id, quantity, staff_id)
  values (p_client_generated_id, p_product_id, p_variant_id, p_from_shop_id, p_to_shop_id, p_quantity, auth.uid())
  on conflict (client_generated_id) do nothing
  returning id into v_transfer_id;

  if v_transfer_id is null then
    select id into v_transfer_id from stock_transfers where client_generated_id = p_client_generated_id;
    return v_transfer_id;
  end if;

  update inventory_levels
  set quantity = quantity - p_quantity, updated_at = now()
  where product_id = p_product_id and shop_id = p_from_shop_id and variant_key = v_variant_key and quantity >= p_quantity;

  if not found then
    raise exception 'Not enough stock at the source shop to transfer % unit(s).', p_quantity;
  end if;

  insert into inventory_levels (product_id, variant_id, shop_id, quantity)
  values (p_product_id, p_variant_id, p_to_shop_id, p_quantity)
  on conflict (product_id, variant_key, shop_id)
  do update set quantity = inventory_levels.quantity + p_quantity, updated_at = now();

  return v_transfer_id;
end;
$$;

grant execute on function transfer_stock(uuid, uuid, uuid, uuid, int, uuid) to authenticated;

-- IMPORTANT DEPLOY NOTE: upsertInventoryLevels (SupabaseClient.cpp) is a
-- direct PostgREST table upsert against inventory_levels using
-- on_conflict=product_id,shop_id. That target no longer matches any
-- constraint once this migration lands (see variant_key above) — every
-- product's stock edit from InventoryPage/ProductDialog will 400 on an
-- un-updated client, not just variant ones. That call site is exclusively
-- admin tooling (never the POS), so apply this migration and rebuild/
-- redeploy the admin machine's Shop Console together rather than relying on
-- the self-updater's normal lagged timing.
