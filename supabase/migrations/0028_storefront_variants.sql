-- Variants on the website: customers pick a shade/size, photos can belong to one,
-- and online orders remember which one was bought.

-- Visitors need the variant names to choose from.
create policy "public read product_variants" on product_variants for select to anon using (true);

-- A photo of one particular variant (null = the product in general).
alter table product_images add column variant_id uuid references product_variants(id) on delete set null;

alter table order_items add column variant_id uuid references product_variants(id);

-- Same idea as in_stock_product_ids(): which variants can be bought, without the counts.
create function in_stock_variant_ids()
returns setof uuid
language sql
stable
security definer
set search_path = public
as $$
  select variant_id from inventory_levels where variant_id is not null group by variant_id having sum(quantity) > 0
$$;

revoke execute on function in_stock_variant_ids() from public;
grant execute on function in_stock_variant_ids() to anon, authenticated;

-- Each order line now takes stock from its own variant. Before, a variant product's
-- line took the quantity off every one of its variants at that shop.
create or replace function confirm_online_order(p_order_id uuid)
returns void
language plpgsql
set search_path = public
as $$
declare
  v_item record;
  v_shop_id uuid;
  v_status order_status;
begin
  select status, pickup_shop_id into v_status, v_shop_id from orders where id = p_order_id;

  if v_status is null then
    raise exception 'Order % not found', p_order_id;
  end if;

  if v_status <> 'pending' then
    return; -- already processed
  end if;

  update orders set status = 'paid', updated_at = now() where id = p_order_id;

  for v_item in select product_id, variant_id, quantity from order_items where order_id = p_order_id loop
    update inventory_levels
    set quantity = quantity - v_item.quantity, updated_at = now()
    where product_id = v_item.product_id and shop_id = v_shop_id
      and variant_id is not distinct from v_item.variant_id;
  end loop;
end;
$$;

-- Adds a variant to an existing product. The first variant takes over the stock the
-- product had on its own, since a product with variants only counts stock per variant.
-- Runs as the caller, so the admin-only RLS on both tables still applies.
create function add_product_variant(p_product_id uuid, p_name text, p_sku text default null)
returns uuid
language plpgsql
set search_path = public
as $$
declare
  v_id uuid;
  v_first boolean;
begin
  v_first := not exists (select 1 from product_variants where product_id = p_product_id);

  insert into product_variants (product_id, name, sku)
  values (p_product_id, trim(p_name), nullif(trim(p_sku), ''))
  returning id into v_id;

  if v_first then
    update inventory_levels set variant_id = v_id, updated_at = now()
    where product_id = p_product_id and variant_id is null;
  end if;

  return v_id;
end;
$$;

revoke execute on function add_product_variant(uuid, text, text) from public, anon;
grant execute on function add_product_variant(uuid, text, text) to authenticated;
