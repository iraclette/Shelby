-- Storefront customer accounts, separate from staff profiles.

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

-- Signups tagged account_type 'customer' go to customers, everything else to profiles.
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
