# Operácia Kopanice — GO design

A GO game is a **node graph** you traverse one edge at a time; every player move
advances a deterministic clockwork of guards; being seen (or walking into a
guard head-on) kills you; you win by reaching the exit; and generous **undo**
turns the whole thing into a pure logic puzzle — in the vein of **Lara Croft GO**
and **Deus Ex GO**.

> This game used to share the tree with a real-time isometric stealth engine
> (continuous movement, probabilistic vision cones, a suspicion-driven enemy
> FSM). That engine was removed from the workspace on 2026-08-09 — it never
> shipped, was fully superseded by the rules below, and had drifted out of
> sync with them. It is still reachable in git history (`git log --all` from
> before that date) if a real-time mode is ever revisited; nothing here
> depends on it.

## Turn model

A turn is two animated halves, both computed by pure functions in
`src/go/model/logic.ts`:

1. **Player half** (`applyPlayerMove`) — the player steps to one orthogonally
   adjacent node, waits, takes down a guard, or triggers a distraction.
   - Reaching the **exit** wins immediately (guards get no reply).
   - Stepping into a currently **lit** tile → spotted (loss).
2. **Guard half** (`advanceGuards` → `resolve`) — every guard patrols/rotates one
   step, then we re-check: a guard now looking at the player, or standing on
   them, ends the run.

## Guards (deterministic)

- **Patrol** — walks a fixed route, **ping-ponging** between its ends; it
  faces the direction it is travelling, so the corridor *ahead* of it is lethal.
- **Sentry** — stationary; optionally **rotates** its facing through a fixed
  cycle, one entry per turn, sweeping its beam like a camera.

Sight is a **straight line** from the guard along its facing, up to `sight`
cells, stopped by the first wall, closed gate, tree, or rock
(`guardSightCells`). The union of all beams is the set of red "danger" tiles
shown to the player.

### Officer alerts

An **officer** (a sentry) whose outermost visible sight cell would otherwise
spot the player instead raises a warning: nearby **infantry** (patrol guards)
within 2 Manhattan cells become `alerted` and deterministically reverse their
patrol direction. The edge-cell exemption is scoped to that one officer only —
if a *different* officer's inner beam also covers that cell, the player is
still spotted (see `applyOfficerAlerts` and the regression tests around
overlapping beams).

## Stealth takedown

Moving onto a guard's node is a **silent takedown** unless the guard is facing
back the way you came (head-on), in which case it grabs you — a loss. The
legal-move set never offers a head-on step, so a takedown is always a deliberate,
safe flank (`isTakedown`).

## Deus Ex GO hacking

A **terminal** node, when entered, toggles a linked **gate**. A shut gate reads
as a wall (blocks movement *and* sight); an open one is a normal floor. This is
the "hack the door" puzzle primitive — the exit can be sealed behind a gate that
only a detour to the terminal will open.

## Distractions

Interactive, level-authored objects that redirect nearby guards for exactly one
response turn, then become spent (state included in undo/reset snapshots):

| Kind | Activation | Effect |
|---|---|---|
| **Generátor** | Stand on it, then `E` | Every living guard within `range` faces the configured `direction` for one reaction. |
| **Kameň** | Stand one cell away, facing it, then `E` | Thrown one cell ahead; guards within `range` face the configured `direction`. |
| **Zvon** | Stand on it, then `E` | Every guard within `range` turns *toward* the bell — direction computed per guard. |

## Village terrain & decorations

Terrain (`CellKind`): `floor` · `road` · `plank` · `mud` (all walkable, visually
distinct) · `wall` / `tree` / `rock` (block movement and sight) · `exit` ·
`void` (gap in the graph).

Decorations (`DecorationSpec`) are visual props anchored to a cell — houses,
trees, crates, fences — with **explicit** `blocksMovement`/`blocksSight` flags
rather than inferred sprite size. Houses and trees default to solid; crates and
fences default to visual-only. See `.agents/memory/village-decoration-collision.md`
for why this default matters.

## Undo & restart

Each committed turn pushes a deep clone of the previous state onto an undo stack;
`U`/`Z` pops it, `R` restarts the level. Because the whole game state is plain,
`structuredClone`-able data, snapshots are trivial and exact.

## Levels

Levels are hand-authored typed data (`src/go/levels/index.ts`). Each ships with
a `test/go.test.ts` brute-force BFS proof that a winning line exists within
`MAX_TURNS` — a broken puzzle cannot merge.

| # | Level | Introduces |
|---|---|---|
| 1 | Zácvik | Reading a sentry's beam; a generator distraction |
| 2 | Hliadka | A rotating sentry + a patroller; takedown-or-time-it |
| 3 | Terminál | Gate hacking — the exit is sealed until a terminal opens it |
| 4 | Ulička | Slipping past a patroller between houses |
| 5 | Prejazd | Two guards, one gate — timing a courtyard crossing |
| 6 | Prielom | Gate hacking under a rotating sentry's beam |
| 7 | Kameň | Throwing a stone to redirect a guard |
| 8 | Výpadok | A generator that is *mandatory*, not optional — the level is unsolvable without it (its own regression test) |

## Architecture

```
src/go/
  model/
    types.ts    data model: Dir, CellKind, GuardSpec/State, GateSpec/State,
                TerminalSpec/State, DistractionSpec/State, GoLevel, GoState, Move
    grid.ts     GoGrid — static terrain + decoration collision, parsed from level data
    logic.ts    PURE rules: sight, danger, legal moves, takedown, officer alerts,
                distractions, terminal/gate toggling, turn resolution  ← unit-tested
  levels/
    index.ts    the 8 hand-authored puzzles
  progress.ts   level unlocking + best-turn persistence (localStorage)  ← unit-tested
  SpriteCache.ts  tiny image preloader; renderer falls back to shapes while pending
  GoRenderer.ts iso board: platform nodes, danger tint, facing arrows, exit/gate/
                terminal/distraction markers, village decorations, legal-move markers
  GoGame.ts     controller: two-phase turn animation, input→move, undo/restart,
                pause, onOutcome callback
  GoApp.ts      UI/UX shell: title + level-select menu, in-game top bar, enemy
                portrait panel, hint/legend bar, win/lose modals
src/App.tsx     thin React mount point — owns #app/#game/#ui, hands off to GoApp
src/main.tsx    ReactDOM bootstrap
test/go.test.ts        rules + "every level is solvable"
test/progress.test.ts  unlocking + best-turn rules
```

`model/` has **zero** DOM/canvas/engine imports — it is pure data and functions,
which is what makes it deterministic and testable. `GoGame` keeps the *logical*
state exact (it jumps instantly through the pure rules) and only eases the
*visual* positions toward it, so animation can never desync from the rules.

The only pieces reused from the wider engine are `src/core/Camera.ts`,
`src/core/Input.ts`, and `src/core/math/{iso,Vec2}.ts` — the isometric
projection, camera framing, and buffered input, none of which are
real-time-specific.

## Verification

From `artifacts/operacia-kopanice/` (or `pnpm --filter @workspace/operacia-kopanice run <script>` from the workspace root):

```bash
pnpm run typecheck   # strict TS, clean
pnpm run test        # vitest — rules, solvability, progress
pnpm run build       # production bundle
```

CI (`.github/workflows/ci.yml`) runs all three on every push/PR.

## Controls

| Input | Action |
|---|---|
| **Click** a highlighted tile / **↑ ↓ ← →** / **WASD** | Step one node |
| **Space** / **.** | Wait one turn |
| **E** | Activate a distraction (generator/stone/bell) |
| **U** / **Z** | Undo the last turn |
| **R** | Restart the level |
| **Esc** | Open the menu |

Red tiles are lethal lines of sight (except an officer's outermost cell, which
only warns). Reach the exit; slip behind a guard to take them down; open gates
from their terminals; spend a distraction to buy a window.

## Open directions

See the project audit artifact/roadmap for prioritized next steps (narrative
tie-in to the game's own SNP-1944 setting, wiring up the already-built Claude
hint endpoint, level-environment variety). This document covers what's built,
not what's next.
