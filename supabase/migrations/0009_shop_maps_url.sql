-- Lets the storefront link each shop's name straight to its pinned Google
-- Maps location, rather than a geocoded-from-address guess.
alter table shops add column maps_url text;

update shops set maps_url = 'https://maps.app.goo.gl/3Bd7uH1rmcVCaCbWA' where name = 'Black Eye Beauty';
update shops set maps_url = 'https://maps.app.goo.gl/m4dQX2ewNcFJWYUE6' where name = 'Shelby';
update shops set maps_url = 'https://maps.app.goo.gl/1q3bmTstEyugi544A' where name = 'End';
