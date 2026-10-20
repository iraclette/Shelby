# Georgian glossary

How Shelby writes Georgian: on the website (`dictionaries/ka.ts`, `legal/ka.ts`, the Telegram
bot's `lib/telegramText.ts`) and in the Shop Console
(`apps/shop-console/resources/translations/ka.json`). Follow it for every new Georgian string;
when a native speaker corrects something, add the lesson here.

## Tone

- **Formal, always: თქვენ.** აირჩიეთ, დააჭირეთ, დაგვიკავშირდით, გვესტუმრეთ. Never the
  informal შენ form (აირჩიე, მოძებნე), even though big shops like extra.ge use it: this is a
  family shop in Gori with customers of every age. (Decided 2026-10-20.)
- Prefer how Georgian shops actually say it over a word-for-word translation of the English.
  If a sentence reads like a translation, rewrite it.
- Errors: **დაფიქსირდა შეცდომა.** (not „რაღაც შეცდომა მოხდა“). Retry hint: **სცადეთ თავიდან.**
  / **სცადეთ ცოტა ხანში.**

## Words

| Use | Not | Meaning |
|---|---|---|
| მიწოდება | მიტანა (in general) | delivery, e.g. „მიწოდება მთელ საქართველოში“ |
| კარამდე მიტანა | | door delivery specifically |
| წაღება | გატანა | pickup from a shop |
| ელფოსტა | ელ. ფოსტა | email |
| ჩილიმი, ჩილიმები | კალიანი | hookah |
| ფანარი, ფანრები | ფარნები | flashlight(s) |
| კალათა | | cart |
| კალათაში დამატება | | add to cart |
| მარაგში არ არის | | out of stock |
| შეკვეთის გაფორმება | | checkout |
| კონტაქტი | კავშირი | contact (footer heading) |
| სუნამო / არომატი / სურნელი | | perfume / fragrance / scent |
| მდგრადი | | long-lasting (perfume) |
| ფლაკონი | | perfume bottle |
| თვითღირებულება | | cost price |

## Shop Console terms

| Use | Meaning |
|---|---|
| სალარო | checkout / the till |
| მარაგი | inventory, stock |
| შტრიხკოდი | barcode |
| მომწოდებელი | supplier |
| ფასდაკლება | discount |
| ხურდა | change (cash back) |
| გადადება / გადადებული გაყიდვა | hold / held sale |
| თანხის დაბრუნება | refund |
| ინდივიდუალური შეკვეთა | custom (made-to-order) request |
| მოქმედებების ჟურნალი | audit log |
| პარამეტრები | settings |
| განახლება | refresh; "update now" is ახლავე განახლება so the two never clash |

## Numbers

Georgian keeps the noun singular after a number: 5 გაყიდვა, 3 პროდუქტი (never 5 გაყიდვები). In the
console's ka.json a plural only needs its "other" form.

## Formatting

- Prices: `12.50 ₾` (number, non-breaking space, ₾). In the Shop Console, amounts follow
  Settings → Business instead.
- Shop names (Shelby, Black Eye Beauty, End), brands and Inex stay in Latin script, with
  Georgian endings attached by a hyphen: Shelby-ში, Inex-ით, Golden Rose-ის.
- Quotes: „…“ (Georgian style), not "…".
- Product names and descriptions come from the database as the shop wrote them; never
  translate or rewrite them.
