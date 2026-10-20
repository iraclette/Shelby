-- The business's own details, edited from the Shop Console's Settings page instead of being
-- hardcoded: name and logo (login screen, sidebar), currency, and time zone (what "today"
-- means for daily totals). One row for the whole business.

create table business_settings (
  id boolean primary key default true check (id), -- only ever one row
  name text not null default 'My shop',
  logo_path text, -- in the branding bucket; null = the name's first letter instead
  currency_code text not null default 'GEL',
  currency_symbol text not null default '₾',
  -- How amounts are written: "GEL 12.50", "₾12.50" or "12.50 ₾".
  currency_style text not null default 'code_before'
    check (currency_style in ('code_before', 'symbol_before', 'symbol_after')),
  timezone text not null default 'Asia/Tbilisi',
  updated_at timestamptz not null default now(),
  updated_by uuid references profiles(id) on delete set null
);

-- What used to be hardcoded.
insert into business_settings (name, currency_code, currency_symbol, currency_style, timezone)
values ('Shelby', 'GEL', '₾', 'code_before', 'Asia/Tbilisi');

alter table business_settings enable row level security;

-- Nothing here is secret, and the console shows the name/logo on its login screen, before
-- anyone has signed in.
create policy "read business_settings" on business_settings for select to anon, authenticated using (true);
create policy "admins update business_settings" on business_settings for update to authenticated
  using (is_admin()) with check (is_admin());

-- Logos. Public bucket so the image URL works without signing; only admins write.
insert into storage.buckets (id, name, public)
values ('branding', 'branding', true)
on conflict (id) do nothing;

create policy "admins read branding" on storage.objects for select to authenticated
  using (bucket_id = 'branding' and is_admin());
create policy "admins upload branding" on storage.objects for insert to authenticated
  with check (bucket_id = 'branding' and is_admin());
create policy "admins update branding" on storage.objects for update to authenticated
  using (bucket_id = 'branding' and is_admin())
  with check (bucket_id = 'branding' and is_admin());
create policy "admins delete branding" on storage.objects for delete to authenticated
  using (bucket_id = 'branding' and is_admin());
