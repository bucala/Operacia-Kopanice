/** Static definition of one tile variety, loaded from assets/tiles.json. */
export interface TileDef {
  id: string;
  name: string;
  /** A* traversal cost; values >= BLOCKED are treated as impassable. */
  moveCost: number;
  /** Relative footstep loudness when walking on this tile. */
  noise: number;
  walkable: boolean;
  /** Whether the tile stops line-of-sight regardless of height. */
  blocksVision: boolean;
  /** Tile height in "z" units, used by vision occlusion and rendering. */
  height: number;
  /** Optional volumetric decoration drawn by the renderer. */
  decor?: 'block' | 'conifer' | 'boulder' | 'haystack';
  colors: { top: string; left: string; right: string };
}

export interface TilesFile {
  tiles: TileDef[];
}

export const BLOCKED_COST = 900;

/** Indexed palette of tile definitions with id-based lookup. */
export class TilePalette {
  private readonly byId = new Map<string, TileDef>();
  private readonly order: TileDef[] = [];

  constructor(defs: TileDef[]) {
    for (const def of defs) {
      if (this.byId.has(def.id)) throw new Error(`Duplicate tile id '${def.id}' in palette`);
      this.byId.set(def.id, def);
      this.order.push(def);
    }
  }

  get(id: string): TileDef {
    const def = this.byId.get(id);
    if (!def) throw new Error(`Unknown tile id '${id}'`);
    return def;
  }

  has(id: string): boolean {
    return this.byId.has(id);
  }

  all(): readonly TileDef[] {
    return this.order;
  }
}
