# Operácia Kopanice

A **turn-based, node-graph tactics puzzle** in the vein of **Lara Croft GO** and
**Deus Ex GO**, built on a from-scratch isometric TypeScript engine. You move one
node at a time across a snow-bound Kopanice homestead: read the guards'
deterministic sight-lines, slip behind them for silent takedowns, hack terminals
to open gates, and reach the exit — with unlimited **undo** so every level is a
solvable puzzle, not a reflex test.

> This began as a real-time isometric stealth core; it was audited and
> transformed into the GO-style game. The original real-time engine still lives
> in the tree. See **[`docs/GO-DESIGN.md`](docs/GO-DESIGN.md)** for the audit,
> the design, and what was reused vs replaced.

![Operácia Kopanice](docs/screenshot.png)

> Slovak is the in-game language (HUD, log). The code and docs are in English.

## The GO puzzle

| Piece | Where it lives |
|---|---|
| **Pure, deterministic rules** (sight, takedown, hacking, turn resolution) | `src/go/model/logic.ts` |
| **Data model** (levels, guards, gates, state) | `src/go/model/types.ts`, `grid.ts` |
| **Hand-authored, solvable levels** | `src/go/levels/` |
| **Iso board renderer** (danger tiles, facing arrows, legal-move markers) | `src/go/GoRenderer.ts` |
| **Turn controller** (two-phase animation, undo, restart) | `src/go/GoGame.ts` |
| **Rules + "every level is solvable" tests** | `test/go.test.ts` |

## Underlying engine (and the original real-time stealth core)

The GO game is built on a from-scratch isometric engine that also still powers
the original real-time stealth RTT (`main.ts` now boots the GO game; the RTT
files remain in the tree). The engine highlights:

| Brief requirement | Where it lives |
|---|---|
| **ECS architecture** — entities = components | `src/core/ecs/` (`World`, `Component`, `System`, `EventBus`) |
| **Isometric tilemap renderer** with depth + volume | `src/systems/RenderSystem.ts`, `src/core/math/iso.ts` |
| **A\* pathfinding** with terrain costs (snow/mud/road) | `src/map/Pathfinding.ts`, `src/core/util/BinaryHeap.ts` |
| **Vision (FoV) raycasting** honouring a height map | `src/map/Fov.ts`, `src/systems/VisionSystem.ts` |
| **Enemy AI FSM**: Patrol → Suspicious → Alert | `src/ai/FSM.ts`, `src/ai/enemyFsm.ts`, `src/systems/AISystem.ts` |
| **Skill system** with code hooks (`knifeAction`, `disguiseAction`, …) | `src/skills/` |
| **Positional 2D audio** with occlusion | `src/systems/AudioSystem.ts`, `public/assets/audio.json` |
| **All assets as readable text/JSON** | `public/assets/*.json` |
| **GitHub + cloud DB + Claude Code** integrations | `.github/`, `src/integrations/`, `api/assistant.ts` |

## Quick start

```bash
npm install
npm run dev        # http://localhost:5173
```

Other scripts:

```bash
npm run build      # type-check + production bundle to dist/
npm run preview    # serve the production build
npm run test       # unit tests (pathfinding, FoV, ECS, FSM, inventory)
npm run typecheck  # tsc --noEmit
```

## Controls (GO mode — the default)

| Input | Action |
|---|---|
| **Click** a highlighted tile · **↑ ↓ ← →** · **WASD** | Step one node |
| **Space** / **.** | Wait one turn |
| **U** / **Z** | Undo the last turn |
| **R** | Restart the level |
| **N** | Next level (after solving) |

Red tiles are lethal lines of sight. Reach the green exit; step onto a guard from
its blind side for a silent takedown (never head-on); open gates from their
terminals.

## How the systems fit together

The game is a fixed-order schedule of systems run every frame against one
`World` (see `src/game/Game.ts`):

```
Input → Skill → Movement → Vision → AI → Audio → Sync → Assistant → Render
```

- **Movement** advances agents along A\* paths; speed scales inversely with tile
  cost, and entering a tile emits a footstep sound whose loudness depends on the
  terrain (snow is loud, road is quiet).
- **Vision** recomputes each viewer's field of view by raycasting across a cone,
  with occlusion driven by the per-tile **height map** — walls and trees block
  line of sight, low hay bales do not.
- **AI** runs every guard through a finite-state machine. Footsteps and thrown
  stones feed a suspicion model that drives `PATROL → SUSPICIOUS → ALERT`
  transitions; a disguise slows how fast guards grow suspicious.
- **Audio** synthesises every sound from the bank (oscillator/noise — no binary
  audio files), then attenuates by distance, pans by screen offset, and muffles
  by counting blockers between the source and the listener.
- **Render** draws the world in isometric projection with a depth-sorted pass
  (ground diamonds, extruded tiles/props for volume, procedurally-shaded
  characters), then fog-of-war and overlay passes.

See [`CLAUDE.md`](CLAUDE.md) for a deeper architecture tour.

## Assets are data

Everything visual and audible is a readable JSON file under `public/assets/`:

- `tiles.json` — the tile palette: movement cost, footstep noise, walkability,
  vision-blocking, height, decoration, and colours.
- `maps/kopanice.json` — the level as ASCII terrain rows + an elevation layer +
  entity placements. Authored by `scripts/genmap.mjs` (`node scripts/genmap.mjs`).
- `sprites.json` — character + prop definitions. Characters render as shaded
  isometric volumes by default; each can reference an optional high-fidelity
  `image` (player, guard) and `props` are image-only (buildings, crates,
  decorations). When the PNG is present and listed in `sprites/manifest.json`
  the renderer uses it, otherwise it falls back to the procedural look — so no
  image files are required to run. See [`docs/ASSETS.md`](docs/ASSETS.md) to add
  art (drop a PNG → `node scripts/genmanifest.mjs`).
- `audio.json` — the synthesised sound bank and occlusion tuning.

### Extending

- **New skill:** implement the `Skill` interface (`src/skills/types.ts`) and
  `registry.register(...)` it in `src/game/Game.ts`. The hook gets a
  `SkillContext` with the world, target, logging, and sound emission.
- **New tile:** add an entry to `tiles.json` and a legend char in the map.
- **New map:** add `public/assets/maps/<name>.json` and load it via
  `loadAssets('<name>')`.

## Integrations

- **GitHub** — CI in `.github/workflows/ci.yml` runs typecheck, tests, and build
  on every push/PR.
- **Cloud database (Firebase Realtime)** — `src/integrations/sync/` persists the
  player's position, health, inventory, and disguise. With Firebase env vars set
  it syncs to the Realtime Database (SDK loaded lazily from the CDN — no build
  dependency); otherwise it falls back to `localStorage`. See `.env.example`.
- **Claude Code assistant** — press **H** for a tactical hint. With
  `VITE_ASSISTANT_ENDPOINT` set, the request goes to the serverless function in
  `api/assistant.ts`, which calls the Claude API server-side (key stays off the
  client). With no endpoint, a deterministic local advisor answers, so the
  assistant always works offline.

## Deploy (Vercel)

The repo is a static Vite build plus one serverless function:

1. Import the repo into Vercel (framework preset: **Vite**, config in
   `vercel.json`).
2. To enable cloud sync and/or the Claude assistant, set the variables from
   `.env.example` in the Vercel project (`ANTHROPIC_API_KEY` server-side;
   `VITE_*` for the client).
3. `npm run build` → `dist/` is served statically; `api/assistant.ts` is
   deployed as a function at `/api/assistant`.

## Project layout

```
src/
  core/            ECS (World/Component/System/EventBus), math, camera, input
  components/      Position, Movement, Render, Vision, AIComp, Skills, Inventory…
  systems/         Input, Skill, Movement, Vision, AI, Audio, Sync, Assistant, Render
  map/             TileMap, tile defs, A* pathfinding, FoV raycasting
  ai/              generic FSM + enemy state machine
  skills/          skill registry + knife / disguise / stone hooks
  integrations/    sync (Firebase + local), Claude assistant, config
  game/            Game orchestration + entity spawners
  main.ts          bootstrap + HUD
public/assets/     tiles.json, sprites.json, audio.json, maps/kopanice.json
api/assistant.ts   serverless Claude endpoint
test/              unit tests
scripts/genmap.mjs level authoring tool
```

## License

MIT.
