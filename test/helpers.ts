import { TileMap, type MapFile } from '@/map/TileMap';
import { TilePalette, type TileDef } from '@/map/tiles';

/** A compact tile palette for tests with distinct, predictable costs. */
const TEST_TILES: TileDef[] = [
  { id: 'road', name: 'road', moveCost: 1, noise: 0.5, walkable: true, blocksVision: false, height: 0, colors: c() },
  { id: 'snow', name: 'snow', moveCost: 5, noise: 1.5, walkable: true, blocksVision: false, height: 0, colors: c() },
  { id: 'wall', name: 'wall', moveCost: 999, noise: 0, walkable: false, blocksVision: true, height: 2, colors: c() },
  { id: 'floor', name: 'floor', moveCost: 1, noise: 0.8, walkable: true, blocksVision: false, height: 0, colors: c() },
  { id: 'hay', name: 'hay', moveCost: 2, noise: 1, walkable: true, blocksVision: true, height: 1, colors: c() },
];

function c() {
  return { top: '#fff', left: '#aaa', right: '#ccc' };
}

const LEGEND: Record<string, string> = {
  '=': 'road',
  '.': 'snow',
  '#': 'wall',
  '+': 'floor',
  h: 'hay',
};

/** Builds a TileMap from ASCII rows using the test palette/legend. */
export function makeTestMap(rows: string[], elevation?: string[]): TileMap {
  const file: MapFile = {
    name: 'test',
    width: rows[0].length,
    height: rows.length,
    tileSize: { w: 64, h: 32, heightStep: 16 },
    legend: LEGEND,
    terrain: rows,
    ...(elevation ? { elevation } : {}),
    entities: [],
  };
  return new TileMap(file, new TilePalette(TEST_TILES));
}
