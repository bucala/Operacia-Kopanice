import type { GoLevel } from '../model/types';

/**
 * Hand-authored puzzle levels. Each is a small, solvable node graph that
 * introduces one idea; the `test/go.test.ts` "every level is solvable" test
 * brute-forces a winning line so a broken puzzle can never ship.
 *
 * Terrain chars: `#` wall · `.` floor · `X` exit · space/`_` void (gap).
 * Gate cells use a plain `.` in the terrain and are declared under `gates`;
 * a shut gate reads as a wall until a terminal toggles it.
 */

/** 1 — sight & the exit. A lone static sentry; simply route around its beam. */
const zacvik: GoLevel = {
  name: 'Zácvik',
  intro: 'Vyhni sa červenému zornému lúču a dostaň sa k východu.',
  width: 8,
  height: 6,
  cells: [
    '........',
    '...X....',
    '........',
    '........',
    '........',
    '........',
  ],
  start: { x: 2, y: 5, facing: 'N' },
  guards: [{ id: 's1', kind: 'sentry', x: 3, y: 3, facing: 'E', sight: 3 }],
};

/** 2 — the takedown. A rotating sentry lights the exit; slip in on its blind beat. */
const hliadka: GoLevel = {
  name: 'Hliadka',
  intro: 'Strážca sa otáča. Zlikviduj ho zozadu alebo prejdi, keď sa nepozerá.',
  width: 9,
  height: 7,
  cells: [
    '.........',
    '.........',
    '.........',
    'X........',
    '.........',
    '.........',
    '.........',
  ],
  start: { x: 8, y: 6, facing: 'N' },
  guards: [
    {
      id: 'sentry1',
      kind: 'sentry',
      x: 2,
      y: 3,
      facing: 'W',
      rotate: ['W', 'N', 'E', 'S'],
      sight: 2,
    },
    {
      id: 'patrol1',
      kind: 'patrol',
      x: 5,
      y: 0,
      facing: 'S',
      route: [
        [5, 0],
        [5, 1],
        [5, 2],
        [5, 3],
        [5, 4],
        [5, 5],
        [5, 6],
      ],
      sight: 3,
    },
  ],
};

/** 3 — Deus Ex GO hacking. A gate seals the exit niche; trip the terminal to open it. */
const terminal: GoLevel = {
  name: 'Terminál',
  intro: 'Východ je zamknutý. Aktivuj terminál, otvor bránu a prekĺzni okolo hliadky.',
  width: 9,
  height: 7,
  cells: [
    '.........',
    '.........',
    '......###',
    '........X',
    '......###',
    '.........',
    '.........',
  ],
  start: { x: 0, y: 6, facing: 'N' },
  guards: [
    {
      id: 'patrol1',
      kind: 'patrol',
      x: 2,
      y: 3,
      facing: 'E',
      route: [
        [2, 3],
        [3, 3],
        [4, 3],
        [5, 3],
      ],
      sight: 3,
    },
  ],
  terminals: [{ id: 't1', x: 1, y: 3, gate: 'gate1' }],
  gates: [{ id: 'gate1', x: 6, y: 3, open: false }],
};

export const LEVELS: readonly GoLevel[] = [zacvik, hliadka, terminal];
