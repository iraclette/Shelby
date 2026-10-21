-- One page of the storefront catalog, so /products never loads every product.
-- Same rules the storefront applied in JS: hidden categories are left out
-- (uncategorised products always show), in-stock products come first, newest
-- first within that. Security definer for inventory_levels, which anon can't
-- read; like in_stock_product_ids() it only exposes ids and an in-stock flag.
create function storefront_product_page(category_ids uuid[], page_size int, page_offset int)
returns table (product_id uuid, in_stock boolean, total bigint)
language sql
stable
security definer
set search_path = public
as $$
  with stock as (
    select product_id from inventory_levels group by product_id having sum(quantity) > 0
  )
  select p.id, s.product_id is not null, count(*) over ()
  from products p
  left join categories c on c.id = p.category_id
  left join stock s on s.product_id = p.id
  where coalesce(c.visible_on_storefront, true)
    and (category_ids is null or p.category_id = any(category_ids))
  order by s.product_id is not null desc, p.created_at desc, p.id
  limit least(greatest(page_size, 1), 100)
  offset greatest(page_offset, 0)
$$;

revoke execute on function storefront_product_page(uuid[], int, int) from public;
grant execute on function storefront_product_page(uuid[], int, int) to anon, authenticated;
