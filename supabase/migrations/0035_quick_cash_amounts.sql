-- Settings → Checkout: the one-tap cash buttons in the till's payment window, in the
-- business's currency. Was hardcoded to Georgian lari notes. Empty = only "Exact".
alter table business_settings
  add column quick_cash_amounts numeric(12, 2)[] not null default '{5,10,20,50,100,200}'
    check (cardinality(quick_cash_amounts) <= 11 and 0 < all (quick_cash_amounts));
