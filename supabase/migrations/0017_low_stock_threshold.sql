-- Per product, across all shops. Null/0 = disabled.

alter table products add column low_stock_threshold integer;
