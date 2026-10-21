-- A ~400px copy of each product photo for cards and thumbnail strips, made on
-- upload (storefront lib/resizeImage.ts, shop-console 2.3.2) next to the
-- original as <name>.thumb.jpg. Null for photos without one: show the original.
alter table product_images add column thumb_path text;

-- storefront_product_page gains a name search. Rebuilt rather than overloaded
-- so PostgREST never has two candidates to choose between.
drop function storefront_product_page(uuid[], int, int);

create function storefront_product_page(
  category_ids uuid[],
  page_size int,
  page_offset int,
  search text default null
)
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
    -- Typed text is matched literally: % and _ are escaped, not wildcards.
    and (
      nullif(btrim(search), '') is null
      or p.name ilike '%' || replace(replace(replace(btrim(search), '\', '\\'), '%', '\%'), '_', '\_') || '%'
    )
  order by s.product_id is not null desc, p.created_at desc, p.id
  limit least(greatest(page_size, 1), 100)
  offset greatest(page_offset, 0)
$$;

revoke execute on function storefront_product_page(uuid[], int, int, text) from public;
grant execute on function storefront_product_page(uuid[], int, int, text) to anon, authenticated;
