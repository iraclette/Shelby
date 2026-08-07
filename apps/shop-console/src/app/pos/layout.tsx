import { redirect } from "next/navigation";
import { createClient } from "@/lib/supabase/server";
import { signOut } from "@/app/actions";

export default async function PosLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const supabase = await createClient();
  const {
    data: { user },
  } = await supabase.auth.getUser();

  if (!user) {
    redirect("/login");
  }

  return (
    <div className="min-h-screen bg-neutral-50">
      <header className="flex items-center justify-between border-b border-neutral-200 bg-white px-6 py-4">
        <h1 className="text-sm font-semibold text-neutral-900">Checkout</h1>
        <form action={signOut}>
          <button
            type="submit"
            className="text-sm text-neutral-500 hover:text-neutral-900"
          >
            Sign out
          </button>
        </form>
      </header>
      <main>{children}</main>
    </div>
  );
}
