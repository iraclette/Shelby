-- Replaces the client-orchestrated sale+items+decrement sequence with one
-- atomic call, so a sale is either fully recorded or not recorded at all.
-- Also makes offline retries safe: client_generated_id is unique, so
-- resubmitting the same locally-queued sale after a successful-but-
-- unconfirmed sync just returns the existing sale instead of duplicating it.
create or replace function record_sale(
  p_shop_id uuid,
  p_client_generated_id uuid,
  p_items jsonb, -- array of {product_id, quantity, unit_price, unit_cost}
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
    insert into sale_items (sale_id, product_id, quantity, unit_price, unit_cost)
    values (
      v_sale_id,
      (v_item ->> 'product_id')::uuid,
      (v_item ->> 'quantity')::int,
      (v_item ->> 'unit_price')::numeric,
      (v_item ->> 'unit_cost')::numeric
    );

    update inventory_levels
    set quantity = quantity - (v_item ->> 'quantity')::int, updated_at = now()
    where product_id = (v_item ->> 'product_id')::uuid and shop_id = p_shop_id;
  end loop;

  return v_sale_id;
end;
$$;

grant execute on function record_sale(uuid, uuid, jsonb, numeric) to authenticated;
