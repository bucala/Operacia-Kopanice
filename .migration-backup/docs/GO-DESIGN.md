# Operácia Kopanice — GO transformation

This document is two things: an **audit** of the original real-time isometric
stealth core, and the **design + implementation notes** for transforming it into
a turn-based, node-graph tactics puzzle in the vein of **Lara Croft GO** and
**Deus Ex GO**.

---

## 1. Audit of the real-time core

The original game (still in the tree under `src/systems/`, `src/game/Game.ts`) is
a real-time isometric stealth RTT built on a hand-written ECS. The audit looked
at what carries over to a GO-style game and what is tied to real-time play.

### Strengths / reusable assets

| Module | Verdict | Why |
|---|---|---|
| `src/core/ecs/*` | Reusable engine | Clean `World`/`Component`/`System`/`EventBus`. Not needed for GO's tiny state, but solid. |
| `src/core/math/iso.ts` | **Reused as-is** | `gridToScreen` / `screenToGrid` / `depthKey` are exactly the projection a GO board needs. |
| `src/core/Camera.ts` | **Reused as-is** | World↔screen transform; GO uses it for a fit-to-board framing instead of follow. |
| `src/core/Input.ts` | **Reused as-is** | Buffered clicks + edge-triggered keys map naturally onto discrete turns. |
| `src/map/Fov.ts`, `Pathfinding.ts`, `TileMap.ts` | Reference / partial | Height-aware raycast FoV and weighted A\* are overkill for GO's straight-line sight and one-step moves; kept for the RTT and as a reference. |
| `src/ai/FSM.ts`, `enemyFsm.ts` | Not used by GO | A suspicion-driven `PATROL→SUSPICIOUS→ALERT` model is a real-time concept. GO guards are deterministic. |
| JSON asset pipeline | Reference | The RTT's "assets are data" approach is great; GO levels are authored as typed data instead (`src/go/levels/`) so puzzles can be unit-checked. |

### Coupling / what did *not* transfer

- **Continuous simulation.** `MovementSystem` interpolates `fx,fy` along A\*
  paths every frame; `AISystem` runs suspicion accumulation and sound reactions.
  GO has no continuous time — a turn is a discrete, fully-resolved step.
- **Probabilistic detection.** The RTT's vision cone + suspicion meter is fuzzy
  by design. GO detection is **binary and deterministic**: a tile is lit or it
  isn't, and stepping into light is always fatal. This is what makes a GO level a
  *solvable puzzle* rather than a reflex test.
- **Skills/audio/sync/assistant systems** are real-time features (cooldowns,
  positional audio occlusion, autosave, live tactical hints). They are out of
  scope for the GO slice and left intact for the RTT build.

### Test surface (why the transform is safe)

The existing unit tests import **only pure modules** (`@/ai/FSM`,
`@/components/{Actor,Inventory,Position}`, `@/core/ecs/World`,
`@/core/util/BinaryHeap`, `@/map/{Fov,Pathfinding,TileMap,tiles}`). No test
imports a `System` or `Game`. So repointing `main.ts` at the GO controller and
adding the GO layer cannot break the existing suite — and indeed all 22 original
tests still pass alongside the 11 new GO tests.

---

## 2. GO design

A GO game is a **node graph** you traverse one edge at a time; every player move
advances a deterministic clockwork of guards; being seen (or walking into a
guard head-on) kills you; you win by reaching the exit; and generous **undo**
turns the whole thing into a pure logic puzzle.

### Turn model

A turn is two animated halves, both computed by pure functions:

1. **Player half** (`applyPlayerMove`) — the player steps to one orthogonally
   adjacent node, waits, takes down a guard, or trips a terminal.
   - Reaching the **exit** wins immediately (guards get no reply).
   - Stepping into a currently **lit** tile → spotted (loss).
2. **Guard half** (`advanceGuards` → `resolve`) — every guard patrols/rotates one
   step, then we re-check: a guard now looking at the player, or standing on
   them, ends the run.

### Guards (deterministic)

- **Patroller** — walks a fixed route, **ping-ponging** between its ends; it
  faces the direction it is travelling, so the corridor *ahead* of it is lethal.
- **Sentry** — stationary; optionally **rotates** its facing through a fixed
  cycle, one entry per turn, sweeping its beam like a camera.

Sight is a **straight line** from the guard along its facing, up to `sight`
cells, stopped by the first wall or shut gate (`guardSightCells`). The union of
all beams is the set of red "danger" tiles shown to the player.

### Stealth takedown

Moving onto a guard's node is a **silent takedown** unless the guard is facing
back the way you came (head-on), in which case it grabs you — a loss. The
legal-move set never offers a head-on step, so a takedown is always a deliberate,
safe flank (`isTakedown`).

### Deus Ex GO hacking

A **terminal** node, when entered, toggles a linked **gate**. A shut gate reads
as a wall (blocks movement *and* sight); an open one is a normal floor. This is
the "hack the door" puzzle primitive — the exit can be sealed behind a gate that
only a detour to the terminal will open.

### Undo & restart

Each committed turn pushes a deep clone of the previous state onto an undo stack;
`U`/`Z` pops it, `R` restarts the level. Because the whole game state is plain,
`structuredClone`-able data, snapshots are trivial and exact.

### Levels

Levels are hand-authored typed data (`src/go/levels/`), each introducing one
idea: **Zácvik** (read the beam, reach the exit), **Hliadka** (a rotating sentry
+ a patroller — takedown or time it), **Terminál** (a gate seals the exit; trip
the terminal). A test brute-forces a winning line for every shipped level so a
broken puzzle can never ship.

---

## 3. Architecture of the GO layer

```
src/go/
  model/
    types.ts    data model: Dir, CellKind, GuardSpec/State, GateSpec/State, GoLevel, GoState, Move
    grid.ts     GoGrid — static terrain parsed from level char-rows
    logic.ts    PURE rules: sight, danger, legal moves, takedown, turn resolution, hacking  ← unit-tested
  levels/
    index.ts    hand-authored puzzles (Zácvik, Hliadka, Terminál)
  progress.ts   level unlocking + best-turn persistence (localStorage)  ← unit-tested
  GoRenderer.ts iso board: platform nodes, danger tint, facing arrows, exit/gate/terminal, legal-move markers
  GoGame.ts     controller: two-phase turn animation, input→move, undo/restart, pause, outcome events
  GoApp.ts      UI/UX shell: title + level-select menu, in-game top bar, hint/legend bar, win/lose modals
src/main.ts     bootstrap → GoApp
test/go.test.ts        rules + "every level is solvable"
test/progress.test.ts  unlocking + best-turn rules
```

### UI / UX shell

`GoApp` is a plain-DOM layer over the canvas (no framework): a title screen with
a **level-select grid** (locked 🔒 / playable ▶ / cleared ✓ with a best-turn
badge), an in-game **top bar** (level · turn · guards + Undo / Restart / Menu),
a bottom **hint + legend** bar, and **win/lose modals** with the natural next
actions (Next level / Undo / Retry / Menu). Progress and best turn counts persist
via `localStorage` (`progress.ts`), so levels unlock as you clear them. While any
modal is up, `GoGame` is paused and the board freezes as a blurred backdrop —
`GoGame` reports terminal states through an `onOutcome` callback rather than
driving the UI itself, keeping rules, rendering, and chrome cleanly separated.

The `model/` layer has **zero** DOM/canvas/engine imports — it is pure data and
functions, which is what makes it deterministic and testable. `GoGame` keeps the
*logical* state exact (it jumps instantly through the pure rules) and only eases
the *visual* positions toward it, so animation can never desync from the rules.

### Reuse summary

- **Reused from the engine:** `iso.ts` projection, `Camera`, `Input`, `Vec2`.
- **New for GO:** the entire `src/go/` tree.
- **Kept but unused by GO (tree-shaken from the bundle):** all real-time
  `systems/`, `Game.ts`, skills, AI FSM, sync, assistant. Production build drops
  from ~60 modules to 13.

---

## 4. Verification

- `npm run typecheck` — clean (strict TS).
- `npm test` — 33 tests (22 original + 11 GO), including the solvability proof.
- `npm run build` — production bundle builds; GO-only tree-shaken output.
- Browser smoke — levels load and solve, undo works, zero console errors.

---

## 5. Controls

| Input | Action |
|---|---|
| **Click** a highlighted tile / **↑ ↓ ← →** / **WASD** | Step one node |
| **Space** / **.** | Wait one turn |
| **U** / **Z** | Undo the last turn |
| **R** | Restart the level |
| **N** | Next level (after solving) |

Red tiles are lethal lines of sight. Reach the green exit; slip behind a guard to
take them down; open gates from their terminals.

---

## 6. Future work

- More levels + a level index/menu; a move counter and par targets.
- Deus Ex GO extras: guards that alarm rather than instakill, hackable cameras,
  moving platforms, keys.
- Optional: fold the GO board back onto the real-time `TileMap`/`Fov` so the two
  modes can share authored maps.
