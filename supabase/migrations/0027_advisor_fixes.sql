-- Security advisor cleanup.

-- Pin the trigger's search_path so it can't be hijacked by a role's own path.
alter function cascade_category_visibility() set search_path = public;

-- The profile helpers back RLS for signed-in users; logged-out visitors have no
-- profile, so there's no reason for them to call these over the API.
revoke execute on function current_profile_role() from public, anon;
revoke execute on function current_profile_shop_id() from public, anon;
revoke execute on function is_admin() from public, anon;
grant execute on function current_profile_role() to authenticated;
grant execute on function current_profile_shop_id() to authenticated;
grant execute on function is_admin() to authenticated;

-- The bucket is public, so image URLs work without a SELECT policy. The broad one
-- only let anyone list every file. Admins still need SELECT to delete images.
drop policy "public read product images" on storage.objects;

create policy "admins read product images" on storage.objects for select to authenticated
  using (bucket_id = 'product-images' and is_admin());
