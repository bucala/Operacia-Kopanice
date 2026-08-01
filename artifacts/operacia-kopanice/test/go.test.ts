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
import { advanceGuards, applyPlayerMove, initState, legalMoves, resolve } from '../src/go/model/logic';
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
        `${g.alive ? 1 : 0},${g.x},${g.y},${g.facing},${g.routeIndex},${g.routeDir},${g.rotateIndex}`,
    )
    .join('|');
  const gates = s.gates.map((g) => (g.open ? '1' : '0')).join('');
  return `${p}/${g}/${gates}`;
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
});

function findExit(level: GoLevel): { x: number; y: number } {
  for (let y = 0; y < level.height; y++) {
    if (level.cells[y]?.includes('X')) {
      return { x: level.cells[y].indexOf('X'), y };
    }
  }
  throw new Error(`Level ${level.name} has no exit`);
}
