-- Developer accounts: the shop console offers them pre-release updates
-- (vX.Y.Z-rcN tags) on top of the normal stable ones.
alter table profiles add column is_dev boolean not null default false;

-- Owners/admins can edit profiles through the API, but not this flag:
-- it's only ever set here, from the database side.
create function guard_profiles_is_dev() returns trigger
language plpgsql set search_path = public as $$
begin
  if auth.role() in ('anon', 'authenticated')
     and new.is_dev is distinct from (case when tg_op = 'UPDATE' then old.is_dev else false end) then
    raise exception 'is_dev can only be changed from the database';
  end if;
  return new;
end;
$$;

create trigger profiles_guard_is_dev
  before insert or update on profiles
  for each row execute function guard_profiles_is_dev();

update profiles set is_dev = true
where id in (select id from auth.users where lower(email) = 'ip20061009@gmail.com');
