-- The Shop Console's existing "read products/categories/product_images"
-- policies only grant SELECT to the `authenticated` role (logged-in staff).
-- The public storefront has anonymous visitors with no login at all, so it
-- needs its own read policies for the `anon` role. Postgres combines
-- multiple permissive SELECT policies with OR, so this is purely additive.

create policy "public read products" on products for select to anon using (true);
create policy "public read categories" on categories for select to anon using (true);
create policy "public read product_images" on product_images for select to anon using (true);
