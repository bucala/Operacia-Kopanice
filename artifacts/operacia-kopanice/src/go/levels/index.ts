import type { GoLevel } from '../model/types';

/**
 * Hand-authored puzzle levels. Each is a small, solvable node graph that
 * introduces one idea; the `test/go.test.ts` "every level is solvable" test
 * brute-forces a winning line so a broken puzzle can never ship.
 *
 * Terrain chars: `#` house wall · `.` snow · `=` road · `-` planks · `~` mud
 * · `T` tree · `R` rock · `X` exit · space/`_` void (gap).
 * Gate cells use a plain `.` in the terrain and are declared under `gates`;
 * a shut gate reads as a wall until a terminal toggles it.
 */

/** 1 — sight & the exit. A lone static sentry; simply route around its beam. */
const zacvik: GoLevel = {
  name: 'Zácvik',
  intro: 'Vyhni sa červenému zornému lúču. Generátor odvráti pozornosť na jeden ťah.',
  width: 8,
  height: 6,
  cells: [
    'TT......',
    '...X....',
    '..====..',
    '..=..=..',
    '..=..=TT',
    '...~~...',
  ],
  start: { x: 2, y: 5, facing: 'N' },
  guards: [{ id: 's1', kind: 'sentry', x: 3, y: 3, facing: 'E', sight: 3, variant: 'sniper' }],
  decorations: [
    { kind: 'house1', x: 0, y: 0, scale: 1.8 },
    { kind: 'house2', x: 7, y: 4, scale: 1.7 },
    { kind: 'fence', x: 1, y: 4 },
  ],
  distractions: [
    { id: 'generator-zacvik', kind: 'generator', x: 2, y: 4, range: 2, direction: 'S' },
  ],
};

/** 2 — the takedown. A rotating sentry lights the exit; slip in on its blind beat. */
const hliadka: GoLevel = {
  name: 'Hliadka',
  intro: 'Strážca sa otáča. Zlikviduj ho zozadu alebo prejdi, keď sa nepozerá.',
  width: 9,
  height: 7,
  cells: [
    'TT...TT..',
    '..===....',
    '..=.=....',
    'X.=.=....',
    '..=.=..TT',
    '..=.=....',
    '...~.....',
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
      variant: 'machinegunner',
    },
  ],
  decorations: [
    { kind: 'house2', x: 6, y: 0, scale: 1.7 },
    { kind: 'house1', x: 8, y: 4, scale: 1.55 },
    { kind: 'crate', x: 6, y: 5 },
  ],
  distractions: [
    { id: 'generator1', kind: 'generator', x: 7, y: 5, range: 3, direction: 'N' },
  ],
};

/** 3 — Deus Ex GO hacking. A gate seals the exit niche; trip the terminal to open it. */
const terminal: GoLevel = {
  name: 'Terminál',
  intro: 'Východ je zamknutý. Aktivuj terminál, otvor bránu a prekĺzni okolo hliadky.',
  width: 9,
  height: 7,
  cells: [
    'TT..=....',
    '....=....',
    '......###',
    '........X',
    '......###',
    '....=....',
    '....=..TT',
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
      variant: 'machinegunner',
    },
  ],
  terminals: [{ id: 't1', x: 1, y: 3, gate: 'gate1' }],
  gates: [{ id: 'gate1', x: 6, y: 3, open: false }],
  decorations: [
    { kind: 'house1', x: 7, y: 2, scale: 1.75 },
    { kind: 'house2', x: 7, y: 4, scale: 1.7 },
    { kind: 'fence', x: 3, y: 0, scale: 0.8 },
  ],
};

/**
 * 4 — Ulička. A narrow alley between two single-file Kopanice house fronts; a
 * sentry sweeps the corridor from its post at the far end. Time the rotation
 * — the moment it turns away, sprint all five tiles north to the exit.
 *
 * Layout (9 × 7): a one-cell-wide alley at x=4, flanked on each side by a
 * single wall column (the house fronts, x=3 and x=5) and then a wooded dead
 * zone (trees/rocks) filling the outer border out to the map edge — not a
 * solid block of tiled houses.
 * Solution: N × 5 (no waiting needed when you leave on the first safe beat).
 */
const ulicka: GoLevel = {
  name: 'Ulička',
  intro: 'Preplíž sa uličkou medzi domami, kým sa strážca otočí chrbtom.',
  width: 9,
  height: 7,
  cells: [
    'TRT#.#TRT', // y=0  sentry post at x=4
    'RTR#X#RTR', // y=1  exit at x=4
    'TRT#.#TRT', // y=2
    'RTR#.#RTR', // y=3
    'TRT#.#TRT', // y=4
    'RTR#.#RTR', // y=5
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
  decorations: [
    { kind: 'house1', x: 1, y: 2, scale: 1.65 },
    { kind: 'house2', x: 7, y: 3, scale: 1.7 },
    { kind: 'fence', x: 5, y: 6, scale: 0.7 },
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
      variant: 'sniper',
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
      variant: 'machinegunner',
    },
  ],
  decorations: [
    { kind: 'house2', x: 0, y: 0, scale: 1.65 },
    { kind: 'house1', x: 8, y: 0, scale: 1.65 },
    { kind: 'fence', x: 0, y: 7, scale: 0.72 },
    { kind: 'crate', x: 7, y: 7 },
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
    '#....X#', // y=1  terminal at (1,1), exit at (5,1)
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
  decorations: [
    { kind: 'house1', x: 0, y: 0, scale: 1.55 },
    { kind: 'house2', x: 6, y: 0, scale: 1.6 },
    { kind: 'crate', x: 1, y: 5 },
    { kind: 'fence', x: 5, y: 5 },
  ],
};

/**
 * 7 — Kameň. A narrow Kopanice alley sealed by single house fronts on both
 * sides. A sentry faces south — directly toward the player — and blocks the
 * only path to the exit. Standing on the alley mouth and facing north throws
 * the stone one cell ahead, distracting the guard into looking east. With its
 * gaze turned, slip north and take it down for a clear run to the exit.
 *
 * Requires the stone distraction: guard faces toward the player's only approach
 * (head-on takedown impossible). The alley (x=2) is flanked by one wall column
 * on each side (x=1, x=3) and a wooded dead zone (trees/rocks) beyond that —
 * not a solid tiled block of houses.
 * Solution: N N [throw stone] N [takedown] N N (6 actions).
 */
const kamen: GoLevel = {
  name: 'Kameň',
  intro: 'Hoď kameň smerom, ktorým sa pozeráš — odvedie stráž z cesty.',
  width: 7,
  height: 8,
  cells: [
    '.......',  // y=0
    '..X....',  // y=1  exit at (2,1)
    '.......',  // y=2
    'R#.#TRT', // y=3  alley at x=2 (guard)
    'T#.#RTR', // y=4  alley at x=2 (stone)
    'R#.#TRT', // y=5  alley at x=2 (player throws from here)
    '.......',  // y=6
    '.......',  // y=7  start area
  ],
  start: { x: 2, y: 7, facing: 'N' },
  guards: [
    {
      id: 'sentry1',
      kind: 'sentry',
      x: 2,
      y: 3,
      facing: 'S',
      sight: 1,
    },
  ],
  decorations: [
    { kind: 'house1', x: 0, y: 2, scale: 1.55 },
    { kind: 'house2', x: 6, y: 3, scale: 1.6 },
    { kind: 'fence', x: 1, y: 7, scale: 0.65 },
  ],
  distractions: [
    { id: 'stone1', kind: 'stone', x: 2, y: 4, range: 2, direction: 'E' },
  ],
};

/**
 * 8 — Výpadok. A narrow three-cell alley hemmed in by solid walls; a static
 * sentry at the far end faces south and watches the cell directly ahead
 * (sight=1) — exactly where the player must walk to reach the exit. There is
 * no rotation, no flanking path, no alternative. The generator lies two cells
 * south of the guard; activating it turns the sentry north for one beat,
 * opening a window to sprint past and take it down from behind.
 *
 * Without the generator the BFS finds no winning line within MAX_TURNS turns.
 *
 * Layout (7 × 8): a single-cell alley at x=2, flanked by one wall column each
 * side (x=1, x=3) for rows y∈{3,4,5}, with a wooded dead zone (trees/rocks)
 * beyond that out to the map edge — not a solid tiled block of houses.
 * Solution: W N N N [activate generator] N N [takedown] N N  (9 actions).
 */
const vypadok: GoLevel = {
  name: 'Výpadok',
  intro:
    'Strážca blokuje jedinú cestu. Aktivuj generátor — na jeden ťah odvráti jeho pohľad a ty môžeš prejsť.',
  width: 7,
  height: 8,
  cells: [
    '.......', // y=0
    '..X....', // y=1  exit at (2,1)
    '.......', // y=2
    'R#.#TRT', // y=3  guard at (2,3)
    'T#.#RTR', // y=4
    'R#.#TRT', // y=5  generator at (2,5)
    '.......', // y=6
    '.......', // y=7  start area
  ],
  start: { x: 3, y: 7, facing: 'N' },
  guards: [
    {
      id: 'sentry1',
      kind: 'sentry',
      x: 2,
      y: 3,
      facing: 'S',
      sight: 1,
    },
  ],
  decorations: [
    { kind: 'house1', x: 0, y: 2, scale: 1.55 },
    { kind: 'house2', x: 5, y: 3, scale: 1.6 },
    { kind: 'fence', x: 4, y: 7, scale: 0.65 },
  ],
  distractions: [
    { id: 'generator1', kind: 'generator', x: 2, y: 5, range: 3, direction: 'N' },
  ],
};

export const LEVELS: readonly GoLevel[] = [
  zacvik,
  hliadka,
  terminal,
  ulicka,
  prejazd,
  prielom,
  kamen,
  vypadok,
];
