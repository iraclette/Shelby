# Shelby

Shelby is the software behind three real shops in Georgia: Black Eye Beauty, Shelby, and End. They sell knives, leather goods, and cosmetics, and everything runs in GEL.

The project is three pieces that all talk to the same backend:

- **Shop Console** (`apps/shop-console`), the desktop app running at the register. Handles checkout and everything on the admin side.
- **Storefront** (`apps/storefront`), the website customers actually shop on.
- **Supabase** (`supabase/`), the database and backend logic both apps rely on.

## What the Shop Console does

At checkout, staff get a tap-to-add product grid grouped by category, plus barcode scanning for anything with a real barcode. Discounts can be applied per item, returns can be processed against a past sale, and a sale can be held and picked back up later if a customer steps away mid-purchase. If the internet drops during a sale, it gets saved locally and syncs on its own once the connection's back, so nobody has to babysit the wifi just to ring someone up.

On the admin side there's a full inventory view: stock per shop, suppliers, low stock warnings, and support for product variants like different cosmetics shades. Stock can be moved between shops right from there too. Beyond inventory, admins manage staff accounts and roles, track daily wages and payouts, see a daily summary of what sold and who sold it, and check an audit log of who changed what and when. The app also updates itself: it checks for new releases and installs them with one click, no manual reinstalling.

## What the storefront does

Customers can browse the catalog and check out online through Bank of Georgia, with their own account system separate from staff logins. Staff get two pages built for a phone: one to upload product photos on the spot, and one to check stock without having to open the desktop app.

## What ties it together

Both apps read and write to one shared database, so a sale rung up at the register updates stock immediately, inventory stays consistent across all three shops, and nothing customers do on the website steps on what's happening at checkout. Every login, whether it's a customer, a cashier, or an admin, only ever sees and touches what it's supposed to.

## Versioning

Shop Console releases follow x.y.z. x is a major overhaul, y is new functionality, z is a bugfix.
