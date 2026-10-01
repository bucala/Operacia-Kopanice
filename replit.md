# Operácia Kopanice workspace

Operácia Kopanice is a turn-based stealth puzzle game set in a snowy mountain village; the workspace also contains its shared API and design-preview artifacts.

## Run & Operate

- `pnpm --filter @workspace/operacia-kopanice run dev` — run the game (artifact workflow supplies `PORT`/`BASE_PATH`)
- `pnpm --filter @workspace/operacia-kopanice run typecheck|test|build` — the game's own checks; also what CI runs
- `pnpm --filter @workspace/api-server run dev` — run the API server (port 5000)
- `pnpm run typecheck` — full typecheck across all packages
- `pnpm run build` — typecheck + build all packages
- `pnpm --filter @workspace/api-spec run codegen` — regenerate API hooks and Zod schemas from the OpenAPI spec
- `pnpm --filter @workspace/db run push` — push DB schema changes (dev only)
- Required env: `DATABASE_URL` — Postgres connection string

## Stack

- pnpm workspaces, Node.js 24, TypeScript 5.9
- API: Express 5
- DB: PostgreSQL + Drizzle ORM
- Validation: Zod (`zod/v4`), `drizzle-zod`
- API codegen: Orval (from OpenAPI spec)
- Build: esbuild (CJS bundle)

## Where things live

- **The game** — `artifacts/operacia-kopanice` (`@workspace/operacia-kopanice`): TypeScript + Vite + Canvas, static, no backend calls. Rules/levels/renderer under `src/go/`; see `docs/GO-DESIGN.md` (rules, architecture, level list) and `docs/ASSETS.md` (sprite pipeline) inside that package.
- **API server** — `artifacts/api-server` (`@workspace/api-server`): Express, `health` + `assistant` (Claude tactical-hint) routes. Not currently called by the game — see "Architecture decisions" below.
- **Design sandbox** — `artifacts/mockup-sandbox`: shadcn/React scaffold for UI mockups. Unrelated to the shipped game; don't confuse its component library with anything the game uses.
- **Shared libs** — `lib/db` (Drizzle/Postgres schema), `lib/api-spec` + `lib/api-client-react` (OpenAPI codegen). Not consumed by the game.
- **CI** — `.github/workflows/ci.yml`, scoped to `@workspace/operacia-kopanice` (typecheck + test + build on every push/PR).

## Architecture decisions

- The game is fully static — no DB, no server calls. Progress (level unlocks, best turn counts) persists client-side in `localStorage` via `src/go/progress.ts`. The workspace's Postgres/Express/API-codegen packages exist for other artifacts, not this one.
- The real-time isometric stealth engine that originally shared this tree (continuous movement, probabilistic vision, a suspicion FSM) was removed on 2026-08-09 in favor of the deterministic GO ruleset described in `docs/GO-DESIGN.md` — it had drifted out of sync with the shipped game and was never reachable from it. Only the isometric projection, camera, and input handling survive from it (`src/core/{Camera,Input,math}`); the rest is in git history if a real-time mode is ever revisited.
- `main.tsx`/`App.tsx` are a thin React wrapper whose only job is mounting the vanilla-DOM/canvas `GoApp` — no shadcn/Tailwind/React state is wired into gameplay; don't reach for it there.
- `src/go/model/*` has zero DOM/canvas imports on purpose. That purity is what makes every level brute-force-solvability-tested in `test/go.test.ts` — a puzzle that can't be solved can't merge.

## Product

- Play a sequence of deterministic GO-style stealth missions with guard patrols, sight lines, terminals, gates, undo, and saved local progress.
- Explore the missions as a snowy Kopanice village with isometric roads, planks, houses, trees, vehicles, and tactical cover.

## User preferences

- Each completed change should be delivered through a new GitHub pull request with a changelog and README update.

## Gotchas

- `artifacts/operacia-kopanice/vite.config.ts` throws immediately if `PORT` or `BASE_PATH` are unset — the artifact workflow supplies both; set them by hand for a manual `dev`/`build` (`PORT=22332 BASE_PATH=/`).
- The Claude tactical-hint endpoint (`artifacts/api-server/src/routes/assistant.ts`) is implemented and safely gated (server-side key, origin allowlist, default-deny) but **not called from the game** — there is no client wired to it. Check the project audit/roadmap before resurrecting it rather than re-deriving the plan from scratch.

## Pointers

- See the `pnpm-workspace` skill for workspace structure, TypeScript setup, and package details
- `artifacts/operacia-kopanice/docs/GO-DESIGN.md` — game rules, level list, module map
- `artifacts/operacia-kopanice/docs/ASSETS.md` — sprite pipeline, how to add art
