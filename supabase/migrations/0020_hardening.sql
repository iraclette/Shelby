-- Hardening from `supabase db advisors`, plus a fix for stock transfers.

-- 0015 added transfer_stock(..., p_variant_id default null) without dropping the
-- 5-arg version, so every non-variant transfer failed with "function is not unique".
drop function if exists transfer_stock(uuid, uuid, uuid, uuid, int);

-- Pin search_path so these can't be redirected to look-alike objects in another schema.
alter function confirm_online_order(uuid) set search_path = public;
alter function decrement_inventory(uuid, uuid, int) set search_path = public;
alter function record_return(uuid, uuid, uuid, text, jsonb) set search_path = public;
alter function record_sale(uuid, uuid, jsonb, numeric) set search_path = public;
alter function transfer_stock(uuid, uuid, uuid, uuid, int, uuid) set search_path = public;

-- Functions are executable by PUBLIC (which includes anon) by default. Nothing here
-- is meant for logged-out visitors; every RLS policy that uses the helpers is
-- scoped to authenticated, so they keep working.
revoke execute on function is_admin() from public, anon;
revoke execute on function current_profile_role() from public, anon;
revoke execute on function current_profile_shop_id() from public, anon;
revoke execute on function record_sale(uuid, uuid, jsonb, numeric) from public, anon;
revoke execute on function record_return(uuid, uuid, uuid, text, jsonb) from public, anon;
revoke execute on function transfer_stock(uuid, uuid, uuid, uuid, int, uuid) from public, anon;
revoke execute on function decrement_inventory(uuid, uuid, int) from public, anon;

-- Trigger on auth.users only; triggers don't need EXECUTE, nobody should call it directly.
revoke execute on function handle_new_user() from public, anon, authenticated;

-- Only the BOG webhook calls this, with the service role key.
revoke execute on function confirm_online_order(uuid) from public, anon, authenticated;
