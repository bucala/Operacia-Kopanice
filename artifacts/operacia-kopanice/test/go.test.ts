/**
 * Brute-force solvability suite for all GO puzzle levels.
 *
 * For each level we run a BFS over reachable game states and assert that a
 * winning state is reachable within MAX_TURNS turns.  If a level has a bug
 * that makes it impossible (exit unreachable, player always spotted, etc.) the
 * test fails before the level can ship.
 */
import { describe, it, expect } from 'vitest';
import { LEVELS } from '../src/go/levels';
import { GoGrid } from '../src/go/model/grid';
import {
  advanceGuards,
  applyOfficerAlerts,
  applyPlayerMove,
  cloneState,
  initState,
  legalMoves,
  resolve,
} from '../src/go/model/logic';
import type { GoLevel, GoState } from '../src/go/model/types';

const MAX_TURNS = 60;

/**
 * A compact string key for a game state — covers every variable that
 * distinguishes two otherwise-identical futures: player position, each guard's
 * position/facing/alive/route-cursor/rotation-cursor, and each gate's state.
 * (Turn is intentionally excluded so the BFS can detect true cycles.)
 */
function stateKey(s: GoState): string {
  const p = `${s.player.x},${s.player.y},${s.player.facing}`;
  const g = s.guards
    .map(
      (g) =>
        `${g.alive ? 1 : 0},${g.alerted ? 1 : 0},${g.x},${g.y},${g.facing},${g.routeIndex},${g.routeDir},${g.rotateIndex}`,
    )
    .join('|');
  const gates = s.gates.map((g) => (g.open ? '1' : '0')).join('');
  const distractions = s.distractions.map((d) => (d.used ? '1' : '0')).join('');
  return `${p}/${g}/${gates}/${distractions}`;
}

function isSolvable(levelIndex: number): boolean {
  const level = LEVELS[levelIndex];
  const grid = new GoGrid(level);

  const start = initState(level);
  const seen = new Set<string>();
  // Each queue entry: [state, turnCount]
  const queue: GoState[] = [start];

  while (queue.length > 0) {
    const state = queue.shift()!;

    if (state.phase === 'won') return true;
    if (state.phase === 'lost') continue;
    if (state.turn >= MAX_TURNS) continue;

    const key = stateKey(state);
    if (seen.has(key)) continue;
    seen.add(key);

    for (const move of legalMoves(grid, state)) {
      let next = applyPlayerMove(grid, state, move);
      if (next.phase === 'won') return true;
      if (next.phase === 'lost') continue;
      next = advanceGuards(grid, next);
      next = resolve(grid, next);
      if (next.phase !== 'lost') queue.push(next);
    }
  }

  return false;
}

describe('GO puzzle levels', () => {
  it.each(LEVELS.map((lvl, i) => [lvl.name, i] as [string, number]))(
    'level %s (#%i) is solvable within ' + MAX_TURNS + ' turns',
    (_name, index) => {
      expect(isSolvable(index)).toBe(true);
    },
  );

  it.each(LEVELS)('keeps every terminal on a walkable cell in %s', (level) => {
    const grid = new GoGrid(level);
    for (const terminal of level.terminals ?? []) {
      expect(['floor', 'road', 'plank', 'mud', 'exit']).toContain(grid.kindAt(terminal.x, terminal.y));
    }
  });

  it.each(LEVELS)('keeps solid decorations away from critical nodes in %s', (level) => {
    const grid = new GoGrid(level);
    const critical = [
      level.start,
      ...(level.guards ?? []),
      ...(level.terminals ?? []),
      ...(level.gates ?? []),
      ...(level.distractions ?? []),
      { x: findExit(level).x, y: findExit(level).y },
    ];

    for (const node of critical) {
      expect(grid.decorationBlocksMovement(node.x, node.y)).toBe(false);
    }
  });

  it('applies explicit decoration collision to movement and sight', () => {
    const level: GoLevel = {
      name: 'decoration collision fixture',
      width: 4,
      height: 1,
      cells: ['....'],
      start: { x: 0, y: 0, facing: 'E' },
      guards: [{ id: 'fixture-guard', kind: 'sentry', x: 3, y: 0, facing: 'W', sight: 3 }],
      decorations: [{ kind: 'crate', x: 1, y: 0, blocksMovement: true, blocksSight: true }],
    };
    const grid = new GoGrid(level);
    const state = initState(level);

    expect(grid.decorationBlocksMovement(1, 0)).toBe(true);
    expect(grid.decorationBlocksSight(1, 0)).toBe(true);
    expect(legalMoves(grid, state)).not.toContainEqual({ kind: 'step', dir: 'E' });
  });

  it.each(LEVELS)('keeps every distraction on a walkable cell in %s', (level) => {
    const grid = new GoGrid(level);
    for (const distraction of level.distractions ?? []) {
      expect(['floor', 'road', 'plank', 'mud', 'exit']).toContain(grid.kindAt(distraction.x, distraction.y));
    }
  });

  it('treats houses and trees as solid by default', () => {
    const level: GoLevel = {
      name: 'default decoration collision fixture',
      width: 3,
      height: 1,
      cells: ['...'],
      start: { x: 0, y: 0, facing: 'E' },
      guards: [],
      decorations: [
        { kind: 'house1', x: 1, y: 0 },
        { kind: 'tree', x: 2, y: 0 },
      ],
    };
    const grid = new GoGrid(level);

    expect(grid.decorationBlocksMovement(1, 0)).toBe(true);
    expect(grid.decorationBlocksSight(1, 0)).toBe(true);
    expect(grid.decorationBlocksMovement(2, 0)).toBe(true);
    expect(grid.decorationBlocksSight(2, 0)).toBe(true);
  });

  it('aggregates the maximum sight range for each guard type', () => {
    const level: GoLevel = {
      name: 'sight aggregation fixture',
      width: 4,
      height: 1,
      cells: ['....'],
      start: { x: 0, y: 0, facing: 'E' },
      guards: [
        { id: 'officer-a', kind: 'sentry', x: 1, y: 0, facing: 'S', sight: 2 },
        { id: 'officer-b', kind: 'sentry', x: 2, y: 0, facing: 'N', sight: 4 },
        { id: 'patrol-a', kind: 'patrol', x: 3, y: 0, facing: 'W', sight: 3 },
      ],
    };
    const state = initState(level);
    const sentries = state.guards.filter((guard) => guard.kind === 'sentry');
    const patrols = state.guards.filter((guard) => guard.kind === 'patrol');

    expect(Math.max(...sentries.map((guard) => guard.sight))).toBe(4);
    expect(Math.max(...patrols.map((guard) => guard.sight))).toBe(3);
  });

  it('restores a complete turn-boundary snapshot after a gate interaction and guard response', () => {
    const level: GoLevel = {
      name: 'undo snapshot fixture',
      width: 4,
      height: 2,
      cells: ['....', '....'],
      start: { x: 0, y: 0, facing: 'E' },
      guards: [
        {
          id: 'snapshot-patrol',
          kind: 'patrol',
          x: 3,
          y: 1,
          facing: 'W',
          route: [[3, 1], [2, 1]],
          sight: 2,
        },
      ],
      terminals: [{ id: 'terminal', x: 1, y: 0, gate: 'gate' }],
      gates: [{ id: 'gate', x: 2, y: 0, open: false }],
    };
    const grid = new GoGrid(level);
    const start = initState(level);
    const snapshot = cloneState(start);
    const afterPlayer = applyPlayerMove(grid, start, { kind: 'step', dir: 'E' });
    const afterTurn = resolve(grid, advanceGuards(grid, afterPlayer));

    expect(afterTurn.turn).toBe(1);
    expect(afterTurn.gates[0]?.open).toBe(true);
    expect(afterTurn.guards[0]?.x).toBe(2);
    expect(snapshot).toEqual(start);
    expect(snapshot).not.toEqual(afterTurn);
  });

  it('alerts nearby infantry at the outer edge of an officer sight beam', () => {
    const level: GoLevel = {
      name: 'officer alert fixture',
      width: 6,
      height: 3,
      cells: ['......', '......', '......'],
      start: { x: 1, y: 1, facing: 'W' },
      guards: [
        { id: 'officer', kind: 'sentry', x: 3, y: 1, facing: 'W', sight: 3 },
        {
          id: 'infantry',
          kind: 'patrol',
          x: 4,
          y: 0,
          facing: 'W',
          route: [[3, 0], [4, 0]],
          sight: 1,
        },
      ],
    };
    const grid = new GoGrid(level);
    const initial = initState(level);
    const alerted = applyPlayerMove(grid, initial, { kind: 'step', dir: 'W' });

    expect(alerted.phase).toBe('await');
    expect(alerted.guards.find((guard) => guard.id === 'infantry')?.alerted).toBe(true);
    expect(alerted.guards.find((guard) => guard.id === 'infantry')?.routeDir).toBe(-1);

    const advanced = advanceGuards(grid, alerted);
    expect(advanced.guards.find((guard) => guard.id === 'infantry')?.x).toBe(3);
  });

  it('keeps the inner officer sight field lethal', () => {
    const level: GoLevel = {
      name: 'officer sight fixture',
      width: 4,
      height: 1,
      cells: ['....'],
      start: { x: 2, y: 0, facing: 'E' },
      guards: [{ id: 'officer', kind: 'sentry', x: 3, y: 0, facing: 'W', sight: 3 }],
    };
    const grid = new GoGrid(level);
    const state = applyPlayerMove(grid, initState(level), { kind: 'wait' });
    expect(state.phase).toBe('lost');
    expect(state.outcome).toBe('spotted');
  });

  it("does not let one officer's edge hide another officer's inner beam", () => {
    const level: GoLevel = {
      name: 'overlapping officer sight fixture',
      width: 6,
      height: 1,
      cells: ['......'],
      start: { x: 1, y: 0, facing: 'E' },
      guards: [
        { id: 'near-officer', kind: 'sentry', x: 4, y: 0, facing: 'W', sight: 2 },
        { id: 'far-officer', kind: 'sentry', x: 5, y: 0, facing: 'W', sight: 4 },
      ],
    };
    const grid = new GoGrid(level);
    const state = applyPlayerMove(grid, initState(level), { kind: 'step', dir: 'E' });

    // (2,0) is the near officer's outer edge, but it is an inner cell for
    // the far officer and must remain lethal.
    expect(state.phase).toBe('lost');
    expect(state.outcome).toBe('spotted');
  });

  it('throws a stone one cell ahead and redirects only guards in range', () => {
    const level: GoLevel = {
      name: 'stone distraction fixture',
      width: 8,
      height: 3,
      cells: ['........', '........', '........'],
      start: { x: 1, y: 1, facing: 'E' },
      guards: [
        { id: 'near', kind: 'sentry', x: 4, y: 1, facing: 'S', sight: 1 },
        { id: 'far',  kind: 'sentry', x: 7, y: 1, facing: 'S', sight: 1 },
      ],
      distractions: [
        { id: 'stone', kind: 'stone', x: 2, y: 1, range: 3, direction: 'N' },
      ],
    };
    const grid = new GoGrid(level);
    const initial = initState(level);

    // Player faces E; stone at (2,1) is exactly 1 cell ahead.
    expect(legalMoves(grid, initial)).toContainEqual({ kind: 'activateDistraction', id: 'stone' });

    const activation = applyPlayerMove(grid, initial, {
      kind: 'activateDistraction',
      id: 'stone',
    });

    expect(activation.turn).toBe(1);
    expect(activation.distractions[0]?.used).toBe(true);
    // Guard 'near' is within range 3 (distance 2) → redirected N.
    expect(activation.guards.find((g) => g.id === 'near')?.facing).toBe('N');
    // Guard 'far' is at distance 5 > 3 → not redirected.
    expect(activation.guards.find((g) => g.id === 'far')?.facing).toBe('S');
    // Stone no longer in legal moves once used.
    expect(legalMoves(grid, activation)).not.toContainEqual({
      kind: 'activateDistraction',
      id: 'stone',
    });
  });

  it('cannot throw a stone when player is not facing it', () => {
    const level: GoLevel = {
      name: 'stone wrong-facing fixture',
      width: 4,
      height: 2,
      cells: ['....', '....'],
      start: { x: 0, y: 0, facing: 'S' }, // facing south; stone is east
      guards: [],
      distractions: [
        { id: 'stone', kind: 'stone', x: 1, y: 0, range: 2, direction: 'W' },
      ],
    };
    const grid = new GoGrid(level);
    const state = initState(level);

    // Stone is 1 cell east but player faces south — must not appear.
    expect(legalMoves(grid, state)).not.toContainEqual({
      kind: 'activateDistraction',
      id: 'stone',
    });
  });

  it('cannot activate a stone by standing on its cell (must throw from adjacent)', () => {
    const level: GoLevel = {
      name: 'stone on-cell fixture',
      width: 4,
      height: 1,
      cells: ['....'],
      start: { x: 1, y: 0, facing: 'E' }, // player IS on the stone cell
      guards: [],
      distractions: [
        { id: 'stone', kind: 'stone', x: 1, y: 0, range: 2, direction: 'N' },
      ],
    };
    const grid = new GoGrid(level);
    const state = initState(level);

    // Stone at player's position but stone requires adjacency, not standing-on.
    // Facing E → the cell ahead is (2,0); stone is at (1,0) → not a match.
    expect(legalMoves(grid, state)).not.toContainEqual({
      kind: 'activateDistraction',
      id: 'stone',
    });
  });

  it('bell turns every guard in range toward the bell dynamically', () => {
    const level: GoLevel = {
      name: 'bell distraction fixture',
      width: 9,
      height: 9,
      cells: [
        '.........',
        '.........',
        '.........',
        '.........',
        '.........',
        '.........',
        '.........',
        '.........',
        '.........',
      ],
      // Player stands on the bell cell.
      start: { x: 4, y: 4, facing: 'N' },
      guards: [
        { id: 'north', kind: 'sentry', x: 4, y: 0, facing: 'S', sight: 1 }, // directly north
        { id: 'east',  kind: 'sentry', x: 8, y: 4, facing: 'W', sight: 1 }, // directly east
        { id: 'sw',    kind: 'sentry', x: 0, y: 8, facing: 'N', sight: 1 }, // SW corner
        { id: 'far',   kind: 'sentry', x: 0, y: 0, facing: 'S', sight: 1 }, // NW corner, range 8
      ],
      distractions: [
        // range 8 covers all guards except 'far' at (0,0) which is distance 8 — on the edge.
        // Use range 7 so 'far' at distance 8 is cleanly out, and 'sw' at distance 8 is also out.
        // Use range 6 to ensure 'sw' (dist 8) is excluded but 'north' (dist 4) and 'east' (dist 4)
        // are included. 'sw' is intentionally out-of-range to test the boundary.
        { id: 'bell', kind: 'bell', x: 4, y: 4, range: 5 },
      ],
    };
    const grid = new GoGrid(level);
    const initial = initState(level);
    const activation = applyPlayerMove(grid, initial, {
      kind: 'activateDistraction',
      id: 'bell',
    });

    expect(activation.distractions[0]?.used).toBe(true);
    // 'north' at (4,0): bell at (4,4) is due south — distance 4 ≤ range 5 → face S.
    expect(activation.guards.find((g) => g.id === 'north')?.facing).toBe('S');
    // 'east' at (8,4): bell at (4,4) is due west — distance 4 ≤ range 5 → face W.
    expect(activation.guards.find((g) => g.id === 'east')?.facing).toBe('W');
    // 'sw' at (0,8): distance = 4+4 = 8 > range 5 → not redirected; stays N.
    expect(activation.guards.find((g) => g.id === 'sw')?.facing).toBe('N');
    // 'far' at (0,0): distance = 4+4 = 8 > range 5 → not redirected; stays S.
    expect(activation.guards.find((g) => g.id === 'far')?.facing).toBe('S');
  });

  it('bell requires standing on its cell to activate', () => {
    const level: GoLevel = {
      name: 'bell adjacent fixture',
      width: 4,
      height: 1,
      cells: ['....'],
      start: { x: 0, y: 0, facing: 'E' },
      guards: [],
      distractions: [
        { id: 'bell', kind: 'bell', x: 1, y: 0, range: 3 },
      ],
    };
    const grid = new GoGrid(level);
    const state = initState(level);

    // Player is adjacent and facing the bell but bell requires standing on it.
    expect(legalMoves(grid, state)).not.toContainEqual({
      kind: 'activateDistraction',
      id: 'bell',
    });

    // Stepping onto the bell cell should allow activation.
    const onBell = applyPlayerMove(grid, state, { kind: 'step', dir: 'E' });
    const afterGuards = resolve(grid, advanceGuards(grid, onBell));
    expect(legalMoves(grid, afterGuards)).toContainEqual({
      kind: 'activateDistraction',
      id: 'bell',
    });
  });

  it('activates a generator for exactly one turn and redirects only guards in range', () => {
    const level: GoLevel = {
      name: 'generator distraction fixture',
      width: 8,
      height: 2,
      cells: ['........', '........'],
      start: { x: 2, y: 0, facing: 'E' },
      guards: [
        { id: 'near', kind: 'sentry', x: 4, y: 0, facing: 'S', sight: 1 },
        { id: 'far', kind: 'sentry', x: 7, y: 1, facing: 'W', sight: 1 },
      ],
      distractions: [
        { id: 'generator', kind: 'generator', x: 2, y: 0, range: 3, direction: 'E' },
      ],
    };
    const grid = new GoGrid(level);
    const initial = initState(level);
    const activation = applyPlayerMove(grid, initial, {
      kind: 'activateDistraction',
      id: 'generator',
    });

    expect(activation.turn).toBe(1);
    expect(activation.distractions[0]?.used).toBe(true);
    expect(activation.guards.find((guard) => guard.id === 'near')?.facing).toBe('E');
    expect(activation.guards.find((guard) => guard.id === 'far')?.facing).toBe('W');
    expect(legalMoves(grid, activation)).not.toContainEqual({
      kind: 'activateDistraction',
      id: 'generator',
    });

    const afterTurn = resolve(grid, advanceGuards(grid, activation));
    expect(afterTurn.turn).toBe(1);
    expect(afterTurn.guards.find((guard) => guard.id === 'near')?.facing).toBe('E');

    const repeated = applyPlayerMove(grid, afterTurn, {
      kind: 'activateDistraction',
      id: 'generator',
    });
    expect(repeated.turn).toBe(1);
    expect(repeated).toEqual(afterTurn);
  });

  it('does not activate a distraction that is not on the player cell', () => {
    const level: GoLevel = {
      name: 'unreachable generator fixture',
      width: 3,
      height: 1,
      cells: ['...'],
      start: { x: 0, y: 0, facing: 'E' },
      guards: [],
      distractions: [{ id: 'generator', kind: 'generator', x: 2, y: 0, range: 2, direction: 'W' }],
    };
    const grid = new GoGrid(level);
    const state = initState(level);
    const attempted = applyPlayerMove(grid, state, {
      kind: 'activateDistraction',
      id: 'generator',
    });

    expect(attempted).toEqual(state);
    expect(legalMoves(grid, state)).not.toContainEqual({
      kind: 'activateDistraction',
      id: 'generator',
    });
  });
});

function findExit(level: GoLevel): { x: number; y: number } {
  for (let y = 0; y < level.height; y++) {
    if (level.cells[y]?.includes('X')) {
      return { x: level.cells[y].indexOf('X'), y };
    }
  }
  throw new Error(`Level ${level.name} has no exit`);
}
