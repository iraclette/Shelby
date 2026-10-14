-- Read access for anonymous storefront visitors.

create policy "public read products" on products for select to anon using (true);
create policy "public read categories" on categories for select to anon using (true);
create policy "public read product_images" on product_images for select to anon using (true);
