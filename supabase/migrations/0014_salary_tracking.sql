-- Daily-wage salary tracking. There's no clock-in system, so a "day worked"
-- is inferred from sales.staff_id: any calendar day an employee has at
-- least one recorded sale counts as a paid day. Pay config is per employee
-- (not every employee earns the same), and actual payouts are logged in an
-- append-only ledger — same philosophy as sales/returns, a payment is never
-- edited or deleted, just recorded.

alter table profiles add column daily_rate numeric(12, 2);
alter table profiles add column bonus_threshold numeric(12, 2);
alter table profiles add column bonus_amount numeric(12, 2);

create table salary_payments (
  id uuid primary key default gen_random_uuid(),
  staff_id uuid not null references profiles(id),
  shop_id uuid not null references shops(id),
  -- The period this payout covers (e.g. a calendar month), so "amount
  -- already paid for period X" can be computed by summing rows here.
  period_start date not null,
  period_end date not null,
  amount numeric(12, 2) not null,
  note text,
  paid_by uuid not null references profiles(id) default auth.uid(),
  paid_at timestamptz not null default now(),
  check (period_end >= period_start)
);

create index on salary_payments (staff_id, period_start);
create index on salary_payments (shop_id);

alter table salary_payments enable row level security;

-- Payroll is admin/owner-only in both directions — unlike sales/returns,
-- staff have no reason to read anyone's pay data here, including their own.
create policy "admins read salary_payments" on salary_payments for select to authenticated
  using (is_admin());
create policy "admins create salary_payments" on salary_payments for insert to authenticated
  with check (is_admin());
