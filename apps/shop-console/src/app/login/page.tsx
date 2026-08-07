import { login } from "@/app/actions";

export default async function LoginPage({
  searchParams,
}: {
  searchParams: Promise<{ error?: string }>;
}) {
  const { error } = await searchParams;

  return (
    <main className="flex min-h-screen items-center justify-center bg-neutral-50 px-4">
      <form
        action={login}
        className="w-full max-w-sm space-y-4 rounded-lg border border-neutral-200 bg-white p-6 shadow-sm"
      >
        <h1 className="text-lg font-semibold text-neutral-900">
          Shop Console
        </h1>
        {error && <p className="text-sm text-red-600">{error}</p>}
        <div className="space-y-1">
          <label htmlFor="email" className="text-sm text-neutral-700">
            Email
          </label>
          <input
            id="email"
            name="email"
            type="email"
            required
            className="w-full rounded border border-neutral-300 px-3 py-2 text-sm"
          />
        </div>
        <div className="space-y-1">
          <label htmlFor="password" className="text-sm text-neutral-700">
            Password
          </label>
          <input
            id="password"
            name="password"
            type="password"
            required
            className="w-full rounded border border-neutral-300 px-3 py-2 text-sm"
          />
        </div>
        <button
          type="submit"
          className="w-full rounded bg-neutral-900 px-3 py-2 text-sm font-medium text-white hover:bg-neutral-800"
        >
          Sign in
        </button>
      </form>
    </main>
  );
}
