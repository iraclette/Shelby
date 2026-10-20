-- Settings → Modules: sections of the Shop Console a business doesn't use, hidden on every
-- till. Keys: online_orders, salary, job_applications, custom_orders, messages, stock_requests.
-- Empty = everything on, which is what Shelby uses.
alter table business_settings add column disabled_modules text[] not null default '{}';
