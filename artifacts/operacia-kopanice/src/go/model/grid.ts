import type { CellKind, DecorationSpec, GoLevel } from './types';

/**
 * Static terrain of a level as a flat grid of {@link CellKind}. Gates and
 * entities are *not* baked in here — they live in the mutable {@link GoState}
 * because they change during play. The grid only holds what never moves.
 */
export class GoGrid {
  readonly width: number;
  readonly height: number;
  readonly decorations: readonly DecorationSpec[];
  private readonly kinds: CellKind[];

  constructor(level: GoLevel) {
    this.width = level.width;
    this.height = level.height;
    this.decorations = level.decorations ?? [];
    this.kinds = new Array(level.width * level.height).fill('void');

    for (let y = 0; y < level.height; y++) {
      const row = level.cells[y] ?? '';
      for (let x = 0; x < level.width; x++) {
        this.kinds[y * level.width + x] = charToKind(row[x]);
      }
    }

    // Entity home cells are implicitly walkable floor, so a level author never
    // has to keep the terrain and the spec grid in perfect sync.
    this.ensureFloor(level.start.x, level.start.y);
    for (const g of level.guards) {
      this.ensureFloor(g.x, g.y);
      for (const [rx, ry] of g.route ?? []) this.ensureFloor(rx, ry);
    }
    for (const t of level.terminals ?? []) this.ensureFloor(t.x, t.y);
  }

  private ensureFloor(x: number, y: number): void {
    if (!this.inBounds(x, y)) return;
    const i = y * this.width + x;
    if (this.kinds[i] === 'void') this.kinds[i] = 'floor';
  }

  inBounds(x: number, y: number): boolean {
    return x >= 0 && y >= 0 && x < this.width && y < this.height;
  }

  kindAt(x: number, y: number): CellKind {
    if (!this.inBounds(x, y)) return 'void';
    return this.kinds[y * this.width + x];
  }
}

function charToKind(ch: string | undefined): CellKind {
  switch (ch) {
    case '#':
      return 'wall';
    case 'X':
      return 'exit';
    case '=':
      return 'road';
    case '-':
      return 'plank';
    case '~':
      return 'mud';
    case 'T':
      return 'tree';
    case 'R':
      return 'rock';
    case '.':
      return 'floor';
    default:
      return 'void';
  }
}
