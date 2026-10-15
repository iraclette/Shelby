// Placeholder block shown while data is on its way; size it with className (h-*, w-*).
export default function Skeleton({ className = "" }: { className?: string }) {
  return <div aria-hidden="true" className={`rounded-sm bg-ink-elevated motion-safe:animate-pulse ${className}`} />;
}

// Same footprint as ProductCard.
export function ProductCardSkeleton() {
  return (
    <div className="overflow-hidden rounded-sm border border-line bg-ink-soft">
      <Skeleton className="aspect-square rounded-none" />
      <div className="space-y-2 p-4">
        <Skeleton className="h-3 w-1/3" />
        <Skeleton className="h-5 w-3/4" />
        <Skeleton className="h-4 w-1/4" />
      </div>
    </div>
  );
}

// Same footprint as a labelled input/select in the storefront forms.
export function FieldSkeleton() {
  return (
    <div>
      <Skeleton className="h-4 w-28" />
      <Skeleton className="mt-1 h-10.5 w-full" />
    </div>
  );
}
