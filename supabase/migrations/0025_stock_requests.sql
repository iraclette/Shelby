-- Out-of-stock products on the storefront: visitors can see whether something is
-- in stock (never how many), and leave a phone/email asking to hear when it's back.

-- Stock summed over every shop and variant. security definer because anon can't
-- read inventory_levels; it only ever returns product ids.
create function in_stock_product_ids()
returns setof uuid
language sql
stable
security definer
set search_path = public
as $$
  select product_id from inventory_levels group by product_id having sum(quantity) > 0
$$;

revoke execute on function in_stock_product_ids() from public;
grant execute on function in_stock_product_ids() to anon, authenticated;

create type stock_request_status as enum ('new', 'contacted', 'dismissed');

create table stock_requests (
  id uuid primary key default gen_random_uuid(),
  product_id uuid not null references products(id) on delete cascade,
  contact text not null, -- phone or email, whichever the visitor gave
  status stock_request_status not null default 'new',
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

-- Asking twice for the same product only keeps one open request.
create unique index stock_requests_open_unique on stock_requests (product_id, lower(contact)) where status = 'new';

alter table stock_requests enable row level security;

-- Admin read/update only; inserts come from the storefront's service-role API route.
create policy "admins read stock_requests" on stock_requests for select to authenticated using (is_admin());
create policy "admins update stock_requests" on stock_requests for update to authenticated using (is_admin()) with check (is_admin());
