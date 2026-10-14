-- Online storefront orders (pickup only for now).

create type order_status as enum ('pending', 'paid', 'failed', 'fulfilled', 'cancelled');

create table orders (
  id uuid primary key default gen_random_uuid(),
  customer_name text not null,
  customer_email text not null,
  customer_phone text,
  pickup_shop_id uuid not null references shops(id),
  status order_status not null default 'pending',
  total numeric(12, 2) not null,
  bog_order_id text,
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

create table order_items (
  id uuid primary key default gen_random_uuid(),
  order_id uuid not null references orders(id) on delete cascade,
  product_id uuid not null references products(id),
  quantity integer not null check (quantity > 0),
  unit_price numeric(12, 2) not null -- snapshotted server-side at checkout, never trusted from the client
);

create index on order_items (order_id);

-- Checkout needs the shop list for pickup.
create policy "public read shops" on shops for select to anon using (true);

-- RLS on with no policies: only the service role (API routes) can touch orders.
alter table orders enable row level security;
alter table order_items enable row level security;

-- Called by the storefront's payment webhook once BOG confirms a payment.
-- Idempotent (safe if the webhook fires more than once for the same order).
create or replace function confirm_online_order(p_order_id uuid)
returns void
language plpgsql
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

  for v_item in select product_id, quantity from order_items where order_id = p_order_id loop
    update inventory_levels
    set quantity = quantity - v_item.quantity, updated_at = now()
    where product_id = v_item.product_id and shop_id = v_shop_id;
  end loop;
end;
$$;
