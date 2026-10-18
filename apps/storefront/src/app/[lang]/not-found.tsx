import Link from "next/link";
import { getDictionary } from "@/i18n/server";

export default async function NotFound() {
  const dict = await getDictionary();

  return (
    <section className="relative overflow-hidden">
      <div className="atmosphere" />
      <div className="relative mx-auto max-w-2xl px-6 py-32 text-center">
        <p className="text-xs tracking-[0.3em] text-brass uppercase">404</p>
        <h1 className="mt-6 font-display text-4xl italic text-paper">{dict.notFound.title}</h1>
        <p className="mx-auto mt-4 max-w-md text-paper-dim">{dict.notFound.text}</p>
        <div className="mt-10 flex flex-wrap justify-center gap-4">
          <Link
            href="/products"
            className="inline-block border border-brass px-8 py-3 text-sm tracking-[0.15em] text-brass uppercase transition-colors hover:bg-brass hover:text-ink active:translate-y-px"
          >
            {dict.common.browseCollection}
          </Link>
          <Link
            href="/"
            className="inline-block border border-line px-8 py-3 text-sm tracking-[0.15em] text-paper-dim uppercase transition-colors hover:border-paper-dim hover:text-paper active:translate-y-px"
          >
            {dict.notFound.backHome}
          </Link>
        </div>
      </div>
    </section>
  );
}
