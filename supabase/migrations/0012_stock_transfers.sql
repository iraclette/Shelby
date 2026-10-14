-- Moving stock between shops (admin only).

create table stock_transfers (
  id uuid primary key default gen_random_uuid(),
  client_generated_id uuid not null unique,
  product_id uuid not null references products(id),
  from_shop_id uuid not null references shops(id),
  to_shop_id uuid not null references shops(id),
  quantity integer not null check (quantity > 0),
  staff_id uuid not null references profiles(id),
  created_at timestamptz not null default now(),
  check (from_shop_id <> to_shop_id)
);

create index on stock_transfers (product_id);
create index on stock_transfers (from_shop_id);
create index on stock_transfers (to_shop_id);

alter table stock_transfers enable row level security;

create policy "admins manage stock_transfers" on stock_transfers for all to authenticated
  using (is_admin()) with check (is_admin());

-- No SECURITY DEFINER; RLS already limits this to admins.
create or replace function transfer_stock(
  p_client_generated_id uuid,
  p_product_id uuid,
  p_from_shop_id uuid,
  p_to_shop_id uuid,
  p_quantity int
)
returns uuid
language plpgsql
as $$
declare
  v_transfer_id uuid;
begin
  if p_from_shop_id = p_to_shop_id then
    raise exception 'Cannot transfer stock to the same shop.';
  end if;
  if p_quantity <= 0 then
    raise exception 'Quantity must be greater than zero.';
  end if;

  insert into stock_transfers (client_generated_id, product_id, from_shop_id, to_shop_id, quantity, staff_id)
  values (p_client_generated_id, p_product_id, p_from_shop_id, p_to_shop_id, p_quantity, auth.uid())
  on conflict (client_generated_id) do nothing
  returning id into v_transfer_id;

  if v_transfer_id is null then
    select id into v_transfer_id from stock_transfers where client_generated_id = p_client_generated_id;
    return v_transfer_id;
  end if;

  update inventory_levels
  set quantity = quantity - p_quantity, updated_at = now()
  where product_id = p_product_id and shop_id = p_from_shop_id and quantity >= p_quantity;

  if not found then
    raise exception 'Not enough stock at the source shop to transfer % unit(s).', p_quantity;
  end if;

  insert into inventory_levels (product_id, shop_id, quantity)
  values (p_product_id, p_to_shop_id, p_quantity)
  on conflict (product_id, shop_id)
  do update set quantity = inventory_levels.quantity + p_quantity, updated_at = now();

  return v_transfer_id;
end;
$$;

grant execute on function transfer_stock(uuid, uuid, uuid, uuid, int) to authenticated;
