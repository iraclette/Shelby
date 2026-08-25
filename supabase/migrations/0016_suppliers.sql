-- Mirrors the categories table/RLS/pattern exactly (0001_init.sql) — same
-- shape, new name. Lets the Inventory page track who a product is sourced
-- from, editable the same low-friction "just type it" way categories are.

create table suppliers (
  id uuid primary key default gen_random_uuid(),
  name text not null unique,
  created_at timestamptz not null default now()
);

alter table suppliers enable row level security;

create policy "read suppliers" on suppliers for select to authenticated using (true);
create policy "write suppliers" on suppliers for all to authenticated using (is_admin()) with check (is_admin());

alter table products add column supplier_id uuid references suppliers(id) on delete set null;
