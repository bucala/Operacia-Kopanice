#!/usr/bin/env node
/**
 * Authoring tool for the "Kopanice — horská usadlosť" level.
 *
 * Builds a 24×24 grid programmatically (so tile rows are always the right
 * width), stamps terrain features and elevation, and writes a readable JSON
 * map to public/assets/maps/kopanice.json. Re-run with `node scripts/genmap.mjs`
 * after tweaking the layout below.
 */
import { writeFileSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

const W = 24;
const H = 24;

// Legend maps single chars to tile ids from tiles.json.
const legend = {
  '.': 'snow',
  ',': 'grass',
  '=': 'road',
  '~': 'mud',
  '#': 'wall',
  '+': 'floor',
  T: 'tree',
  R: 'rock',
  W: 'water',
  h: 'hay',
};

const terrain = Array.from({ length: H }, () => Array.from({ length: W }, () => '.'));
const elevation = Array.from({ length: H }, () => Array.from({ length: W }, () => 0));

const set = (x, y, ch) => {
  if (x >= 0 && x < W && y >= 0 && y < H) terrain[y][x] = ch;
};
const rect = (x0, y0, x1, y1, ch) => {
  for (let y = y0; y <= y1; y++) for (let x = x0; x <= x1; x++) set(x, y, ch);
};

// --- Base terrain patches ---------------------------------------------------
rect(0, 0, W - 1, 4, ','); // grassy upper meadow
rect(0, 16, W - 1, H - 1, '.'); // snow field to the south

// Forested borders for cover and to enclose the play space.
for (let x = 0; x < W; x++) {
  set(x, 0, 'T');
  set(x, H - 1, 'T');
}
for (let y = 0; y < H; y++) {
  set(0, y, 'T');
  set(W - 1, y, 'T');
}

// A rocky ridge (elevated, blocks vision) running across the north-east.
for (let x = 13; x <= 20; x++) {
  set(x, 3, 'R');
  elevation[3][x] = 2;
}
set(19, 4, 'R');
elevation[4][19] = 2;

// --- The winding mountain road (the cheap path through expensive snow) ------
const road = [
  [2, 22], [3, 21], [4, 20], [5, 19], [6, 18], [7, 17], [8, 16],
  [9, 15], [10, 14], [11, 13], [12, 12], [12, 11], [12, 10],
  [13, 9], [14, 8], [15, 7], [16, 6], [17, 5], [18, 4], [19, 3],
];
for (const [x, y] of road) set(x, y, '=');

// --- A stream with a single muddy ford --------------------------------------
for (let y = 5; y <= 15; y++) set(4, y, 'W');
set(4, 10, '~'); // ford

// --- The objective: a stone cottage near the centre -------------------------
rect(14, 12, 20, 18, '#'); // walls
rect(15, 13, 19, 17, '+'); // interior floor
set(17, 18, '+'); // doorway on the south wall
for (let y = 13; y <= 17; y++) for (let x = 15; x <= 19; x++) elevation[y][x] = 0;
for (let y = 12; y <= 18; y++) { elevation[y][14] = 2; elevation[y][20] = 2; }
for (let x = 14; x <= 20; x++) { elevation[12][x] = 2; elevation[18][x] = 2; }
set(17, 18, '+');
elevation[18][17] = 0;

// --- Hay piles for soft cover (block vision, but walkable & noisy) ----------
for (const [x, y] of [[8, 19], [9, 19], [11, 8], [12, 20], [21, 14]]) {
  set(x, y, 'h');
  elevation[y][x] = 1;
}

// Scattered trees inside the field for cover.
for (const [x, y] of [[6, 6], [7, 12], [9, 9], [18, 21], [21, 8], [10, 21], [3, 14]]) {
  set(x, y, 'T');
  elevation[y][x] = 3;
}

// Elevation for the rocky ridge tiles already blocking vision.
for (let x = 13; x <= 20; x++) if (terrain[3][x] === 'R') elevation[3][x] = 2;

// --- Entities ---------------------------------------------------------------
const entities = [
  { kind: 'player', x: 2, y: 22, facing: 'N' },
  {
    kind: 'guard',
    name: 'Stráž — Dvor',
    x: 16,
    y: 20,
    facing: 'N',
    patrol: [[16, 20], [10, 20], [10, 16], [16, 16]],
  },
  {
    kind: 'guard',
    name: 'Stráž — Chodník',
    x: 12,
    y: 10,
    facing: 'S',
    patrol: [[12, 10], [12, 14], [8, 16], [12, 14]],
  },
  {
    kind: 'guard',
    name: 'Stráž — Cottage',
    x: 17,
    y: 15,
    facing: 'S',
    patrol: [[17, 15], [17, 13], [15, 15], [19, 15]],
  },
];

// Image-only props (rendered when the matching art is present; see docs/ASSETS.md).
// The cottage prop sits over the tile-built objective; the crate is the goal cache.
const props = [
  { sprite: 'cottage', x: 17, y: 15 },
  { sprite: 'tnt_crate', x: 17, y: 16 },
];

const map = {
  $schema: '../schema/map.schema.json',
  name: 'Kopanice — horská usadlosť',
  description:
    'Úvodná misia: prejdi zasneženým údolím, vyhni sa hliadkam a dostaň sa do kamennej chalupy.',
  width: W,
  height: H,
  tileSize: { w: 64, h: 32, heightStep: 16 },
  legend,
  terrain: terrain.map((row) => row.join('')),
  elevation: elevation.map((row) => row.map((n) => n.toString(36)).join('')),
  entities,
  props,
};

const here = dirname(fileURLToPath(import.meta.url));
const out = resolve(here, '..', 'public', 'assets', 'maps', 'kopanice.json');
mkdirSync(dirname(out), { recursive: true });
writeFileSync(out, JSON.stringify(map, null, 2) + '\n');

// Sanity check: every terrain row must be exactly W chars.
for (const [i, row] of map.terrain.entries()) {
  if (row.length !== W) throw new Error(`row ${i} has width ${row.length}, expected ${W}`);
}
console.log(`Wrote ${out} (${W}×${H}, ${entities.length} entities)`);
