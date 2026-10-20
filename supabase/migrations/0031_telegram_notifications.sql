-- Telegram notifications for owners/admins: a staff page hands out a one-time code, the
-- person opens the bot with it, and the bot's webhook (storefront /api/telegram/webhook)
-- links that Telegram chat to their account. The storefront's API routes then message every
-- linked chat that wants the event.

-- One-time codes, put in t.me/<bot>?start=<code>. Created by the signed-in admin from the
-- staff page; read and deleted only by the webhook (service role).
create table telegram_link_codes (
  code text primary key,
  profile_id uuid not null references profiles(id) on delete cascade,
  lang text not null default 'en' check (lang in ('en', 'ka', 'ru')),
  created_at timestamptz not null default now()
);

alter table telegram_link_codes enable row level security;

create policy "admins create own telegram_link_codes" on telegram_link_codes for insert to authenticated
  with check (profile_id = auth.uid() and is_admin());

-- A linked chat. One chat belongs to one account; an account can have several chats
-- (e.g. two people sharing the owner account).
create table telegram_links (
  id uuid primary key default gen_random_uuid(),
  profile_id uuid not null references profiles(id) on delete cascade,
  chat_id bigint not null unique,
  chat_name text,
  lang text not null default 'en' check (lang in ('en', 'ka', 'ru')),
  events text[] not null default array['contact_message', 'stock_request', 'custom_order', 'job_application', 'online_order'],
  created_at timestamptz not null default now()
);

create index on telegram_links (profile_id);

alter table telegram_links enable row level security;

-- Inserts come from the webhook (service role); people manage their own chats.
create policy "read own telegram_links" on telegram_links for select to authenticated
  using (profile_id = auth.uid());
create policy "update own telegram_links" on telegram_links for update to authenticated
  using (profile_id = auth.uid()) with check (profile_id = auth.uid());
create policy "delete own telegram_links" on telegram_links for delete to authenticated
  using (profile_id = auth.uid());
