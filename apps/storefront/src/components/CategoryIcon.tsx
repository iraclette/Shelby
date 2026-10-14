// Line icons for the home page category tiles, matched by category name (case-insensitive).
// Unknown categories get a price tag, so a new category from the Shop Console still looks right.

const paths: Record<string, React.ReactNode> = {
  cosmetics: (
    <>
      <rect x="8" y="12" width="8" height="9" rx="1" />
      <path d="M9.5 12V8h5v4" />
      <path d="M9.5 8V5l5-2v5" />
    </>
  ),
  skincare: (
    <>
      <rect x="7" y="9" width="10" height="12" rx="2" />
      <path d="M10 9V6h4v3" />
      <path d="M12 6V3h3" />
      <path d="M10 14h4" />
    </>
  ),
  perfume: (
    <>
      <rect x="5" y="10" width="14" height="11" rx="3" />
      <path d="M9 10V7h6v3" />
      <path d="M10 7V4h4v3" />
      <path d="M9 15h6" />
    </>
  ),
  "oil perfumes": (
    <>
      <rect x="8" y="10" width="8" height="11" rx="2" />
      <path d="M11 10V7h2v3" />
      <path d="M10 7c0-2 1-4 2-4s2 2 2 4z" />
      <path d="M12 14c-1 1.5-1 3 0 3s1-1.5 0-3z" />
    </>
  ),
  "leather goods": (
    <>
      <path d="M5 9h14l-1 12H6z" />
      <path d="M9 9V7a3 3 0 0 1 6 0v2" />
      <path d="M5.5 13h13" />
    </>
  ),
  "handmade leather": (
    <>
      <path d="M5 9h14l-1 12H6z" />
      <path d="M9 9V7a3 3 0 0 1 6 0v2" />
      <path d="M8 13l1 1 1-1 1 1 1-1 1 1 1-1 1 1 1-1" />
    </>
  ),
  "hunting knives": (
    <>
      <path d="M3 21c4-1 8-4 11-8l2 2c-3 3-8 6-13 6z" />
      <path d="M13 11l4 4" />
      <path d="M16 12l4-4-2-2-4 4" />
    </>
  ),
  "decorative katanas": (
    <>
      <path d="M3 21C9 17 13 12 16 8" />
      <path d="M14.5 6.5l3 3" />
      <path d="M17 7l4-4" />
    </>
  ),
  "gift boxes": (
    <>
      <rect x="4" y="10" width="16" height="11" />
      <rect x="3" y="7" width="18" height="3" />
      <path d="M12 7v14" />
      <path d="M12 7c-1.5-3.5-6-3.5-5 0M12 7c1.5-3.5 6-3.5 5 0" />
    </>
  ),
  souvenirs: (
    // a drinking horn (yantsi), the classic Georgian souvenir
    <>
      <path d="M3.5 3.5h5c0 7 4 12.5 11.5 14.5l-1 3C10 19 4 12 3.5 3.5z" />
      <path d="M3.8 6.5h4.9" />
      <path d="M4.3 9h4.8" />
      <circle cx="19.2" cy="19.6" r="0.6" />
    </>
  ),
  hookahs: (
    <>
      <path d="M8 21h8l-1-5a3 3 0 0 0-6 0z" />
      <path d="M12 13V7" />
      <path d="M9 7h6l-1-3h-4z" />
      <path d="M13 11c4 0 7 2 7 6" />
    </>
  ),
  "hair care": (
    <>
      <rect x="3" y="7" width="18" height="4" rx="1" />
      <path d="M5.5 11v6M8.5 11v6M11.5 11v6M14.5 11v6M17.5 11v6" />
    </>
  ),
  "nail care": (
    <>
      <rect x="9" y="11" width="6" height="10" rx="1.5" />
      <path d="M10 11V8h4v3" />
      <path d="M11 8V3h2v5" />
    </>
  ),
};

const fallback = (
  <>
    <path d="M3 12V4h8l9 9-8 8z" />
    <circle cx="7.5" cy="7.5" r="1.5" />
  </>
);

export default function CategoryIcon({ name, className = "h-8 w-8" }: { name: string; className?: string }) {
  return (
    <svg
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.3"
      strokeLinecap="round"
      strokeLinejoin="round"
      className={className}
      aria-hidden="true"
    >
      {paths[name.trim().toLowerCase()] ?? fallback}
    </svg>
  );
}
