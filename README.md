# Shelby

Shelby is the software behind three real shops in Gori, Georgia, boasting their variety in goods, going from knives, decorative souvenirs, leather goods, both handmade and imported, camping equipment, lighters, jewelry, perfume, both sprayed and oil, and cosmetics.

The project is three pieces that all talk to the same backend:

- **Shop Console** (`apps/shop-console`), the desktop app running at the register. Handles checkout and everything on the admin side.
- **Storefront** (`apps/storefront`), the website customers browse, order from and get in touch through.
- **Supabase** (`supabase/`), the database and backend logic both apps rely on.

## What the Shop Console does

At checkout, staff get a tap-to-add product grid grouped by category, plus barcode scanning for anything with a real barcode. Discounts can be applied per item, returns can be processed against a past sale, and a sale can be held and picked back up later if a customer steps away mid-purchase. Payment is cash, with quick buttons for the usual notes and the change worked out, or card. If the internet drops during a sale, it gets saved locally and syncs on its own once the connection's back, so nobody has to babysit the wifi just to ring someone up.

Admins open on an overview of the week's sales, best sellers, low stock and anything waiting on them. From there, there's a full inventory view: stock per shop, categories and subcategories, suppliers, low stock warnings, and product variants like different cosmetics shades. Stock can be moved between shops right from there too. Admins also manage staff accounts and roles, track daily wages and payouts, see a daily summary of what sold and who sold it, and check an audit log of who changed what and when, refunds and discounts included.

Everything that comes in from the website lands in the console too: online orders to prepare or ship with Inex, custom leather orders, contact form messages, "let me know when it's back" requests, and job candidates. While the console is open, new ones pop up in the corner of the screen and the sidebar counts update by themselves. Owners can also get them on their phone through a Telegram bot, linked by scanning a QR code.

The Settings page covers the business itself (name, logo, currency, time zone), switching off the sections a business doesn't use, how the till takes payment, defaults for new products, notifications, and the look of the console: several themes, an accent colour and the interface size. The console speaks Georgian, English and Russian. It also updates itself: it checks for new releases and installs them with one click, no manual reinstalling.

## What the storefront does

The website is in Georgian, English and Russian. Customers can browse the catalog with its variants and photos, ask to be told when something out of stock is back, order handmade leather goods made to measure, get in touch through the contact form, and apply for a job at the shops. Online checkout through Bank of Georgia, with pickup at a shop or delivery by Inex, is built in. For now the site runs as a catalogue, where product pages point people to the shops to order, and one switch turns online ordering back on. Customers have their own accounts, separate from staff logins.

Staff get a few pages built for a phone: one to upload product photos on the spot, one to add a new product, one to check stock without opening the desktop app, and one to connect Telegram notifications.

## What ties it together

Both apps read and write to one shared database, so a sale rung up at the register updates stock immediately, inventory stays consistent across all three shops, and nothing customers do on the website steps on what's happening at checkout. Every login, whether it's a customer, a cashier, or an admin, only ever sees and touches what it's supposed to.
