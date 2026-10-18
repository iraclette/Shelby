import type { Metadata } from "next";
import { fetchShops } from "@/lib/supabase";
import { getDictionary } from "@/i18n/server";
import JobsForm from "./JobsForm";

export async function generateMetadata(): Promise<Metadata> {
  const dict = await getDictionary();
  return { title: dict.meta.jobs };
}

export const revalidate = 60;

export default async function JobsPage() {
  const [shops, dict] = await Promise.all([fetchShops(), getDictionary()]);

  return (
    <div className="mx-auto grid max-w-6xl gap-12 px-6 py-16 md:grid-cols-[1fr_minmax(0,28rem)] md:gap-16">
      <div>
        <p className="text-xs tracking-[0.3em] text-brass uppercase">{dict.town}</p>
        <h1 className="mt-3 font-display text-4xl italic text-paper">{dict.jobs.title}</h1>
        <p className="mt-4 max-w-prose leading-relaxed text-paper-dim">{dict.jobs.intro}</p>
        <ul className="mt-10 space-y-6 border-l border-line pl-6">
          {dict.jobs.points.map((point) => (
            <li key={point.title}>
              <p className="text-xs tracking-[0.2em] text-brass uppercase">{point.title}</p>
              <p className="mt-1 text-sm text-paper-dim">{point.text}</p>
            </li>
          ))}
        </ul>
      </div>
      <JobsForm shops={shops} />
    </div>
  );
}
