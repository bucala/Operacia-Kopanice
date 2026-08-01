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

/**
 * 4 — Ulička. A narrow alley between two Kopanice houses; a sentry sweeps the
 * corridor from its post at the far end. Time the rotation — the moment it
 * turns away, sprint all five tiles north to the exit.
 *
 * Layout (9 × 7): two solid house blocks flanking a single-cell alley at x=4.
 * Solution: N × 5 (no waiting needed when you leave on the first safe beat).
 */
const ulicka: GoLevel = {
  name: 'Ulička',
  intro: 'Preplíž sa uličkou medzi domami, kým sa strážca otočí chrbtom.',
  width: 9,
  height: 7,
  cells: [
    '####.####', // y=0  sentry post
    '####X####', // y=1  exit
    '####.####', // y=2
    '####.####', // y=3
    '####.####', // y=4
    '####.####', // y=5
    '.........', // y=6  open start area
  ],
  start: { x: 4, y: 6, facing: 'N' },
  guards: [
    {
      id: 'sentry1',
      kind: 'sentry',
      x: 4,
      y: 0,
      facing: 'S',
      rotate: ['S', 'E', 'N', 'W'],
      sight: 2,
    },
  ],
};

/**
 * 5 — Prejazd. A courtyard with a single narrow gate in the south wall; a
 * sentry patrols the north end while a patrol guard paces the inner courtyard.
 * Thread through the gate the moment the patrol clears the corridor.
 *
 * Layout (9 × 9): perimeter walls with a single gap at (4,6).
 * Solution: N × 5.
 */
const prejazd: GoLevel = {
  name: 'Prejazd',
  intro: 'Jedna brána, dva strážcovia. Vojdi do dvora v správny moment a dostaň sa na sever.',
  width: 9,
  height: 9,
  cells: [
    '#########', // y=0
    '#.......#', // y=1
    '#.......#', // y=2
    '#...X...#', // y=3  exit at (4,3)
    '#.......#', // y=4
    '#.......#', // y=5
    '####.####', // y=6  gate gap at (4,6)
    '.........', // y=7
    '.........', // y=8  start area
  ],
  start: { x: 4, y: 8, facing: 'N' },
  guards: [
    {
      id: 'sentry1',
      kind: 'sentry',
      x: 4,
      y: 1,
      facing: 'S',
      rotate: ['S', 'N'],
      sight: 3,
    },
    {
      id: 'patrol1',
      kind: 'patrol',
      x: 2,
      y: 4,
      facing: 'E',
      route: [
        [2, 4],
        [3, 4],
        [4, 4],
        [5, 4],
        [6, 4],
      ],
      sight: 2,
    },
  ],
};

/**
 * 6 — Prielom. A compact Kopanice house with a single entrance at the south.
 * The exit is sealed; a terminal on the west wall opens the gate. A sentry
 * rotates a four-beat cycle in the centre of the room — find its blind spot,
 * reach the terminal, then slip east past the open gate to freedom.
 *
 * Layout (7 × 7): outer walls with one gap at (3,4); terminal at (1,1);
 * gate at (4,1); exit at (5,1). Sentry sweeps N→E→S→W from (3,2), sight 2.
 * Solution: N N wait N W W N N E E E E  (12 moves).
 */
const prielom: GoLevel = {
  name: 'Prielom',
  intro: 'Zamknutý východ, terminál v rohu. Načasuj pohyb podľa rotácie strážcu a aktivuj bránu.',
  width: 7,
  height: 7,
  cells: [
    '#######', // y=0
    '#T...X#', // y=1  terminal at (1,1), exit at (5,1)
    '#.....#', // y=2
    '#.....#', // y=3
    '###.###', // y=4  single gap at (3,4)
    '.......', // y=5
    '...@...', // y=6  start (3,6)
  ],
  start: { x: 3, y: 6, facing: 'N' },
  guards: [
    {
      id: 'sentry1',
      kind: 'sentry',
      x: 3,
      y: 2,
      facing: 'N',
      rotate: ['N', 'E', 'S', 'W'],
      sight: 2,
    },
  ],
  terminals: [{ id: 't1', x: 1, y: 1, gate: 'gate1' }],
  gates: [{ id: 'gate1', x: 4, y: 1, open: false }],
};

export const LEVELS: readonly GoLevel[] = [zacvik, hliadka, terminal, ulicka, prejazd, prielom];
