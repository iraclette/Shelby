import type { Locale } from "@/i18n/config";

// What the Telegram bot says, in the language the chat was linked in (or the person's
// Telegram language, before it's linked). {placeholders} are filled with fmt().

export const TELEGRAM_EVENTS = ["contact_message", "stock_request", "custom_order", "job_application", "online_order"] as const;
export type TelegramEvent = (typeof TELEGRAM_EVENTS)[number];

type TelegramText = {
  events: Record<TelegramEvent, string>;
  preferredShop: string; // appended to custom orders / job applications when a shop was picked
  pickup: string;
  delivery: string;
  linked: string;
  linkExpired: string;
  stopped: string;
  help: string;
};

export const TELEGRAM_TEXT: Record<Locale, TelegramText> = {
  en: {
    events: {
      contact_message: "✉️ New message from {name} ({contact}):\n\n{message}",
      stock_request: "🔔 Someone wants “{product}” back in stock.\nContact: {contact}",
      custom_order: "🧵 New handmade leather request from {name} ({contact}){shop}:\n\n{description}",
      job_application: "📄 New job application from {name} ({phone}){shop}. The details are in the Shop Console.",
      online_order: "🛒 New paid online order: GEL {total} from {customer} ({how}).",
    },
    preferredShop: ", preferred shop: {shop}",
    pickup: "pickup at {shop}",
    delivery: "Inex delivery to {city}",
    linked:
      "✅ Connected to {name}'s Shelby account. Notifications will arrive here; choose which ones on the Notifications page. Send /stop to disconnect.",
    linkExpired: "This link has expired. Open the Notifications page on shelby.ge and tap “Connect Telegram” again.",
    stopped: "Disconnected. You won't get Shelby notifications here anymore.",
    help: "This bot only sends Shelby notifications. To connect, open the Notifications page on shelby.ge and tap “Connect Telegram”.",
  },
  ka: {
    events: {
      contact_message: "✉️ ახალი შეტყობინება — {name} ({contact}):\n\n{message}",
      stock_request: "🔔 მომხმარებელი ელოდება, რომ „{product}“ ისევ იქნება მარაგში.\nკონტაქტი: {contact}",
      custom_order: "🧵 ახალი შეკვეთა ტყავის ნაკეთობაზე — {name} ({contact}){shop}:\n\n{description}",
      job_application: "📄 ახალი კანდიდატი — {name} ({phone}){shop}. დეტალები Shop Console-შია.",
      online_order: "🛒 ახალი გადახდილი ონლაინ შეკვეთა: GEL {total} — {customer} ({how}).",
    },
    preferredShop: ", სასურველი მაღაზია: {shop}",
    pickup: "წაღება: {shop}",
    delivery: "Inex-ით მიწოდება: {city}",
    linked:
      "✅ დაკავშირებულია {name}-ის Shelby ანგარიშთან. შეტყობინებები აქ მოვა; რომელი — აირჩიეთ შეტყობინებების გვერდზე. გასათიშად გამოგზავნეთ /stop.",
    linkExpired: "ბმულს ვადა გაუვიდა. გახსენით შეტყობინებების გვერდი shelby.ge-ზე და ხელახლა დააჭირეთ „Telegram-ის დაკავშირებას“.",
    stopped: "გათიშულია. Shelby-ის შეტყობინებები აქ აღარ მოვა.",
    help: "ეს ბოტი მხოლოდ Shelby-ის შეტყობინებებს აგზავნის. დასაკავშირებლად გახსენით შეტყობინებების გვერდი shelby.ge-ზე და დააჭირეთ „Telegram-ის დაკავშირებას“.",
  },
  ru: {
    events: {
      contact_message: "✉️ Новое сообщение от {name} ({contact}):\n\n{message}",
      stock_request: "🔔 Кто-то ждёт, когда «{product}» снова появится в наличии.\nКонтакт: {contact}",
      custom_order: "🧵 Новый заказ на изделие из кожи от {name} ({contact}){shop}:\n\n{description}",
      job_application: "📄 Новая заявка на работу от {name} ({phone}){shop}. Подробности в Shop Console.",
      online_order: "🛒 Новый оплаченный онлайн-заказ: GEL {total} от {customer} ({how}).",
    },
    preferredShop: ", магазин: {shop}",
    pickup: "самовывоз: {shop}",
    delivery: "доставка Inex: {city}",
    linked:
      "✅ Подключено к аккаунту Shelby ({name}). Уведомления будут приходить сюда; какие именно — выберите на странице уведомлений. Чтобы отключить, отправьте /stop.",
    linkExpired: "Срок действия ссылки истёк. Откройте страницу уведомлений на shelby.ge и снова нажмите «Подключить Telegram».",
    stopped: "Отключено. Уведомления Shelby сюда больше не придут.",
    help: "Этот бот только присылает уведомления Shelby. Чтобы подключиться, откройте страницу уведомлений на shelby.ge и нажмите «Подключить Telegram».",
  },
};

export function telegramLocale(code: string | undefined | null): Locale {
  return code === "ka" || code === "ru" ? code : "en";
}
