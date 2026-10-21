-- Bulk product import from the Shop Console (Inventory → Import…, 2.4.0).
-- The console reads the spreadsheet, checks it and shows a preview; this
-- applies it in one transaction, so a bad row means nothing is saved at all.
--
-- items: one object per product, grouped and resolved by the console:
--   { "sku": "...", "native_barcode": true,         -- false: an internal code
--     "name": "...", "category": "Parent / Child", "supplier": "...",
--     "cost_price": 12.5, "sell_price": 25, "low_stock_threshold": 3,
--     "descriptions": { "ka": "...", "en": "...", "ru": "..." },
--     "stock": { "<shop id>": 4, ... },              -- the product's own stock
--     "variants": [ { "name": "Red", "sku": "...", "stock": { "<shop id>": 2 } } ] }
-- A key that's missing or null leaves that field as it is. Nothing is ever
-- deleted. Products are matched by sku, variants by name within the product.
--
-- Returns { created, updated, variants_created, translate: [product ids whose
-- descriptions changed] } for the console to show and to translate.
create function import_products(items jsonb)
returns jsonb
language plpgsql
security invoker -- the usual admin-only write policies apply
set search_path = public
as $$
declare
  item jsonb;
  variant jsonb;
  shop text;
  v_product_id uuid;
  v_variant_id uuid;
  v_category_id uuid;
  v_supplier_id uuid;
  v_existing jsonb;
  v_descriptions jsonb;
  v_changed text[];
  v_lang text;
  v_part text;
  v_parent uuid;
  v_created int := 0;
  v_updated int := 0;
  v_variants_created int := 0;
  v_translate uuid[] := '{}';
begin
  if not is_admin() then
    raise exception 'Only owners and admins can import products.';
  end if;

  for item in select * from jsonb_array_elements(items) loop
    -- "Parent / Child" category paths; missing ones are created (the preview listed
    -- them). Category names are unique shop-wide, so an existing one is matched by
    -- name alone, wherever it sits.
    v_category_id := null;
    if nullif(trim(item ->> 'category'), '') is not null then
      v_parent := null;
      foreach v_part in array string_to_array(item ->> 'category', '/') loop
        v_part := trim(v_part);
        select id into v_category_id from categories where lower(name) = lower(v_part);
        if v_category_id is null then
          insert into categories (name, parent_id) values (v_part, v_parent) returning id into v_category_id;
        end if;
        v_parent := v_category_id;
      end loop;
    end if;

    v_supplier_id := null;
    if nullif(trim(item ->> 'supplier'), '') is not null then
      select id into v_supplier_id from suppliers where lower(name) = lower(trim(item ->> 'supplier'));
      if v_supplier_id is null then
        insert into suppliers (name) values (trim(item ->> 'supplier')) returning id into v_supplier_id;
      end if;
    end if;

    select id, descriptions into v_product_id, v_existing from products where sku = item ->> 'sku';

    -- Descriptions: a language whose text changed becomes staff-written; the
    -- source (what the rest is translated from) is Georgian when that changed.
    v_descriptions := coalesce(v_existing, '{}'::jsonb);
    v_changed := '{}';
    foreach v_lang in array array['ka', 'en', 'ru'] loop
      if nullif(trim(item -> 'descriptions' ->> v_lang), '') is not null
         and trim(item -> 'descriptions' ->> v_lang) is distinct from v_descriptions -> v_lang ->> 'text' then
        v_descriptions := v_descriptions
          || jsonb_build_object(v_lang, jsonb_build_object('text', trim(item -> 'descriptions' ->> v_lang), 'auto', false));
        v_changed := v_changed || v_lang;
      end if;
    end loop;
    if cardinality(v_changed) > 0 then
      v_descriptions := v_descriptions || jsonb_build_object('source', v_changed[1]);
      -- Machine translations of the old text are out of date: drop them, so the site
      -- falls back to the new text and "Translate missing languages" finds them.
      foreach v_lang in array array['ka', 'en', 'ru'] loop
        if not v_lang = any(v_changed) and coalesce((v_descriptions -> v_lang ->> 'auto')::boolean, false) then
          v_descriptions := v_descriptions - v_lang;
        end if;
      end loop;
    end if;

    if v_product_id is null then
      insert into products (name, sku, has_native_barcode, category_id, supplier_id,
                            cost_price, sell_price, low_stock_threshold, descriptions)
      values (
        trim(item ->> 'name'),
        item ->> 'sku',
        coalesce((item ->> 'native_barcode')::boolean, true),
        v_category_id,
        v_supplier_id,
        coalesce((item ->> 'cost_price')::numeric, 0),
        coalesce((item ->> 'sell_price')::numeric, 0),
        (item ->> 'low_stock_threshold')::int, -- null: the shop's default (products_default_low_stock)
        v_descriptions
      )
      returning id into v_product_id;
      v_created := v_created + 1;
    else
      update products set
        name = coalesce(nullif(trim(item ->> 'name'), ''), name),
        category_id = coalesce(v_category_id, category_id),
        supplier_id = coalesce(v_supplier_id, supplier_id),
        cost_price = coalesce((item ->> 'cost_price')::numeric, cost_price),
        sell_price = coalesce((item ->> 'sell_price')::numeric, sell_price),
        low_stock_threshold = coalesce((item ->> 'low_stock_threshold')::int, low_stock_threshold),
        descriptions = v_descriptions,
        updated_at = now()
      where id = v_product_id;
      v_updated := v_updated + 1;
    end if;
    if cardinality(v_changed) > 0 then
      v_translate := v_translate || v_product_id;
    end if;

    for shop in select jsonb_object_keys(coalesce(item -> 'stock', '{}'::jsonb)) loop
      insert into inventory_levels (product_id, shop_id, quantity)
      values (v_product_id, shop::uuid, (item -> 'stock' ->> shop)::int)
      on conflict (product_id, variant_key, shop_id)
      do update set quantity = excluded.quantity, updated_at = now();
    end loop;

    for variant in select * from jsonb_array_elements(coalesce(item -> 'variants', '[]'::jsonb)) loop
      select id into v_variant_id from product_variants
      where product_id = v_product_id and lower(name) = lower(trim(variant ->> 'name'));
      if v_variant_id is null then
        -- Moves the product's own stock onto its first variant, like the console does.
        v_variant_id := add_product_variant(v_product_id, variant ->> 'name', variant ->> 'sku');
        v_variants_created := v_variants_created + 1;
      elsif nullif(trim(variant ->> 'sku'), '') is not null then
        update product_variants set sku = trim(variant ->> 'sku') where id = v_variant_id;
      end if;

      for shop in select jsonb_object_keys(coalesce(variant -> 'stock', '{}'::jsonb)) loop
        insert into inventory_levels (product_id, variant_id, shop_id, quantity)
        values (v_product_id, v_variant_id, shop::uuid, (variant -> 'stock' ->> shop)::int)
        on conflict (product_id, variant_key, shop_id)
        do update set quantity = excluded.quantity, updated_at = now();
      end loop;
    end loop;
  end loop;

  insert into audit_log (actor_id, action, entity_type, detail)
  values (auth.uid(), 'products_imported', 'product',
          format('imported from a spreadsheet: %s new, %s updated, %s new variants',
                 v_created, v_updated, v_variants_created));

  return jsonb_build_object(
    'created', v_created,
    'updated', v_updated,
    'variants_created', v_variants_created,
    'translate', to_jsonb(v_translate)
  );
end;
$$;

revoke execute on function import_products(jsonb) from public, anon;
grant execute on function import_products(jsonb) to authenticated;
