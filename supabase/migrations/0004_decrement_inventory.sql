-- Plain UPDATE is atomic; no SECURITY DEFINER so RLS still applies.
create function decrement_inventory(p_product_id uuid, p_shop_id uuid, p_quantity int)
returns void
language sql
as $$
  update inventory_levels
  set quantity = quantity - p_quantity, updated_at = now()
  where product_id = p_product_id and shop_id = p_shop_id;
$$;

grant execute on function decrement_inventory(uuid, uuid, int) to authenticated;
