-- Product descriptions in every storefront language (ka, en, ru):
--   {"source": "ka",
--    "ka": {"text": "...", "auto": false},   -- written by staff
--    "en": {"text": "...", "auto": true}}    -- filled in by translate-description
-- "source" is the language staff last wrote in. Whenever it changes, the
-- translate-description edge function re-translates every language that is
-- missing or still "auto"; a language staff edited by hand is never overwritten.
alter table products add column descriptions jsonb not null default '{}';

-- Every existing description was written in Georgian.
update products
set descriptions = jsonb_build_object('source', 'ka', 'ka', jsonb_build_object('text', description, 'auto', false))
where coalesce(description, '') <> '';

-- description stays the source-language text, for anything that still reads it.
create function products_sync_description() returns trigger
language plpgsql
as $$
begin
  if new.descriptions ? 'source' and (tg_op = 'INSERT' or new.descriptions is distinct from old.descriptions) then
    new.description := nullif(new.descriptions -> (new.descriptions ->> 'source') ->> 'text', '');
  elsif tg_op = 'INSERT' or new.description is distinct from old.description then
    -- Written by a client older than shop-console 2.3.3, which only knows the
    -- one description field: take it as Georgian, translations of the old text go.
    new.descriptions := case
      when coalesce(new.description, '') = '' then '{}'::jsonb
      else jsonb_build_object('source', 'ka', 'ka', jsonb_build_object('text', new.description, 'auto', false))
    end;
  end if;
  return new;
end;
$$;

create trigger products_sync_description before insert or update on products
  for each row execute function products_sync_description();

-- Called by translate-description with the caller's own token (so the usual
-- admin-only write policy applies). Fills in only languages that are still
-- missing or "auto", and does nothing if the source text changed while the
-- translation was running: that newer edit's own translation run wins.
create function apply_description_translations(
  target_product uuid,
  source_lang text,
  source_text text,
  translations jsonb
)
returns boolean
language plpgsql
security invoker
set search_path = public
as $$
declare
  existing jsonb;
  merged jsonb;
  lang text;
begin
  select descriptions into existing from products where id = target_product for update;
  if existing is null
     or existing ->> 'source' is distinct from source_lang
     or existing -> source_lang ->> 'text' is distinct from source_text then
    return false;
  end if;

  merged := existing;
  for lang in select jsonb_object_keys(translations) loop
    if lang <> source_lang and coalesce((existing -> lang ->> 'auto')::boolean, true) then
      merged := merged || jsonb_build_object(lang, jsonb_build_object('text', translations ->> lang, 'auto', true));
    end if;
  end loop;

  update products set descriptions = merged where id = target_product;
  return true;
end;
$$;

revoke execute on function apply_description_translations(uuid, text, text, jsonb) from public;
grant execute on function apply_description_translations(uuid, text, text, jsonb) to authenticated;
