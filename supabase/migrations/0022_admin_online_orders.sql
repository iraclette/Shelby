-- Let the Shop Console's admins see website orders and mark them collected.
-- Until now only the storefront's server (service role) could touch these tables.
create policy "admins read orders" on orders for select to authenticated using (is_admin());
create policy "admins update orders" on orders for update to authenticated using (is_admin()) with check (is_admin());
create policy "admins read order_items" on order_items for select to authenticated using (is_admin());
