# CLAUDE.md — project guide for Claude Code

Context for working in this repository with Claude Code. Read alongside
[`README.md`](README.md).

## What this is

`Operácia Kopanice` is the core of a 2D isometric, grid-based tactical stealth
game. It is a TypeScript + Vite browser app rendering to a 2D canvas, built on a
small hand-written ECS engine. There is no game framework dependency — the
engine, renderer, pathfinder, FoV, AI, and audio are all in this repo.

## Commands

```bash
npm run dev        # dev server
npm run build      # tsc --noEmit + vite build  (run before pushing)
npm run test       # node --test via tsx
npm run typecheck  # tsc --noEmit
node scripts/genmap.mjs   # regenerate public/assets/maps/kopanice.json
```

CI (`.github/workflows/ci.yml`) runs typecheck, tests, and build on push/PR —
keep all three green.

## Architecture in one screen

- **ECS** (`src/core/ecs/`): `World` owns entities, component stores, and
  singleton "resources". Components are plain data tagged with a `type`; each
  component module exports an interface **and** a same-named `ComponentKind`
  value so `world.get(e, Position)` is fully typed. Systems implement
  `System.update(world, ctx)` and run in a fixed order via `Scheduler`.
- **Communication is via events**, not direct system references — see
  `EventBus` and `src/core/events.ts` (sounds, AI state changes, skill use,
  actor death, log lines, assistant requests).
- **System order** (`src/game/Game.ts`): Input → Skill → Movement → Vision →
  AI → Audio → Sync → Assistant → Render. Order matters: Movement emits sounds
  the AI buffers and reacts to the same frame; Vision runs before AI.

## Where things live

| Concern | File(s) |
|---|---|
| Isometric math / projection | `src/core/math/iso.ts` |
| A\* pathfinding (weighted, octile heuristic) | `src/map/Pathfinding.ts` |
| Field of view (height-aware raycast) | `src/map/Fov.ts` |
| Tile grid + queries | `src/map/TileMap.ts`, `src/map/tiles.ts` |
| Generic FSM | `src/ai/FSM.ts` |
| Enemy states (Patrol/Suspicious/Alert) | `src/ai/enemyFsm.ts` |
| Skill hooks | `src/skills/knifeAction.ts`, `disguiseAction.ts`, `stoneAction.ts` |
| Rendering | `src/systems/RenderSystem.ts` |
| Cloud sync + Claude assistant | `src/integrations/` + `api/assistant.ts` |

## Conventions

- **Logical vs visual position:** `Position` carries integer grid `gx,gy` (used
  by gameplay) and float `fx,fy` (interpolated by `MovementSystem` for smooth
  on-screen motion). Use `cellOf(pos)` to get a `{x,y}` grid `Vec2`.
- **Assets are data.** Don't hard-code tiles, sprites, or sounds — edit the JSON
  under `public/assets/`. The map is generated; edit `scripts/genmap.mjs` and
  re-run it rather than hand-editing `kopanice.json`.
- **Images are optional, manifest-gated.** Characters/props can use real PNGs
  (`sprites.json` `image`/`props`), but the renderer only loads files listed in
  `public/assets/sprites/manifest.json` (rebuild via `node scripts/genmanifest.mjs`)
  and otherwise falls back to procedural art. Keep that fallback intact — never
  assume an image exists. See `docs/ASSETS.md`.
- **Strict TypeScript** (`noUnusedLocals`, `noImplicitReturns`, etc.). `tsc` only
  type-checks `src/` and `test/`; the Vercel function in `api/` is checked by
  Vercel at deploy time.
- Keep new code in the established style: small modules, documented public
  types, behaviour split into systems.

## Adding things

- **A skill:** implement `Skill` (`src/skills/types.ts`), register it in
  `Game.buildWorld()`, and (optionally) bind a key in
  `src/systems/InputSystem.ts`.
- **A system:** implement `System` and `scheduler.add(...)` it in the right
  order in `Game.buildWorld()`.
- **A tile / map:** edit `public/assets/tiles.json` / add a map JSON and load via
  `loadAssets('<name>')`.

## Gotchas

- Browsers require a user gesture before the `AudioContext` can play — handled
  by `AudioSystem.resume()` wired to the first click/keypress.
- FoV is recomputed only when a viewer's tile or facing changes (cached in
  `VisionSystem`); if you add inputs that should refresh vision, invalidate the
  cache key accordingly.
