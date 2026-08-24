-- Public customer accounts for the storefront (sign up / sign in), kept
-- separate from `profiles` (internal shop staff only) so a customer signup
-- can never end up with a staff role. This is intentionally minimal — just
-- enough for an account to exist; it's not a loyalty program.

create table customers (
  id uuid primary key references auth.users(id) on delete cascade,
  email text not null,
  full_name text,
  created_at timestamptz not null default now()
);

alter table customers enable row level security;

create policy "customers read own row" on customers for select to authenticated
  using (id = auth.uid());
create policy "customers update own row" on customers for update to authenticated
  using (id = auth.uid()) with check (id = auth.uid());
-- Row creation only happens via handle_new_user() below (security definer),
-- so there's deliberately no insert policy for customers/authenticated here.
create policy "admins read customers" on customers for select to authenticated
  using (is_admin());

-- handle_new_user() (0001_init.sql) previously inserted every new
-- auth.users row into `profiles`, assuming every signup was staff. Now that
-- the storefront lets anyone sign up, that assumption no longer holds — the
-- storefront's sign-up call tags itself with account_type: 'customer' in
-- the auth metadata, and this routes those into `customers` instead. Any
-- signup that doesn't set that (i.e. the existing staff-creation path)
-- keeps landing in `profiles`, unchanged.
create or replace function handle_new_user() returns trigger
  language plpgsql security definer set search_path = public as $$
begin
  if new.raw_user_meta_data ->> 'account_type' = 'customer' then
    insert into public.customers (id, email, full_name)
    values (new.id, new.email, new.raw_user_meta_data ->> 'full_name');
  else
    insert into public.profiles (id, full_name) values (new.id, new.raw_user_meta_data ->> 'full_name');
  end if;
  return new;
end;
$$;
