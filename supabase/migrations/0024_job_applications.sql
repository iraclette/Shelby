-- "Work with us" applications from the storefront's /jobs page, shown in the Shop Console.

create type job_application_status as enum ('new', 'contacted', 'hired', 'rejected');

create table job_applications (
  id uuid primary key default gen_random_uuid(),
  name text not null,
  phone text not null,
  email text,
  preferred_shop_id uuid references shops(id), -- null = any shop
  availability text not null check (availability in ('full_time', 'part_time', 'either')),
  languages text[] not null default '{}',
  about text not null,
  status job_application_status not null default 'new',
  created_at timestamptz not null default now(),
  updated_at timestamptz not null default now()
);

alter table job_applications enable row level security;

-- Admin read/update only; inserts come from the storefront's service-role API route.
create policy "admins read job_applications" on job_applications for select to authenticated using (is_admin());
create policy "admins update job_applications" on job_applications for update to authenticated using (is_admin()) with check (is_admin());
