import {
  Amphora,
  Axe,
  BadgeCheck,
  BadgePercent,
  Binoculars,
  Brush,
  Car,
  ChessKnight,
  Crown,
  Droplet,
  Droplets,
  Flame,
  Flashlight,
  Gem,
  Gift,
  MoonStar,
  PerfumeBottle,
  PocketKnife,
  Ribbon,
  Scissors,
  Shield,
  ShoppingBag,
  SoapDispenserDroplet,
  Sparkles,
  SprayCan,
  Tag,
  WalletMinimal,
  Watch,
  createLucideIcon,
  type LucideIcon,
} from "lucide-react";

// Lucide has no nail polish, hip flask or hookah, so those are drawn on the same 24px grid
// and stroke. Nail polish: a tall brush cap, a collar and a shouldered bottle.
const NailPolish = createLucideIcon("nail-polish", [
  ["rect", { x: "10", y: "2", width: "4", height: "8", rx: "1", key: "cap" }],
  ["rect", { x: "7.5", y: "10", width: "9", height: "2.5", rx: "0.5", key: "collar" }],
  [
    "path",
    { d: "M8 12.5 6.5 14a2 2 0 0 0-.5 1.4V20a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2v-4.6a2 2 0 0 0-.5-1.4L16 12.5", key: "bottle" },
  ],
  ["path", { d: "M9.5 16v3", key: "shine" }],
]);

// Hip flask: screw cap on a body with pinched sides.
const HipFlask = createLucideIcon("hip-flask", [
  ["rect", { x: "10", y: "2", width: "4", height: "3", rx: "1", key: "cap" }],
  ["path", { d: "M11 5v2M13 5v2", key: "neck" }],
  [
    "path",
    {
      d: "M7.5 7h9c1.4 0 2.5 1.1 2.5 2.5 0 1.8-1 2.6-1 4.5s1 2.7 1 4.5c0 1.4-1.1 2.5-2.5 2.5h-9C6.1 21 5 19.9 5 18.5c0-1.8 1-2.6 1-4.5s-1-2.7-1-4.5C5 8.1 6.1 7 7.5 7z",
      key: "body",
    },
  ],
  ["path", { d: "M9 11v6", key: "shine" }],
]);

// Hookah: bowl and tray on a stem over a pear-shaped base, hose curling off its side.
const Hookah = createLucideIcon("hookah", [
  ["path", { d: "M10 2h4l-.5 3h-3z", key: "bowl" }],
  ["path", { d: "M8 7h8", key: "tray" }],
  ["path", { d: "M12 5v7", key: "stem" }],
  ["path", { d: "M10 12h4c0 2 3 3 3 5.5a5 5 0 0 1-10 0c0-2.5 3-3.5 3-5.5z", key: "base" }],
  ["path", { d: "M16 15c2.5 0 4 1.5 4 4v3", key: "hose" }],
]);

// Home page category tiles, matched by category name (case-insensitive). Icons are Lucide
// (lucide.dev) so they share one stroke style. Unknown categories get a price tag, so a
// category added in the Shop Console still looks right.
const icons: Record<string, LucideIcon> = {
  "decorative cosmetics": Brush,
  "skincare & body care": SoapDispenserDroplet,
  "hair care": SprayCan,
  "hair clips & ties": Ribbon,
  "nail care": NailPolish,
  perfumes: PerfumeBottle,
  "mini perfumes 8-12 gel": Droplet,
  "special offer 25 gel": BadgePercent,
  "dubai perfumes": MoonStar,
  "original tester quality": BadgeCheck,
  "high quality replicas": Sparkles,
  "niche perfumes": Crown,
  "originals & replicas": PerfumeBottle,
  "original oil perfumes": Droplets,
  knives: PocketKnife,
  "hunting knives & axes": Axe,
  "rosaries, knuckles & batons": Shield,
  flashlights: Flashlight,
  binoculars: Binoculars,
  lighters: Flame,
  hookah: Hookah,
  "flasks & flask sets": HipFlask,
  "hair & beard care": Scissors,
  "car fragrances": Car,
  jewelry: Gem,
  watches: Watch,
  "gift boxes": Gift,
  "gift packages": ShoppingBag,
  "board games": ChessKnight,
  souvenirs: Amphora, // clay vessels, as in Georgian qvevri wine
  "handmade leather": WalletMinimal,
};

export default function CategoryIcon({ name, className = "h-8 w-8" }: { name: string; className?: string }) {
  const Icon = icons[name.trim().toLowerCase()] ?? Tag;
  return <Icon className={className} strokeWidth={1.4} aria-hidden="true" />;
}
