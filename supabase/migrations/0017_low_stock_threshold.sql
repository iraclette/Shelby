-- One threshold per product, applies uniformly across every shop's quantity
-- for that product. Null/0 = disabled (0 is never a meaningful alert
-- threshold anyway, since quantity can't go negative).

alter table products add column low_stock_threshold integer;
