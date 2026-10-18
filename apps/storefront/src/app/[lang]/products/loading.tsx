import Skeleton, { ProductGridSkeleton } from "@/components/Skeleton";

export default function ProductsLoading() {
  return (
    <div className="mx-auto max-w-6xl px-6 py-16" aria-busy="true">
      <Skeleton className="h-9 w-40" />
      <div className="mt-6 flex flex-wrap gap-3">
        {Array.from({ length: 6 }, (_, i) => (
          <Skeleton key={i} className="h-8.5 w-24" />
        ))}
      </div>
      <ProductGridSkeleton />
    </div>
  );
}
