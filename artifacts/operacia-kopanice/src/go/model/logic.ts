/**
 * The rules of the turn-based node-graph game, as **pure functions**.
 *
 * Nothing here touches the DOM, canvas, timers, or RNG — every function takes a
 * {@link GoGrid} plus a {@link GoState} and returns data (often a fresh state).
 * That is what makes the game deterministic, unit-testable (see `test/go.test.ts`),
 * and cheap to snapshot for the undo stack.
 *
 * ## Turn structure
 * A turn is two halves. The controller animates between them:
 *  1. {@link applyPlayerMove} — the player steps / waits / takes down a guard /
 *     trips a terminal. Reaching the exit wins immediately; stepping into a lit
 *     tile (a guard's current sight line) is fatal.
 *  2. {@link advanceGuards} then {@link resolve} — every guard patrols or rotates
 *     one step, and we re-check: a guard now looking at the player, or standing
 *     on them, ends the run.
 */
import { GoGrid } from './grid';
import {
  type Dir,
  DIR_VEC,
  DIRS,
  dirOf,
  type GateState,
  type GoLevel,
  type GoState,
  type GuardState,
  type Move,
  opposite,
} from './types';

export const key = (x: number, y: number): string => `${x},${y}`;

/** Deep, structural clone of a state — the unit of the undo stack. */
export function cloneState(s: GoState): GoState {
  return structuredClone(s);
}

/** Build the initial runtime state from a hand-authored level. */
export function initState(level: GoLevel): GoState {
  const guards: GuardState[] = level.guards.map((g) => {
    const route = g.route ?? [[g.x, g.y]];
    const rotate = g.rotate ?? [];
    const routeIndex = Math.max(
      0,
      route.findIndex(([rx, ry]) => rx === g.x && ry === g.y),
    );
    const rotateIndex = Math.max(0, rotate.indexOf(g.facing));
    return {
      id: g.id,
      kind: g.kind,
      x: g.x,
      y: g.y,
      facing: g.facing,
      routeIndex,
      routeDir: 1,
      rotateIndex,
      route,
      rotate,
      sight: g.sight,
      alive: true,
      alerted: false,
      variant: g.variant,
    };
  });

  const gates: GateState[] = (level.gates ?? []).map((g) => ({ ...g }));
  const terminals = (level.terminals ?? []).map((t) => ({ ...t }));

  return {
    player: { x: level.start.x, y: level.start.y, facing: level.start.facing },
    guards,
    gates,
    terminals,
    phase: 'await',
    outcome: null,
    turn: 0,
  };
}

// --- Queries -----------------------------------------------------------------

function gateAt(state: GoState, x: number, y: number): GateState | undefined {
  return state.gates.find((g) => g.x === x && g.y === y);
}

/** Can something stand on / move through this cell right now? */
export function walkable(grid: GoGrid, state: GoState, x: number, y: number): boolean {
  const gate = gateAt(state, x, y);
  if (gate) return gate.open;
  if (grid.decorationBlocksMovement(x, y)) return false;
  const k = grid.kindAt(x, y);
  return k === 'floor' || k === 'road' || k === 'plank' || k === 'mud' || k === 'exit';
}

/** Does this cell stop a guard's line of sight? Walls and shut gates do. */
export function blocksSight(grid: GoGrid, state: GoState, x: number, y: number): boolean {
  const gate = gateAt(state, x, y);
  if (gate) return !gate.open;
  if (grid.decorationBlocksSight(x, y)) return true;
  const kind = grid.kindAt(x, y);
  return kind === 'wall' || kind === 'tree' || kind === 'rock';
}

/** The living guard occupying (x, y), if any. */
export function guardAt(state: GoState, x: number, y: number): GuardState | undefined {
  return state.guards.find((g) => g.alive && g.x === x && g.y === y);
}

/**
 * The ordered lethal cells a guard currently watches: a straight beam from its
 * cell along its facing, up to `sight`, stopped by the first sight-blocker.
 * The guard's own cell is excluded (that interaction is a takedown/collision).
 */
export function guardSightCells(grid: GoGrid, state: GoState, guard: GuardState): string[] {
  if (!guard.alive) return [];
  const { dx, dy } = DIR_VEC[guard.facing];
  const cells: string[] = [];
  for (let i = 1; i <= guard.sight; i++) {
    const x = guard.x + dx * i;
    const y = guard.y + dy * i;
    if (!grid.inBounds(x, y)) break;
    if (blocksSight(grid, state, x, y)) break;
    cells.push(key(x, y));
  }
  return cells;
}

/** Union of every living guard's sight beam — the tiles that are "lit". */
export function dangerCells(grid: GoGrid, state: GoState): Set<string> {
  const set = new Set<string>();
  for (const g of state.guards) {
    if (!g.alive) continue;
    for (const c of guardSightCells(grid, state, g)) set.add(c);
  }
  return set;
}

/**
 * The outermost currently visible cell for every living officer.
 * Sentries are the existing officer slot; patrols are the infantry slot.
 */
export function officerAlertCells(grid: GoGrid, state: GoState): Set<string> {
  const cells = new Set<string>();
  for (const guard of state.guards) {
    if (!guard.alive || guard.kind !== 'sentry') continue;
    const sight = guardSightCells(grid, state, guard);
    const edge = sight.at(-1);
    if (edge) cells.add(edge);
  }
  return cells;
}

/**
 * Alert nearby infantry once when the player reaches an officer's sight edge.
 * The route direction is reversed deterministically; no random pursuit is added.
 */
export function applyOfficerAlerts(grid: GoGrid, prev: GoState, beforeMove?: GoState): GoState {
  const state = cloneState(prev);
  if (state.phase !== 'await') return state;

  const playerKey = key(state.player.x, state.player.y);
  const wasAlreadyAtEdge =
    beforeMove !== undefined &&
    officerAlertCells(grid, beforeMove).has(key(beforeMove.player.x, beforeMove.player.y));
  if (wasAlreadyAtEdge) return state;
  const officers = state.guards.filter(
    (guard) =>
      guard.alive &&
      guard.kind === 'sentry' &&
      officerAlertCells(grid, state).has(playerKey),
  );
  if (officers.length === 0) return state;

  for (const infantry of state.guards) {
    if (!infantry.alive || infantry.kind !== 'patrol' || infantry.alerted) continue;
    const nearby = officers.some(
      (officer) => Math.abs(officer.x - infantry.x) + Math.abs(officer.y - infantry.y) <= 2,
    );
    if (!nearby) continue;
    infantry.alerted = true;
    if (infantry.routeIndex === 0) infantry.routeDir = 1;
    else if (infantry.routeIndex === infantry.route.length - 1) infantry.routeDir = -1;
    else infantry.routeDir = (infantry.routeDir * -1) as 1 | -1;
  }
  return state;
}

/** An officer's edge cell is a warning, but every other visible beam cell is lethal. */
function playerIsInLethalSight(grid: GoGrid, state: GoState): boolean {
  const playerKey = key(state.player.x, state.player.y);
  const officerEdges = officerAlertCells(grid, state);
  return state.guards.some(
    (guard) =>
      guard.alive &&
      guardSightCells(grid, state, guard).includes(playerKey) &&
      !(guard.kind === 'sentry' && officerEdges.has(playerKey)),
  );
}

/**
 * Whether stepping in `moveDir` onto a guard is a stealth takedown. You succeed
 * from the side or behind; a guard facing back the way you came (head-on) grabs
 * you instead.
 */
export function isTakedown(guard: GuardState, moveDir: Dir): boolean {
  return guard.facing !== opposite(moveDir);
}

/** The moves the player may legally choose from the current state. */
export function legalMoves(grid: GoGrid, state: GoState): Move[] {
  if (state.phase !== 'await') return [];
  const moves: Move[] = [{ kind: 'wait' }];
  for (const dir of DIRS) {
    const { dx, dy } = DIR_VEC[dir];
    const tx = state.player.x + dx;
    const ty = state.player.y + dy;
    const guard = guardAt(state, tx, ty);
    if (guard) {
      // Can only enter a guard's cell if it's a takedown (never head-on).
      if (isTakedown(guard, dir)) moves.push({ kind: 'step', dir });
      continue;
    }
    if (walkable(grid, state, tx, ty)) moves.push({ kind: 'step', dir });
  }
  return moves;
}

// --- Turn resolution ---------------------------------------------------------

/** First half of a turn: apply the player's action and its immediate results. */
export function applyPlayerMove(grid: GoGrid, prev: GoState, move: Move): GoState {
  const state = cloneState(prev);
  if (state.phase !== 'await') return state;
  state.turn += 1;

  if (move.kind === 'step') {
    const { dx, dy } = DIR_VEC[move.dir];
    const tx = state.player.x + dx;
    const ty = state.player.y + dy;
    state.player.facing = move.dir;

    const guard = guardAt(state, tx, ty);
    if (guard) {
      if (isTakedown(guard, move.dir)) {
        const g = state.guards.find((q) => q.id === guard.id);
        if (g) g.alive = false;
        state.player.x = tx;
        state.player.y = ty;
      } else {
        // Walked straight into a guard's face.
        state.player.x = tx;
        state.player.y = ty;
        state.phase = 'lost';
        state.outcome = 'spotted';
        return state;
      }
    } else if (walkable(grid, state, tx, ty)) {
      state.player.x = tx;
      state.player.y = ty;
    } else {
      // Illegal step (blocked): no-op, refund the turn.
      state.turn -= 1;
      return state;
    }
  }

  // Trip any terminal on the destination cell (toggles its gate).
  const term = state.terminals.find((t) => t.x === state.player.x && t.y === state.player.y);
  if (term) {
    const gate = state.gates.find((g) => g.id === term.gate);
    if (gate) gate.open = !gate.open;
  }

  // Reaching the exit wins outright — guards do not get a move.
  if (grid.kindAt(state.player.x, state.player.y) === 'exit') {
    state.phase = 'won';
    state.outcome = 'exit';
    return state;
  }

  // Reaching an officer's outermost sight cell triggers an alert instead of
  // immediate detection. Any other lit cell remains lethal.
  const alerted = applyOfficerAlerts(grid, state, prev);
  state.guards = alerted.guards;
  if (playerIsInLethalSight(grid, state)) {
    state.phase = 'lost';
    state.outcome = 'spotted';
  }
  return state;
}

/** Second half of a turn: every living guard patrols or rotates one step. */
export function advanceGuards(grid: GoGrid, prev: GoState): GoState {
  const state = cloneState(prev);
  if (state.phase !== 'await') return state;

  for (const g of state.guards) {
    if (!g.alive) continue;
    if (g.kind === 'sentry') {
      if (g.rotate.length > 0) {
        g.rotateIndex = (g.rotateIndex + 1) % g.rotate.length;
        g.facing = g.rotate[g.rotateIndex];
      }
      continue;
    }
    // Patroller: ping-pong to the next cell of its route.
    if (g.route.length < 2) continue;
    let next = g.routeIndex + g.routeDir;
    if (next < 0 || next >= g.route.length) {
      g.routeDir = (g.routeDir * -1) as 1 | -1;
      next = g.routeIndex + g.routeDir;
    }
    const [nx, ny] = g.route[next];
    if (!walkable(grid, state, nx, ny)) continue; // e.g. a gate shut in its path
    const d = dirOf(nx - g.x, ny - g.y);
    if (d) g.facing = d;
    g.x = nx;
    g.y = ny;
    g.routeIndex = next;
  }
  return state;
}

/** Verdict after the guards have moved: caught by a beam or a body? */
export function resolve(grid: GoGrid, prev: GoState): GoState {
  const state = cloneState(prev);
  if (state.phase !== 'await') return state;
  const { x, y } = state.player;

  if (guardAt(state, x, y)) {
    state.phase = 'lost';
    state.outcome = 'collision';
    return state;
  }
  if (playerIsInLethalSight(grid, state)) {
    state.phase = 'lost';
    state.outcome = 'spotted';
  }
  return state;
}
