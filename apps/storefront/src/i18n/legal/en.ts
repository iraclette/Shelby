// Policy page texts. Kept out of the main dictionaries because they're long.
// {placeholders} are filled from src/lib/business.ts.

export type LegalDoc = {
  title: string;
  intro: string;
  sections: { heading: string; body: string[] }[];
};

export type LegalTexts = {
  updated: string;
  terms: LegalDoc;
  privacy: LegalDoc;
  returns: LegalDoc;
  delivery: LegalDoc;
};

const en: LegalTexts = {
  updated: "Last updated: 17 October 2026",
  terms: {
    title: "Terms of Sale",
    intro:
      "These terms apply to every order placed on this website. The seller is {legalName} (identification code {idNumber}), {address}.",
    sections: [
      {
        heading: "Orders",
        body: [
          "An order is confirmed once the payment has gone through. You'll see a confirmation page with your order number, and we'll use the email or phone you gave us if we need to reach you about it.",
          "If an item turns out to be out of stock after you've paid, we'll contact you and either offer a replacement or refund the item in full.",
        ],
      },
      {
        heading: "Prices and payment",
        body: [
          "All prices are in Georgian lari (GEL) and include VAT where it applies. Delivery is charged separately and shown before you pay.",
          "Card payments (Visa, Mastercard and others offered on the payment page) are processed by Bank of Georgia on its own secure page. We never see or store your card details.",
        ],
      },
      {
        heading: "Pickup and delivery",
        body: [
          "You can collect your order from one of our shops in Gori, or have it delivered anywhere in Georgia by {courier}. See the Delivery page for prices and times.",
        ],
      },
      {
        heading: "Returns",
        body: [
          "You can return most items within 14 days of receiving them. The Returns & Refunds page explains how, and which items can't be returned.",
        ],
      },
      {
        heading: "Age-restricted products",
        body: [
          "Knives, lighters and hookah products are sold only to customers aged 18 or over. By ordering them you confirm that you are 18 or older, and we may ask for ID at pickup or delivery.",
        ],
      },
      {
        heading: "Contact",
        body: ["Questions about an order: {phone}, {email}, or the contact form on this website."],
      },
    ],
  },
  privacy: {
    title: "Privacy Policy",
    intro:
      "This policy explains what personal data {legalName} collects through this website, why, and what you can ask us to do with it.",
    sections: [
      {
        heading: "What we collect",
        body: [
          "When you order: your name, email, phone number, the items you bought, and, for delivery, your address.",
          "When you write to us or request a handmade leather item: your name, contact details and your message.",
          "We don't collect your card details. Payments are handled entirely by Bank of Georgia.",
        ],
      },
      {
        heading: "Why we use it",
        body: [
          "Only to process and deliver your order, to answer your messages, and to keep the accounting records the law requires. We don't sell your data or use it for advertising.",
        ],
      },
      {
        heading: "Who we share it with",
        body: [
          "Bank of Georgia, to take your payment; {courier}, to deliver your parcel (name, phone and address only); and the services that host this website and its database, which store it on our behalf.",
        ],
      },
      {
        heading: "Cookies and storage",
        body: [
          "We use one cookie to remember your language, and your browser's local storage to keep your cart. There are no advertising or tracking cookies.",
        ],
      },
      {
        heading: "How long we keep it",
        body: [
          "Order records are kept for as long as Georgian tax law requires. Messages are deleted when they're no longer needed.",
        ],
      },
      {
        heading: "Your rights",
        body: [
          "You can ask to see the data we hold about you, to correct it, or to delete it where the law allows. Write to {email}.",
        ],
      },
    ],
  },
  returns: {
    title: "Returns & Refunds",
    intro: "Changed your mind, or something's wrong with your order? Here's how returns work.",
    sections: [
      {
        heading: "14 days to change your mind",
        body: [
          "You can return an item within 14 days of receiving it, for any reason, as long as it's unused, in its original packaging and with all its parts.",
          "Bring it to any of our shops, or contact us first at {phone} or {email} and we'll explain how to send it back.",
        ],
      },
      {
        heading: "Items that can't be returned",
        body: [
          "For hygiene reasons: cosmetics, skincare, perfume and nail or hair care products once the seal or packaging has been opened.",
          "Oil perfumes measured out by the gram, and handmade leather items made to your order.",
        ],
      },
      {
        heading: "Damaged or wrong items",
        body: [
          "If your parcel arrives damaged or isn't what you ordered, tell us within 3 days, with a photo if you can. We'll replace it or refund you in full, including delivery.",
        ],
      },
      {
        heading: "Refunds",
        body: [
          "Refunds go back to the card you paid with, usually within 10 working days of us receiving the item. Your bank may take a few more days to show it.",
          "The original delivery fee is refunded only if the item was faulty or wrong. Return shipping for a change of mind is paid by you.",
        ],
      },
    ],
  },
  delivery: {
    title: "Delivery & Pickup",
    intro: "Collect your order in Gori for free, or have it delivered anywhere in Georgia.",
    sections: [
      {
        heading: "Free pickup in Gori",
        body: [
          "Choose a shop at checkout and collect your order there. We'll let you know when it's ready, usually the same or next working day.",
        ],
      },
      {
        heading: "Delivery across Georgia",
        body: [
          "We ship with {courier}, to every city and region in Georgia:",
          "To an {courier} branch near you: {branchPrice} GEL. You collect it there with your ID.",
          "To your door: {homePrice} GEL.",
          "Parcels are handed to {courier} within 1–2 working days of payment, and {courier} usually delivers within a few working days after that.",
        ],
      },
      {
        heading: "Receiving your parcel",
        body: [
          "Please check the parcel when you receive it. If it's damaged, tell the courier and let us know within 3 days (see Returns & Refunds).",
          "Age-restricted items (knives, lighters, hookah products) are handed over only to someone aged 18 or over.",
        ],
      },
    ],
  },
};

export default en;
