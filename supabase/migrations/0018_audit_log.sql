-- Scoped to actions that don't already have their own ledger (sales,
-- returns, and stock_transfers already record who/when/what) — product
-- field edits, manual inventory adjustments made directly in the Inventory
-- page, and staff account create/delete/role changes.

create table audit_log (
  id uuid primary key default gen_random_uuid(),
  actor_id uuid not null references profiles(id),
  action text not null,       -- e.g. 'product_updated', 'inventory_adjusted', 'staff_created', 'staff_deleted', 'staff_updated'
  entity_type text not null,  -- 'product' | 'inventory_levels' | 'profile'
  entity_id uuid,
  detail text not null,       -- short human-readable summary, e.g. "sell_price: 45.00 -> 50.00"
  created_at timestamptz not null default now()
);

create index on audit_log (created_at desc);

alter table audit_log enable row level security;

-- Same trust model as sales/returns: the acting user's own RLS-permitted
-- write inserts their own row (no security definer needed); only
-- admins/owners can read the trail back.
create policy "insert own audit_log" on audit_log for insert to authenticated with check (actor_id = auth.uid());
create policy "admins read audit_log" on audit_log for select to authenticated using (is_admin());
