-- Subcategories: a category can sit under one parent (one level deep). Perfumes is
-- the first: its own top-level category with the subcategories the shops sell by.

alter table categories add column parent_id uuid references categories(id) on delete set null;
alter table categories add constraint categories_not_own_parent check (parent_id <> id);

-- Hiding or showing a parent on the website does the same to its subcategories.
create function cascade_category_visibility()
returns trigger
language plpgsql
as $$
begin
  update categories set visible_on_storefront = new.visible_on_storefront where parent_id = new.id;
  return new;
end
$$;

create trigger categories_cascade_visibility
  after update of visible_on_storefront on categories
  for each row
  when (old.visible_on_storefront is distinct from new.visible_on_storefront)
  execute function cascade_category_visibility();

-- The old flat "Perfume" category becomes the parent, so its products keep a home.
update categories set name = 'Perfumes' where lower(name) = 'perfume';
insert into categories (name) values ('Perfumes') on conflict (name) do nothing;

insert into categories (name, parent_id)
select sub.name, parent.id
from categories parent,
  (values
    ('Mini Perfumes 8-12 GEL'),
    ('Special Offer 25 GEL'),
    ('Dubai Perfumes'),
    ('Original Tester Quality'),
    ('High Quality Replicas'),
    ('Niche Perfumes'),
    ('Originals & Replicas')
  ) as sub(name)
where parent.name = 'Perfumes'
on conflict (name) do update set parent_id = excluded.parent_id;
