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
import type { GoState } from '../src/go/model/types';

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
});
