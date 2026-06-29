import type { Vec2 } from '@/core/math/Vec2';
import { BLOCKED_COST, type TileDef, TilePalette } from './tiles';

/** Raw level data as authored in assets/maps/*.json. */
export interface MapFile {
  name: string;
  description?: string;
  width: number;
  height: number;
  tileSize: { w: number; h: number; heightStep: number };
  legend: Record<string, string>;
  /** One string per row; each char maps through `legend` to a tile id. */
  terrain: string[];
  /** One string per row of base-36 digits giving per-cell elevation. */
  elevation?: string[];
  entities: MapEntity[];
  /** Image-only decorative placements (buildings, crates), drawn by the renderer. */
  props?: MapProp[];
}

export interface MapProp {
  /** Sprite id in the sprites atlas `props` section. */
  sprite: string;
  x: number;
  y: number;
}

export interface MapEntity {
  kind: 'player' | 'guard';
  name?: string;
  x: number;
  y: number;
  facing?: string;
  patrol?: [number, number][];
}

interface Cell {
  tile: TileDef;
  elevation: number;
}

/**
 * The runtime tile grid. Owns terrain, per-cell elevation, and the queries that
 * the movement, vision, and render systems depend on.
 */
export class TileMap {
  readonly width: number;
  readonly height: number;
  readonly name: string;
  readonly entities: MapEntity[];
  readonly props: MapProp[];
  private readonly cells: Cell[];

  constructor(file: MapFile, palette: TilePalette) {
    this.width = file.width;
    this.height = file.height;
    this.name = file.name;
    this.entities = file.entities;
    this.props = file.props ?? [];
    this.cells = new Array(file.width * file.height);

    for (let y = 0; y < file.height; y++) {
      const row = file.terrain[y] ?? '';
      const elevRow = file.elevation?.[y] ?? '';
      for (let x = 0; x < file.width; x++) {
        const ch = row[x] ?? '.';
        const tileId = file.legend[ch];
        if (!tileId) throw new Error(`Legend has no entry for '${ch}' at ${x},${y}`);
        const tile = palette.get(tileId);
        const elevChar = elevRow[x];
        const elevation = elevChar ? parseInt(elevChar, 36) : tile.height;
        this.cells[y * file.width + x] = { tile, elevation };
      }
    }
  }

  inBounds(x: number, y: number): boolean {
    return x >= 0 && y >= 0 && x < this.width && y < this.height;
  }

  private cell(x: number, y: number): Cell {
    return this.cells[y * this.width + x];
  }

  tileAt(x: number, y: number): TileDef {
    return this.cell(x, y).tile;
  }

  /** Combined occluder height: max of terrain elevation and the tile's own height. */
  heightAt(x: number, y: number): number {
    if (!this.inBounds(x, y)) return 99;
    const c = this.cell(x, y);
    return Math.max(c.elevation, c.tile.height);
  }

  isWalkable(x: number, y: number): boolean {
    return this.inBounds(x, y) && this.cell(x, y).tile.walkable;
  }

  blocksVision(x: number, y: number): boolean {
    return !this.inBounds(x, y) || this.cell(x, y).tile.blocksVision;
  }

  /** A* movement cost to enter (x, y); BLOCKED_COST or more = impassable. */
  moveCost(x: number, y: number): number {
    if (!this.inBounds(x, y)) return BLOCKED_COST;
    const c = this.cell(x, y);
    return c.tile.walkable ? c.tile.moveCost : BLOCKED_COST;
  }

  noiseAt(x: number, y: number): number {
    return this.inBounds(x, y) ? this.cell(x, y).tile.noise : 0;
  }

  /**
   * Walkable 8-connected neighbours. Diagonal moves are rejected when they would
   * "cut the corner" past an impassable tile.
   */
  neighbors(x: number, y: number): Vec2[] {
    const result: Vec2[] = [];
    for (let dy = -1; dy <= 1; dy++) {
      for (let dx = -1; dx <= 1; dx++) {
        if (dx === 0 && dy === 0) continue;
        const nx = x + dx;
        const ny = y + dy;
        if (!this.isWalkable(nx, ny)) continue;
        if (dx !== 0 && dy !== 0) {
          // Disallow squeezing diagonally between two blockers.
          if (!this.isWalkable(x + dx, y) || !this.isWalkable(x, y + dy)) continue;
        }
        result.push({ x: nx, y: ny });
      }
    }
    return result;
  }

  forEach(fn: (x: number, y: number, tile: TileDef, elevation: number) => void): void {
    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        const c = this.cell(x, y);
        fn(x, y, c.tile, c.elevation);
      }
    }
  }
}
