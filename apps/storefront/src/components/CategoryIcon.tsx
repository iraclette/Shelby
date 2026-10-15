import {
  Amphora,
  Cigarette,
  Droplets,
  Gift,
  MirrorRound,
  PerfumeBottle,
  PocketKnife,
  Scissors,
  SoapDispenserDroplet,
  Sword,
  Tag,
  Wallet,
  WalletMinimal,
  createLucideIcon,
  type LucideIcon,
} from "lucide-react";

// Home page category tiles, matched by category name (case-insensitive). Icons are Lucide
// (lucide.dev) so they share one stroke style. Unknown categories get a price tag, so a
// category added in the Shop Console still looks right.
// Lucide has no nail polish, so this one is drawn on the same 24px grid and stroke:
// a tall brush cap, a collar and a shouldered bottle with a highlight.
const NailPolish = createLucideIcon("nail-polish", [
  ["rect", { x: "10", y: "2", width: "4", height: "8", rx: "1", key: "cap" }],
  ["rect", { x: "7.5", y: "10", width: "9", height: "2.5", rx: "0.5", key: "collar" }],
  [
    "path",
    { d: "M8 12.5 6.5 14a2 2 0 0 0-.5 1.4V20a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2v-4.6a2 2 0 0 0-.5-1.4L16 12.5", key: "bottle" },
  ],
  ["path", { d: "M9.5 16v3", key: "shine" }],
]);

const icons: Record<string, LucideIcon> = {
  cosmetics: MirrorRound,
  skincare: SoapDispenserDroplet,
  perfume: PerfumeBottle,
  "oil perfumes": Droplets,
  "hair care": Scissors,
  "nail care": NailPolish,
  "leather goods": Wallet,
  "handmade leather": WalletMinimal,
  knives: PocketKnife,
  "decorative katanas": Sword,
  "gift boxes": Gift,
  souvenirs: Amphora, // clay vessels, as in Georgian qvevri wine
  smoking: Cigarette, // hookahs, lighters, snus
};

export default function CategoryIcon({ name, className = "h-8 w-8" }: { name: string; className?: string }) {
  const Icon = icons[name.trim().toLowerCase()] ?? Tag;
  return <Icon className={className} strokeWidth={1.4} aria-hidden="true" />;
}
