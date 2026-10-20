-- Settings → Checkout: what the till offers, and the defaults for new products. Shared by
-- every till, like the rest of business_settings.
alter table business_settings
  -- The payment methods the till shows, at least one.
  add column payment_methods text[] not null default '{cash,card}'
    check (payment_methods <@ array['cash', 'card'] and cardinality(payment_methods) > 0),
  -- The one already selected when the payment window opens.
  add column default_payment_method text not null default 'cash'
    check (default_payment_method in ('cash', 'card')),
  -- Low stock alert for new products; 0 = off. Each product can still change its own.
  add column default_low_stock_threshold integer not null default 0
    check (default_low_stock_threshold >= 0),
  -- Codes made for products without a barcode: <prefix>-XXXXXX. Existing codes keep theirs.
  add column sku_prefix text not null default 'INT'
    check (sku_prefix ~ '^[A-Z0-9]{1,6}$');

-- The console fills the alert in itself; this covers products added from the website, which
-- leave it out.
create function products_default_low_stock() returns trigger
language plpgsql
set search_path = public
as $$
begin
  if new.low_stock_threshold is null then
    select nullif(default_low_stock_threshold, 0) into new.low_stock_threshold from business_settings;
  end if;
  return new;
end;
$$;

create trigger products_default_low_stock before insert on products
  for each row execute function products_default_low_stock();
