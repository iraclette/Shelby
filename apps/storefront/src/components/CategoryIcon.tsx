import {
  Amphora,
  Cigarette,
  Droplets,
  Gift,
  HandHeart,
  Handbag,
  MirrorRound,
  PerfumeBottle,
  PocketKnife,
  Scissors,
  SoapDispenserDroplet,
  Sword,
  Tag,
  type LucideIcon,
} from "lucide-react";

// Home page category tiles, matched by category name (case-insensitive). Icons are Lucide
// (lucide.dev) so they share one stroke style. Unknown categories get a price tag, so a
// category added in the Shop Console still looks right.
const icons: Record<string, LucideIcon> = {
  cosmetics: MirrorRound,
  skincare: SoapDispenserDroplet,
  perfume: PerfumeBottle,
  "oil perfumes": Droplets,
  "hair care": Scissors,
  "nail care": HandHeart,
  "leather goods": Handbag,
  "handmade leather": Handbag,
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
