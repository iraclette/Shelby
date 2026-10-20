-- Refunds and discounted sales go into the audit log by themselves, so they show up in the
-- Shop Console's Audit Log without the till having to remember (or being able to skip) it.
-- Written by triggers, so sales made offline and synced later are covered too.

-- record_return() inserts the return with total 0, adds its items, then sets the real total:
-- that last update is when the whole refund is known.
create function audit_refund()
returns trigger
language plpgsql
security definer
set search_path = public
as $$
declare
  v_items text;
  v_shop text;
begin
  select string_agg(
           ri.quantity || '× ' || p.name || coalesce(' (' || v.name || ')', ''),
           ', ' order by p.name)
    into v_items
  from return_items ri
  join products p on p.id = ri.product_id
  left join product_variants v on v.id = ri.variant_id
  where ri.return_id = new.id;

  select name into v_shop from shops where id = new.shop_id;

  insert into audit_log (actor_id, action, entity_type, entity_id, detail)
  values (
    new.staff_id,
    'refund',
    'return',
    new.id,
    'GEL ' || to_char(new.total, 'FM999999990.00') || ' at ' || coalesce(v_shop, 'unknown shop')
      || ': ' || coalesce(v_items, 'no items')
      || coalesce(' · reason: ' || nullif(trim(new.reason), ''), '')
  );
  return new;
end;
$$;

create trigger returns_audit_refund
after update of total on returns
for each row
when (old.total = 0 and new.total > 0 and new.staff_id is not null)
execute function audit_refund();

-- One entry per discounted line: charged less than its list price.
create function audit_discount()
returns trigger
language plpgsql
security definer
set search_path = public
as $$
declare
  v_staff uuid;
  v_shop text;
  v_product text;
begin
  select s.staff_id, sh.name into v_staff, v_shop
  from sales s
  left join shops sh on sh.id = s.shop_id
  where s.id = new.sale_id;

  if v_staff is null then
    return new;
  end if;

  select p.name || coalesce(' (' || v.name || ')', '') into v_product
  from products p
  left join product_variants v on v.id = new.variant_id
  where p.id = new.product_id;

  insert into audit_log (actor_id, action, entity_type, entity_id, detail)
  values (
    v_staff,
    'sale_discounted',
    'sale',
    new.sale_id,
    new.quantity || '× ' || coalesce(v_product, 'unknown product') || ': GEL '
      || to_char(new.list_price, 'FM999999990.00') || ' → GEL ' || to_char(new.unit_price, 'FM999999990.00')
      || ' (−' || round((1 - new.unit_price / new.list_price) * 100) || '%) at ' || coalesce(v_shop, 'unknown shop')
  );
  return new;
end;
$$;

create trigger sale_items_audit_discount
after insert on sale_items
for each row
when (new.list_price is not null and new.list_price > 0 and new.unit_price < new.list_price)
execute function audit_discount();

-- Trigger functions only; nobody should call them over the API.
revoke execute on function audit_refund() from public, anon, authenticated;
revoke execute on function audit_discount() from public, anon, authenticated;
