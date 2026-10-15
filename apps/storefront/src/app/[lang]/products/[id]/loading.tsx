import Skeleton from "@/components/Skeleton";

export default function ProductLoading() {
  return (
    <div className="mx-auto max-w-6xl px-6 py-16" aria-busy="true">
      <div className="grid gap-12 md:grid-cols-2">
        <Skeleton className="aspect-square border border-line" />
        <div>
          <Skeleton className="h-3 w-24" />
          <Skeleton className="mt-3 h-10 w-3/4" />
          <Skeleton className="mt-5 h-7 w-32" />
          <div className="mt-8 space-y-2">
            <Skeleton className="h-4 w-full" />
            <Skeleton className="h-4 w-full" />
            <Skeleton className="h-4 w-2/3" />
          </div>
          <Skeleton className="mt-6 h-12 w-full" />
          <Skeleton className="mt-4 h-16 w-full" />
        </div>
      </div>
    </div>
  );
}
