-- Mirrors 0008_customer_accounts.sql's reasoning: PostgREST only exposes the
-- public schema, so profiles can't read auth.users.email directly. The
-- Employees admin page needs to show who's who, so this denormalizes it
-- the same way customers.email already does.

alter table profiles add column email text;

-- Backfill existing rows — this runs as a plain SQL statement in the editor,
-- so (unlike PostgREST) it can read auth.users directly.
update profiles set email = auth.users.email from auth.users where profiles.id = auth.users.id;

create or replace function handle_new_user() returns trigger
  language plpgsql security definer set search_path = public as $$
begin
  if new.raw_user_meta_data ->> 'account_type' = 'customer' then
    insert into public.customers (id, email, full_name)
    values (new.id, new.email, new.raw_user_meta_data ->> 'full_name');
  else
    insert into public.profiles (id, email, full_name) values (new.id, new.email, new.raw_user_meta_data ->> 'full_name');
  end if;
  return new;
end;
$$;
