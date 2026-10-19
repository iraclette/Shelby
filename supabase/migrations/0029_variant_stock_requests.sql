-- "Let me know when it's back" for one shade/size of a product.
alter table stock_requests add column variant_id uuid references product_variants(id) on delete cascade;

-- One open request per contact for each variant (or for the product, when it has none).
drop index stock_requests_open_unique;
create unique index stock_requests_open_unique on stock_requests
  (product_id, coalesce(variant_id, '00000000-0000-0000-0000-000000000000'::uuid), lower(contact))
  where status = 'new';
