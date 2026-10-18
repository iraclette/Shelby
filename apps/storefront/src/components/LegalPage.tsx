import type { Metadata } from "next";
import { getLegalTexts } from "@/i18n/legal";
import type { LegalTexts } from "@/i18n/legal/en";
import { fmt } from "@/i18n/format";
import { legalVars } from "@/lib/business";

type DocKey = keyof Omit<LegalTexts, "updated">;

export async function legalMetadata(doc: DocKey): Promise<Metadata> {
  const legal = await getLegalTexts();
  return { title: `${legal[doc].title} — Shelby` };
}

// Terms, privacy and delivery all share this layout.
export default async function LegalPage({ doc }: { doc: DocKey }) {
  const legal = await getLegalTexts();
  const { title, intro, sections } = legal[doc];

  return (
    <article className="mx-auto max-w-2xl px-6 py-16">
      <h1 className="font-display text-3xl text-paper">{title}</h1>
      <p className="mt-2 text-xs tracking-wide text-paper-dim">{legal.updated}</p>
      <p className="mt-6 text-paper-dim">{fmt(intro, legalVars)}</p>

      {sections.map((section) => (
        <section key={section.heading} className="mt-10">
          <h2 className="font-display text-xl text-paper">{section.heading}</h2>
          <div className="mt-3 space-y-3 text-paper-dim">
            {section.body.map((paragraph) => (
              <p key={paragraph}>{fmt(paragraph, legalVars)}</p>
            ))}
          </div>
        </section>
      ))}
    </article>
  );
}
