import Skeleton, { ProductCardSkeleton } from "@/components/Skeleton";

export default function ProductsLoading() {
  return (
    <div className="mx-auto max-w-6xl px-6 py-16" aria-busy="true">
      <Skeleton className="h-9 w-40" />
      <div className="mt-6 flex flex-wrap gap-3">
        {Array.from({ length: 6 }, (_, i) => (
          <Skeleton key={i} className="h-8.5 w-24" />
        ))}
      </div>
      <div className="mt-10 grid grid-cols-2 gap-6 sm:grid-cols-3 md:grid-cols-4">
        {Array.from({ length: 8 }, (_, i) => (
          <ProductCardSkeleton key={i} />
        ))}
      </div>
    </div>
  );
}
