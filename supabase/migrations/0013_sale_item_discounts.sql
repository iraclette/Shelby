-- Lets the POS record a line at less than the product's current sell_price
-- (shops routinely negotiate down) while still knowing how much was given
-- away: list_price snapshots the price the cashier started from, the same
-- way unit_cost already snapshots cost_price, so it stays meaningful even
-- if sell_price changes later.

alter table sale_items add column list_price numeric(12, 2);

-- Backfill existing rows so the column can be made not-null: without a
-- record of what the discount actually was, the least wrong assumption is
-- that the row wasn't discounted, i.e. list_price equaled unit_price.
update sale_items set list_price = unit_price where list_price is null;

alter table sale_items alter column list_price set not null;

create or replace function record_sale(
  p_shop_id uuid,
  p_client_generated_id uuid,
  p_items jsonb, -- array of {product_id, quantity, unit_price, unit_cost, list_price}
  p_total numeric
)
returns uuid
language plpgsql
as $$
declare
  v_sale_id uuid;
  v_item jsonb;
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
    insert into sale_items (sale_id, product_id, quantity, unit_price, unit_cost, list_price)
    values (
      v_sale_id,
      (v_item ->> 'product_id')::uuid,
      (v_item ->> 'quantity')::int,
      (v_item ->> 'unit_price')::numeric,
      (v_item ->> 'unit_cost')::numeric,
      -- Older queued offline sales built before this column existed won't
      -- carry list_price; fall back to unit_price rather than reject them.
      coalesce((v_item ->> 'list_price')::numeric, (v_item ->> 'unit_price')::numeric)
    );

    update inventory_levels
    set quantity = quantity - (v_item ->> 'quantity')::int, updated_at = now()
    where product_id = (v_item ->> 'product_id')::uuid and shop_id = p_shop_id;
  end loop;

  return v_sale_id;
end;
$$;
