// The seller's legal details and delivery prices, shown on the policy pages
// (/terms, /privacy, /returns, /delivery) and in the footer. Banks look for
// these before approving online card payments, so keep them accurate.
// TODO: fill in from the business registration extract before going live.
export const BUSINESS = {
  legalName: "[legal name]",
  idNumber: "[identification code]",
  address: "[registered address], Gori, Georgia",
  phone: "[phone]",
  email: "[email]",
};

// Shipping is through Inex; parcels are dropped off at Inex in Gori.
export const DELIVERY = {
  courier: "Inex",
  branchPrice: 6, // GEL, collected from an Inex branch
  homePrice: 14, // GEL, delivered to the door
};

// Each shop's social pages, keyed by shop name as in the shops table.
// Tracking parameters (?igsh=…, ?rpxt=…) stripped from the shared links.
export const SOCIALS: Record<string, { instagram?: string; facebook?: string }> = {
  "Black Eye Beauty": {
    instagram: "https://www.instagram.com/blackeyebeauty7/",
    facebook: "https://www.facebook.com/profile.php?id=100064280611306",
  },
  Shelby: {
    instagram: "https://www.instagram.com/shelby.shelby2020/",
    facebook: "https://www.facebook.com/shel.bi.180560",
  },
  End: {
    facebook: "https://www.facebook.com/profile.php?id=61579817609363",
  },
};

// Placeholders the policy texts can use: {legalName}, {homePrice}, ...
export const legalVars: Record<string, string | number> = {
  ...BUSINESS,
  courier: DELIVERY.courier,
  branchPrice: DELIVERY.branchPrice,
  homePrice: DELIVERY.homePrice,
};
