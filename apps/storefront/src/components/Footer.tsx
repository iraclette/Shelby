const SHOPS = ["Black Eye Beauty", "Shelby", "End"];

export default function Footer() {
  return (
    <footer className="border-t border-line bg-ink-soft">
      <div className="mx-auto max-w-6xl px-6 py-12">
        <div className="flex flex-col gap-8 sm:flex-row sm:justify-between">
          <div>
            <p className="font-display text-xl text-paper">Shelby</p>
            <p className="mt-2 max-w-xs text-sm text-paper-dim">
              Knives, leather goods, cosmetics, and souvenirs — browse online,
              visit us in store.
            </p>
          </div>
          <div>
            <p className="text-xs tracking-[0.2em] text-brass uppercase">Visit us</p>
            <ul className="mt-3 space-y-1 text-sm text-paper-dim">
              {SHOPS.map((shop) => (
                <li key={shop}>{shop}</li>
              ))}
            </ul>
          </div>
        </div>
        <p className="mt-10 border-t border-line pt-6 text-xs text-paper-dim/70">
          © {new Date().getFullYear()} Shelby. All rights reserved.
        </p>
      </div>
    </footer>
  );
}
