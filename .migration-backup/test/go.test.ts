import { test } from 'node:test';
import assert from 'node:assert/strict';
import { GoGrid } from '@/go/model/grid';
import {
  advanceGuards,
  applyPlayerMove,
  dangerCells,
  guardSightCells,
  initState,
  isTakedown,
  key,
  legalMoves,
  resolve,
  walkable,
} from '@/go/model/logic';
import type { GoLevel, GoState, GuardState } from '@/go/model/types';
import { LEVELS } from '@/go/levels';

function makeLevel(cells: string[], patch: Partial<GoLevel> = {}): GoLevel {
  return {
    name: 'test',
    width: cells[0].length,
    height: cells.length,
    cells,
    start: { x: 0, y: 0, facing: 'S' },
    guards: [],
    ...patch,
  };
}

function build(level: GoLevel): { grid: GoGrid; state: GoState } {
  return { grid: new GoGrid(level), state: initState(level) };
}

test('a guard sight beam runs straight and stops at a wall', () => {
  const level = makeLevel(['..#...'], {
    guards: [{ id: 'g', kind: 'sentry', x: 0, y: 0, facing: 'E', sight: 5 }],
  });
  const { grid, state } = build(level);
  const beam = guardSightCells(grid, state, state.guards[0]);
  assert.deepEqual(beam, [key(1, 0)]); // the wall at (2,0) halts the beam
});

test('sight range is honoured', () => {
  const level = makeLevel(['......'], {
    guards: [{ id: 'g', kind: 'sentry', x: 0, y: 0, facing: 'E', sight: 3 }],
  });
  const { grid, state } = build(level);
  assert.deepEqual(guardSightCells(grid, state, state.guards[0]), [key(1, 0), key(2, 0), key(3, 0)]);
});

test('takedown succeeds from behind, fails head-on', () => {
  const behind: GuardState = { facing: 'E' } as GuardState; // player steps E onto it
  assert.ok(isTakedown(behind, 'E'));
  const facingYou: GuardState = { facing: 'W' } as GuardState;
  assert.ok(!isTakedown(facingYou, 'E'));
});

test('stepping onto a guard from its blind side eliminates it', () => {
  const level = makeLevel(['...'], {
    start: { x: 0, y: 0, facing: 'E' },
    guards: [{ id: 'g', kind: 'sentry', x: 1, y: 0, facing: 'E', sight: 0 }],
  });
  const { grid, state } = build(level);
  assert.ok(legalMoves(grid, state).some((m) => m.kind === 'step' && m.dir === 'E'));
  const after = applyPlayerMove(grid, state, { kind: 'step', dir: 'E' });
  assert.equal(after.guards[0].alive, false);
  assert.deepEqual([after.player.x, after.player.y], [1, 0]);
  assert.equal(after.phase, 'await');
});

test('walking head-on into a facing guard is fatal and not offered as legal', () => {
  const level = makeLevel(['...'], {
    start: { x: 0, y: 0, facing: 'E' },
    guards: [{ id: 'g', kind: 'sentry', x: 1, y: 0, facing: 'W', sight: 0 }],
  });
  const { grid, state } = build(level);
  assert.ok(!legalMoves(grid, state).some((m) => m.kind === 'step' && m.dir === 'E'));
  const after = applyPlayerMove(grid, state, { kind: 'step', dir: 'E' });
  assert.equal(after.phase, 'lost');
});

test('stepping into a lit tile gets you spotted', () => {
  const level = makeLevel(['...', '...', '...'], {
    start: { x: 0, y: 0, facing: 'S' },
    guards: [{ id: 'g', kind: 'sentry', x: 2, y: 1, facing: 'W', sight: 2 }],
  });
  const { grid, state } = build(level);
  assert.ok(dangerCells(grid, state).has(key(0, 1)));
  const after = applyPlayerMove(grid, state, { kind: 'step', dir: 'S' });
  assert.equal(after.phase, 'lost');
  assert.equal(after.outcome, 'spotted');
});

test('a terminal toggles its gate, opening the path', () => {
  const level = makeLevel(['....'], {
    start: { x: 0, y: 0, facing: 'E' },
    terminals: [{ id: 't', x: 1, y: 0, gate: 'gate' }],
    gates: [{ id: 'gate', x: 2, y: 0, open: false }],
  });
  const { grid, state } = build(level);
  assert.ok(!walkable(grid, state, 2, 0)); // shut gate blocks
  const after = applyPlayerMove(grid, state, { kind: 'step', dir: 'E' });
  assert.equal(after.gates[0].open, true);
  assert.ok(walkable(grid, after, 2, 0)); // now passable
});

test('reaching the exit wins immediately', () => {
  const level = makeLevel(['.X'], { start: { x: 0, y: 0, facing: 'E' } });
  const { grid, state } = build(level);
  const after = applyPlayerMove(grid, state, { kind: 'step', dir: 'E' });
  assert.equal(after.phase, 'won');
  assert.equal(after.outcome, 'exit');
});

test('a patroller ping-pongs between the ends of its route', () => {
  const level = makeLevel(['....', '....'], {
    start: { x: 0, y: 1, facing: 'S' },
    guards: [
      {
        id: 'p',
        kind: 'patrol',
        x: 0,
        y: 0,
        facing: 'E',
        route: [
          [0, 0],
          [1, 0],
          [2, 0],
        ],
        sight: 1,
      },
    ],
  });
  const { grid } = build(level);
  let state = initState(level);
  const xs: number[] = [];
  for (let i = 0; i < 5; i++) {
    state = advanceGuards(grid, state);
    xs.push(state.guards[0].x);
  }
  assert.deepEqual(xs, [1, 2, 1, 0, 1]); // forward to the end, then reverse
});

test('a rotating sentry advances its facing each turn', () => {
  const level = makeLevel(['...', '...', '...'], {
    start: { x: 0, y: 0, facing: 'S' },
    guards: [
      { id: 's', kind: 'sentry', x: 1, y: 1, facing: 'N', rotate: ['N', 'E', 'S', 'W'], sight: 1 },
    ],
  });
  const { grid } = build(level);
  let state = initState(level);
  const facings: string[] = [];
  for (let i = 0; i < 4; i++) {
    state = advanceGuards(grid, state);
    facings.push(state.guards[0].facing);
  }
  assert.deepEqual(facings, ['E', 'S', 'W', 'N']);
});

test('every shipped level is solvable', () => {
  for (const level of LEVELS) {
    assert.ok(solvable(level), `${level.name} should be solvable`);
  }
});

/** Brute-force BFS over reachable turn-boundary states to prove a level winnable. */
function solvable(level: GoLevel): boolean {
  const grid = new GoGrid(level);
  const seen = new Set<string>();
  const queue: GoState[] = [initState(level)];
  seen.add(stateKey(queue[0]));
  while (queue.length) {
    const state = queue.shift()!;
    for (const move of legalMoves(grid, state)) {
      const afterPlayer = applyPlayerMove(grid, state, move);
      if (afterPlayer.phase === 'won') return true;
      if (afterPlayer.phase === 'lost') continue;
      const next = resolve(grid, advanceGuards(grid, afterPlayer));
      if (next.phase === 'won') return true;
      if (next.phase === 'lost') continue;
      const k = stateKey(next);
      if (!seen.has(k)) {
        seen.add(k);
        queue.push(next);
      }
    }
  }
  return false;
}

function stateKey(s: GoState): string {
  const guards = s.guards
    .map((g) => `${g.x},${g.y},${g.facing},${g.alive ? 1 : 0},${g.routeIndex},${g.routeDir},${g.rotateIndex}`)
    .join('|');
  const gates = s.gates.map((g) => (g.open ? 1 : 0)).join('');
  return `${s.player.x},${s.player.y};${guards};${gates}`;
}
