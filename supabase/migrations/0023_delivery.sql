-- Website orders can now be shipped with Inex instead of picked up in Gori.
create type delivery_method as enum ('pickup', 'inex_branch', 'inex_home');

alter table orders
  add column delivery_method delivery_method not null default 'pickup',
  add column delivery_fee numeric(12, 2) not null default 0, -- already included in total
  add column delivery_city text,
  -- Home delivery: the street address. Branch delivery: the customer's preferred Inex branch, if any.
  add column delivery_address text,
  add constraint orders_delivery_city_required check (delivery_method = 'pickup' or delivery_city is not null),
  add constraint orders_home_address_required check (delivery_method <> 'inex_home' or delivery_address is not null);

-- Unchanged column, wider meaning: confirm_online_order() takes stock from this shop either way.
comment on column orders.pickup_shop_id is
  'Pickup orders: where the customer collects. Delivery orders: the shop that ships it (chosen at checkout by stock).';
