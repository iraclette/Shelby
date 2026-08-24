-- In-person POS returns (against sales/sale_items). Deliberately not
-- touching online orders/BOG — that's a real payment refund, a separate
-- undertaking once BOG is actually live.
--
-- Same append-only philosophy as sales/sale_items: a return never edits or
-- deletes the original sale, it's a new ledger entry that reverses its
-- effects (restores stock, records what was refunded).

create table returns (
  id uuid primary key default gen_random_uuid(),
  client_generated_id uuid not null unique,
  sale_id uuid not null references sales(id),
  shop_id uuid not null references shops(id),
  staff_id uuid not null references profiles(id),
  reason text,
  total numeric(12, 2) not null,
  created_at timestamptz not null default now()
);

create table return_items (
  id uuid primary key default gen_random_uuid(),
  return_id uuid not null references returns(id) on delete cascade,
  sale_item_id uuid not null references sale_items(id),
  product_id uuid not null references products(id),
  quantity integer not null check (quantity > 0),
  unit_price numeric(12, 2) not null -- copied from the sale_item, not re-trusted from the client
);

create index on returns (shop_id);
create index on return_items (return_id);
create index on return_items (sale_item_id);

alter table returns enable row level security;
alter table return_items enable row level security;

-- Mirrors "read/create own shop sales" from 0001_init.sql exactly.
create policy "read own shop returns" on returns for select to authenticated
  using (is_admin() or shop_id = current_profile_shop_id());
create policy "create own shop returns" on returns for insert to authenticated
  with check (is_admin() or shop_id = current_profile_shop_id());

create policy "read own shop return_items" on return_items for select to authenticated
  using (is_admin() or exists (
    select 1 from returns where returns.id = return_items.return_id and returns.shop_id = current_profile_shop_id()
  ));
create policy "create own shop return_items" on return_items for insert to authenticated
  with check (is_admin() or exists (
    select 1 from returns where returns.id = return_items.return_id and returns.shop_id = current_profile_shop_id()
  ));

-- No SECURITY DEFINER — same as record_sale/decrement_inventory, runs as
-- the calling staff member so the RLS policies above still apply; staff_id
-- comes from auth.uid(), never a client-supplied parameter.
create function record_return(
  p_shop_id uuid,
  p_client_generated_id uuid,
  p_sale_id uuid,
  p_reason text,
  p_items jsonb -- array of {sale_item_id, quantity}
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

    select unit_price, product_id, quantity into v_unit_price, v_product_id, v_original_quantity
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

    insert into return_items (return_id, sale_item_id, product_id, quantity, unit_price)
    values (v_return_id, v_sale_item_id, v_product_id, v_quantity, v_unit_price);

    update inventory_levels
    set quantity = quantity + v_quantity, updated_at = now()
    where product_id = v_product_id and shop_id = p_shop_id;

    v_total := v_total + (v_unit_price * v_quantity);
  end loop;

  update returns set total = v_total where id = v_return_id;

  return v_return_id;
end;
$$;

grant execute on function record_return(uuid, uuid, uuid, text, jsonb) to authenticated;
