import { Camera } from '@/core/Camera';
import { depthKey, gridToScreen, type IsoConfig } from '@/core/math/iso';
import type { Vec2 } from '@/core/math/Vec2';
import { GoGrid } from './model/grid';
import { key } from './model/logic';
import { DIR_VEC, type CellKind, type DecorationSpec, type Dir, type GoState, type GuardVariant } from './model/types';
import { SpriteCache } from './SpriteCache';

/** Everything the renderer needs for one frame, assembled by {@link GoGame}. */
export interface RenderModel {
  grid: GoGrid;
  state: GoState;
  /** Interpolated (visual) player position + facing. */
  player: { x: number; y: number; facing: Dir };
  guards: GuardView[];
  /** Lit (lethal) cells, packed "x,y". */
  danger: Set<string>;
  /** Legal step-target cells to highlight for the player. */
  legal: Vec2[];
  /** Guard ids highlighted from the enemy panel. */
  highlightedGuardIds: Set<string>;
  hover: Vec2 | null;
}

export interface GuardView {
  id: string;
  x: number;
  y: number;
  facing: Dir;
  alive: boolean;
  /** 1 = solid, 0 = fully faded (used to dissolve a taken-down guard). */
  fade: number;
  /** Which sprite to render for this guard. */
  variant?: GuardVariant;
}

interface DrawItem {
  depth: number;
  draw: () => void;
}

// ---------------------------------------------------------------------------
// Winter / SNP colour palette
// ---------------------------------------------------------------------------
const COL = {
  // Snow ground
  floorTop: '#d8e8f4',
  floorLeft: '#8ca8c0',
  floorRight: '#a8c0d4',
  edge: 'rgba(40,70,100,0.45)',
  roadTop: '#78808a',
  roadLeft: '#4b535d',
  roadRight: '#5d6670',
  plankTop: '#947657',
  plankLeft: '#60452f',
  plankRight: '#75563b',
  mudTop: '#655348',
  mudLeft: '#3d3029',
  mudRight: '#4e3d33',

  // Exit tile — amber lantern glow
  exitTop: '#c8a030',
  exitGlow: '#f0cc58',

  // Wall cube fallback (hidden behind house sprite when image loads)
  wallTop: '#7a8a96',
  wallLeft: '#3e4e5c',
  wallRight: '#526070',

  // Gate
  gateBar: '#c58a3a',
  gateFrame: 'rgba(197,138,58,0.5)',

  // Terminal screen
  terminal: '#39c6d6',

  // Player — partisan green coat
  player: '#5a9068',
  playerDark: '#38604a',

  // Guard — Wehrmacht field-grey / danger red accent
  guard: '#c05048',
  guardDark: '#8a2828',

  // Danger sight-cone overlay
  danger: 'rgba(210,50,40,0.28)',

  // Legal-move marker
  legal: '#90d8ff',
};

// Sprite paths — use Vite's BASE_URL so they work regardless of deploy sub-path.
const _base = import.meta.env.BASE_URL.replace(/\/$/, '');
const SPRITE = {
  house1: `${_base}/assets/sprites/house1.png`,
  house2: `${_base}/assets/sprites/house2.png`,
  trees: `${_base}/assets/sprites/trees.png`,
  /** Odbojár / Resister — the player character. */
  playerChar: `${_base}/assets/sprites/player.png`,
  /** German Officer — default enemy guard sprite. */
  guardOfficer: `${_base}/assets/sprites/guard.png`,
  /** Sniper — long-range threat, white winter cloak. */
  guardSniper: `${_base}/assets/sprites/guard-sniper.png`,
  /** Machine Gunner — wide-threat patrol, holds LMG. */
  guardMachinegunner: `${_base}/assets/sprites/guard-machinegunner.png`,
};

/**
 * Isometric renderer for the GO puzzle.
 *
 * Visual upgrade: snow-white tile surfaces, photorealistic house sprites on
 * wall nodes, partisan-green player figure, atmospheric winter sky gradient.
 * All sprites degrade gracefully to geometric shapes when images are not yet
 * loaded.
 */
export class GoRenderer {
  private readonly sprites: SpriteCache;

  constructor(
    private readonly ctx: CanvasRenderingContext2D,
    private readonly canvas: HTMLCanvasElement,
    private readonly cam: Camera,
    private readonly iso: IsoConfig,
    sprites?: SpriteCache,
  ) {
    this.sprites = sprites ?? new SpriteCache();
    // Kick off image loading; renders work even while loading (fallback shapes).
    void this.sprites.preload(Object.values(SPRITE));
  }

  /** Fit the whole board on screen with a margin (called per level / resize). */
  frameBoard(grid: GoGrid): void {
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    for (const [gx, gy] of [
      [0, 0],
      [grid.width - 1, 0],
      [0, grid.height - 1],
      [grid.width - 1, grid.height - 1],
    ]) {
      const p = gridToScreen(gx, gy, 0, this.iso);
      minX = Math.min(minX, p.x);
      minY = Math.min(minY, p.y);
      maxX = Math.max(maxX, p.x);
      maxY = Math.max(maxY, p.y);
    }
    const marginX = this.iso.tileWidth * 2;
    const marginY = this.iso.tileHeight * 4;
    const spanX = maxX - minX + marginX * 2;
    const spanY = maxY - minY + marginY * 2;
    this.cam.target = { x: (minX + maxX) / 2, y: (minY + maxY) / 2 };
    this.cam.zoom = Math.min(this.canvas.width / spanX, this.canvas.height / spanY, 1.6);
  }

  render(m: RenderModel): void {
    this.clear();
    const items: DrawItem[] = [];
    const grid = m.grid;

    for (let y = 0; y < grid.height; y++) {
      for (let x = 0; x < grid.width; x++) {
        const kind = grid.kindAt(x, y);
        if (kind === 'void') continue;
        const gate = m.state.gates.find((g) => g.x === x && g.y === y);
        const terminal = m.state.terminals.find((t) => t.x === x && t.y === y);
        const lit = m.danger.has(key(x, y));
        items.push({
          depth: depthKey(x, y, 0),
          draw: () => this.drawCell(x, y, kind, gate?.open, !!gate, !!terminal, lit),
        });
      }
    }
    for (const decor of grid.decorations) {
      items.push({
        depth: depthKey(decor.x, decor.y, decor.layer ?? 0.48),
        draw: () => this.drawDecoration(decor),
      });
    }

    // Player.
    items.push({
      depth: depthKey(m.player.x, m.player.y, 0.6),
      draw: () =>
        this.drawFigure(m.player.x, m.player.y, m.player.facing, COL.player, COL.playerDark, 1),
    });
    // Guards.
    for (const g of m.guards) {
      if (g.fade <= 0.01) continue;
      items.push({
        depth: depthKey(g.x, g.y, 0.6),
        draw: () =>
          this.drawFigure(
            g.x,
            g.y,
            g.facing,
            COL.guard,
            COL.guardDark,
            g.fade,
            true,
            g.variant,
            m.highlightedGuardIds.has(g.id),
          ),
      });
    }

    items.sort((a, b) => a.depth - b.depth);
    for (const it of items) it.draw();

    // Overlays drawn on top.
    for (const cell of m.legal) this.drawLegalMarker(cell.x, cell.y);
    if (m.hover) this.drawHover(m.hover.x, m.hover.y);
  }

  // ---------------------------------------------------------------------------
  // Background
  // ---------------------------------------------------------------------------

  private clear(): void {
    const { width, height } = this.canvas;
    // Deep winter night sky gradient.
    const sky = this.ctx.createLinearGradient(0, 0, 0, height);
    sky.addColorStop(0, '#0b1320');
    sky.addColorStop(0.45, '#101826');
    sky.addColorStop(1, '#090e16');
    this.ctx.fillStyle = sky;
    this.ctx.fillRect(0, 0, width, height);

    // Subtle ambient snow haze around the board centre.
    const cx = width / 2;
    const cy = height * 0.48;
    const r = Math.max(width, height) * 0.7;
    const haze = this.ctx.createRadialGradient(cx, cy, 0, cx, cy, r);
    haze.addColorStop(0, 'rgba(160,200,240,0.07)');
    haze.addColorStop(0.55, 'rgba(100,150,200,0.03)');
    haze.addColorStop(1, 'rgba(0,0,0,0)');
    this.ctx.fillStyle = haze;
    this.ctx.fillRect(0, 0, width, height);
  }

  // ---------------------------------------------------------------------------
  // Tile drawing
  // ---------------------------------------------------------------------------

  private drawCell(
    x: number,
    y: number,
    kind: CellKind,
    gateOpen: boolean | undefined,
    isGate: boolean,
    isTerminal: boolean,
    lit: boolean,
  ): void {
    const zoom = this.cam.zoom;
    const halfW = (this.iso.tileWidth / 2) * zoom;
    const halfH = (this.iso.tileHeight / 2) * zoom;
    const closedGate = isGate && !gateOpen;
    const isWall = kind === 'wall' || kind === 'tree' || kind === 'rock' || closedGate;
    const z = isWall ? 1.4 : 0.35;

    const top = this.center(x, y, z);
    const base = this.center(x, y, 0);

    let topColor = COL.floorTop;
    let leftColor = COL.floorLeft;
    let rightColor = COL.floorRight;

    if (kind === 'exit') {
      topColor = COL.exitTop;
      leftColor = '#8c6820';
      rightColor = '#a87c28';
    }
    if (kind === 'road') {
      topColor = COL.roadTop;
      leftColor = COL.roadLeft;
      rightColor = COL.roadRight;
    }
    if (kind === 'plank') {
      topColor = COL.plankTop;
      leftColor = COL.plankLeft;
      rightColor = COL.plankRight;
    }
    if (kind === 'mud') {
      topColor = COL.mudTop;
      leftColor = COL.mudLeft;
      rightColor = COL.mudRight;
    }
    if (kind === 'wall' || kind === 'tree' || kind === 'rock') {
      topColor = COL.wallTop;
      leftColor = COL.wallLeft;
      rightColor = COL.wallRight;
    }
    if (closedGate) {
      topColor = COL.gateBar;
      leftColor = COL.wallLeft;
      rightColor = COL.wallRight;
    }

    // Extruded sides for volume.
    this.ctx.fillStyle = leftColor;
    this.poly([
      { x: top.x - halfW, y: top.y },
      { x: top.x, y: top.y + halfH },
      { x: base.x, y: base.y + halfH },
      { x: base.x - halfW, y: base.y },
    ]);
    this.ctx.fillStyle = rightColor;
    this.poly([
      { x: top.x, y: top.y + halfH },
      { x: top.x + halfW, y: top.y },
      { x: base.x + halfW, y: base.y },
      { x: base.x, y: base.y + halfH },
    ]);

    // Top face.
    this.ctx.fillStyle = topColor;
    this.diamond(top.x, top.y, halfW, halfH);
    this.ctx.strokeStyle = COL.edge;
    this.ctx.lineWidth = 1;
    this.diamondPath(top.x, top.y, halfW, halfH);
    this.ctx.stroke();

    // Subtle snow sparkle on floor tiles (tiny white highlight at the peak).
    if (!isWall) {
      this.ctx.fillStyle = 'rgba(255,255,255,0.18)';
      this.diamond(top.x, top.y - halfH * 0.25, halfW * 0.35, halfH * 0.22);
    }
    if (kind === 'road' || kind === 'plank') this.drawSurfaceDetail(top, halfW, halfH, kind);

    // Danger tint.
    if (lit && !isWall) {
      this.ctx.fillStyle = COL.danger;
      this.diamond(top.x, top.y, halfW, halfH);
    }

    if (kind === 'exit') this.drawExitMark(top, halfW, halfH);
    if (isGate && gateOpen) this.drawOpenGate(top, halfW, halfH);
    if (closedGate) this.drawGateBars(top, halfW, halfH);
    if (isTerminal) this.drawTerminal(top, halfW, halfH);

    // House sprite on wall tiles — drawn after the cube geometry so its
    // transparent pixels reveal the grey block beneath it.
    if (kind === 'wall') {
      this.drawHouseSprite(x, y);
    }
    if (kind === 'tree') this.drawTreeSprite(x, y);
    if (kind === 'rock') this.drawRock(x, y);
  }

  /**
   * Draw the photorealistic house sprite on a wall tile, alternating between
   * house1 and house2 for visual variety.
   */
  private drawHouseSprite(gx: number, gy: number): void {
    // Alternate houses so a row of walls doesn't look uniform.
    const src = (gx + gy) % 2 === 0 ? SPRITE.house1 : SPRITE.house2;
    const img = this.sprites.get(src);
    if (!img) return; // still loading — fallback cube is already drawn

    const zoom = this.cam.zoom;
    const tileW = this.iso.tileWidth * zoom;         // e.g. 64 * zoom
    const halfH = (this.iso.tileHeight / 2) * zoom;  // 16 * zoom

    // Scale: house occupies about 2.8× tile width
    const spriteW = tileW * 2.8;
    const ratio = img.naturalHeight / img.naturalWidth;
    const spriteH = spriteW * ratio;

    // Anchor: the base slab bottom of the house sits at the tile base level.
    // For these renders the slab bottom is ~90 % down the image.
    const anchorY = 0.90;
    const base = this.center(gx, gy, 0);
    const tileBaseY = base.y + halfH; // bottom of diamond at z=0

    const drawX = base.x - spriteW / 2;
    const drawY = tileBaseY - spriteH * anchorY;

    this.ctx.drawImage(img, drawX, drawY, spriteW, spriteH);
  }

  private drawSurfaceDetail(top: Vec2, halfW: number, halfH: number, kind: 'road' | 'plank'): void {
    this.ctx.save();
    this.ctx.strokeStyle = kind === 'road' ? 'rgba(218,225,230,0.22)' : 'rgba(57,37,23,0.48)';
    this.ctx.lineWidth = Math.max(1, this.cam.zoom);
    const count = kind === 'road' ? 3 : 4;
    for (let i = 1; i <= count; i++) {
      const t = i / (count + 1);
      this.ctx.beginPath();
      this.ctx.moveTo(top.x - halfW * (1 - t), top.y - halfH * (1 - t));
      this.ctx.lineTo(top.x + halfW * t, top.y + halfH * t);
      this.ctx.stroke();
    }
    this.ctx.restore();
  }

  private drawTreeSprite(gx: number, gy: number): void {
    const img = this.sprites.get(SPRITE.trees);
    if (!img) return;
    const base = this.center(gx, gy, 0);
    const tileW = this.iso.tileWidth * this.cam.zoom;
    const spriteW = tileW * 1.75;
    const crop = 0.28;
    const srcW = img.naturalWidth * crop;
    const srcH = img.naturalHeight * 0.52;
    this.ctx.drawImage(
      img,
      (gx + gy) % 2 ? 0 : img.naturalWidth * crop,
      0,
      srcW,
      srcH,
      base.x - spriteW / 2,
      base.y - spriteW * 1.3,
      spriteW,
      spriteW * 1.45,
    );
  }

  private drawRock(gx: number, gy: number): void {
    const base = this.center(gx, gy, 0);
    const size = this.iso.tileWidth * this.cam.zoom * 0.36;
    this.ctx.fillStyle = '#69717a';
    this.blob(base.x, base.y, size, size * 0.45);
    this.ctx.fillStyle = 'rgba(255,255,255,0.35)';
    this.blob(base.x - size * 0.16, base.y - size * 0.16, size * 0.48, size * 0.2);
  }

  private drawDecoration(decor: DecorationSpec): void {
    if (decor.kind === 'house1' || decor.kind === 'house2') {
      this.drawDecorationHouse(decor);
      return;
    }
    if (decor.kind === 'tree') {
      this.drawTreeSprite(decor.x, decor.y);
      return;
    }
    const base = this.center(decor.x, decor.y, 0);
    const size = this.iso.tileWidth * this.cam.zoom * (decor.scale ?? 0.38);
    if (decor.kind === 'fence') {
      this.ctx.strokeStyle = '#725238';
      this.ctx.lineWidth = Math.max(2, this.cam.zoom * 2);
      this.ctx.beginPath();
      this.ctx.moveTo(base.x - size, base.y);
      this.ctx.lineTo(base.x + size, base.y);
      this.ctx.stroke();
      return;
    }
    this.ctx.fillStyle = '#7b512f';
    this.ctx.fillRect(base.x - size / 2, base.y - size * 0.55, size, size * 0.55);
    this.ctx.strokeStyle = '#c1d7e7';
    this.ctx.strokeRect(base.x - size / 2, base.y - size * 0.55, size, size * 0.55);
  }

  private drawDecorationHouse(decor: DecorationSpec): void {
    const src = decor.kind === 'house1' ? SPRITE.house1 : SPRITE.house2;
    const img = this.sprites.get(src);
    if (!img) return;
    const base = this.center(decor.x, decor.y, 0);
    const width = this.iso.tileWidth * this.cam.zoom * (decor.scale ?? 1.6);
    const height = width * (img.naturalHeight / img.naturalWidth);
    this.ctx.drawImage(img, base.x - width / 2, base.y - height * 0.9, width, height);
  }

  // ---------------------------------------------------------------------------
  // Tile decorations
  // ---------------------------------------------------------------------------

  private drawExitMark(top: Vec2, halfW: number, halfH: number): void {
    // Glowing amber exit beacon.
    this.ctx.shadowColor = COL.exitGlow;
    this.ctx.shadowBlur = 12 * this.cam.zoom;
    this.ctx.strokeStyle = COL.exitGlow;
    this.ctx.lineWidth = 2;
    this.diamondPath(top.x, top.y, halfW * 0.62, halfH * 0.62);
    this.ctx.stroke();
    this.ctx.fillStyle = COL.exitGlow;
    this.ctx.font = `bold ${Math.round(halfH * 0.95)}px ui-monospace, monospace`;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText('▲', top.x, top.y - halfH * 0.1);
    this.ctx.shadowBlur = 0;
  }

  private drawOpenGate(top: Vec2, halfW: number, halfH: number): void {
    this.ctx.strokeStyle = COL.gateFrame;
    this.ctx.lineWidth = 2;
    this.diamondPath(top.x, top.y, halfW * 0.8, halfH * 0.8);
    this.ctx.stroke();
  }

  private drawGateBars(top: Vec2, halfW: number, halfH: number): void {
    this.ctx.strokeStyle = '#e2b26a';
    this.ctx.lineWidth = 2 * this.cam.zoom;
    for (let i = -1; i <= 1; i++) {
      this.ctx.beginPath();
      this.ctx.moveTo(top.x + i * halfW * 0.35, top.y - halfH * 0.7);
      this.ctx.lineTo(top.x + i * halfW * 0.35, top.y - halfH * 2.0);
      this.ctx.stroke();
    }
  }

  private drawTerminal(top: Vec2, halfW: number, halfH: number): void {
    const w = halfW * 0.5;
    const h = halfH * 1.1;
    this.ctx.fillStyle = '#0e2430';
    this.ctx.fillRect(top.x - w / 2, top.y - h, w, h);
    this.ctx.fillStyle = COL.terminal;
    this.ctx.fillRect(top.x - w / 2 + 2, top.y - h + 2, w - 4, h * 0.4);
    // Blinking cursor glow
    this.ctx.shadowColor = COL.terminal;
    this.ctx.shadowBlur = 6;
    this.ctx.fillStyle = COL.terminal;
    this.ctx.fillRect(top.x - w / 4, top.y - h * 0.45, w * 0.15, h * 0.12);
    this.ctx.shadowBlur = 0;
  }

  // ---------------------------------------------------------------------------
  // Character figures
  // ---------------------------------------------------------------------------

  private drawFigure(
    x: number,
    y: number,
    facing: Dir,
    color: string,
    dark: string,
    alpha: number,
    isGuard = false,
    variant?: GuardVariant,
    highlighted = false,
  ): void {
    const zoom = this.cam.zoom;
    const tileW = this.iso.tileWidth * zoom;
    const halfH = (this.iso.tileHeight / 2) * zoom;
    const base = this.center(x, y, 0);
    const s = this.center(x, y, 0.35);
    const r = 8 * zoom;
    const prev = this.ctx.globalAlpha;
    this.ctx.globalAlpha = alpha;

    if (highlighted) {
      this.ctx.save();
      this.ctx.shadowColor = '#e7cf91';
      this.ctx.shadowBlur = 16 * zoom;
      this.ctx.strokeStyle = '#f4dfaa';
      this.ctx.lineWidth = Math.max(2, 2.5 * zoom);
      this.diamondPath(base.x, base.y + halfH * 0.18, tileW * 0.54, halfH * 0.34);
      this.ctx.stroke();
      this.ctx.restore();
    }

    // Ground shadow.
    this.ctx.fillStyle = 'rgba(0,0,0,0.45)';
    this.blob(base.x, base.y + halfH * 0.5, tileW * 0.48, tileW * 0.18);

    // Facing direction arrow — always drawn under the portrait for legibility.
    const v = DIR_VEC[facing];
    const fx = (v.dx - v.dy) * 0.5;
    const fy = (v.dx + v.dy) * 0.5;
    this.ctx.fillStyle = isGuard ? COL.guard : COL.legal;
    this.poly([
      { x: s.x + fx * r * 2.4, y: s.y + fy * r * 1.2 },
      { x: s.x + (fx * 1.0 - fy * 0.7) * r, y: s.y + (fy * 1.0 + fx * 0.7) * r * 0.5 },
      { x: s.x + (fx * 1.0 + fy * 0.7) * r, y: s.y + (fy * 1.0 - fx * 0.7) * r * 0.5 },
    ]);

    // --- Portrait sprite (photorealistic) ---------------------------------
    const spriteSrc = isGuard
      ? variant === 'sniper'
        ? SPRITE.guardSniper
        : variant === 'machinegunner'
          ? SPRITE.guardMachinegunner
          : SPRITE.guardOfficer
      : SPRITE.playerChar;
    const img = this.sprites.get(spriteSrc);

    if (img) {
      // Keep the actor inside one logical cell rather than treating a portrait
      // as a multi-cell prop.
      // The sprite image includes the stone base; anchor its bottom-centre to
      // the bottom edge of the tile's floor diamond.
      const spriteW = tileW * 0.92;
      const spriteH = spriteW * (img.naturalHeight / img.naturalWidth);
      const drawX = base.x - spriteW / 2;
      const drawY = base.y + halfH - spriteH; // bottom of sprite == tile base bottom
      this.ctx.drawImage(img, drawX, drawY, spriteW, spriteH);
    } else {
      // --- Procedural fallback (used until sprites finish loading) ----------
      const h = 22 * zoom;
      this.ctx.fillStyle = dark;
      this.roundedColumn(s.x, s.y, r, h);
      this.ctx.fillStyle = color;
      this.roundedColumn(s.x - r * 0.15, s.y, r * 0.82, h);
      const headColor = isGuard ? '#c8a878' : '#d4aa80';
      this.ctx.fillStyle = headColor;
      this.blob(s.x, s.y - h, r * 0.62, r * 0.62);
      const hatColor = isGuard ? '#4a3020' : '#2a3a28';
      this.ctx.fillStyle = hatColor;
      this.ctx.fillRect(s.x - r * 0.58, s.y - h - r * 0.68, r * 1.16, r * 0.38);
      this.ctx.fillRect(s.x - r * 0.32, s.y - h - r * 1.1, r * 0.64, r * 0.45);
    }

    this.ctx.globalAlpha = prev;
  }

  // ---------------------------------------------------------------------------
  // Move markers
  // ---------------------------------------------------------------------------

  private drawLegalMarker(x: number, y: number): void {
    const zoom = this.cam.zoom;
    const halfW = (this.iso.tileWidth / 2) * zoom;
    const halfH = (this.iso.tileHeight / 2) * zoom;
    const c = this.center(x, y, 0.36);
    // Outer ring.
    this.ctx.strokeStyle = COL.legal;
    this.ctx.lineWidth = 1.5;
    this.diamondPath(c.x, c.y, halfW * 0.52, halfH * 0.52);
    this.ctx.stroke();
    // Subtle fill.
    this.ctx.fillStyle = 'rgba(144,216,255,0.08)';
    this.diamond(c.x, c.y, halfW * 0.52, halfH * 0.52);
  }

  private drawHover(x: number, y: number): void {
    const zoom = this.cam.zoom;
    const halfW = (this.iso.tileWidth / 2) * zoom;
    const halfH = (this.iso.tileHeight / 2) * zoom;
    const c = this.center(x, y, 0.36);
    this.ctx.strokeStyle = 'rgba(255,255,255,0.6)';
    this.ctx.lineWidth = 1.5;
    this.diamondPath(c.x, c.y, halfW, halfH);
    this.ctx.stroke();
  }

  // ---------------------------------------------------------------------------
  // Primitives
  // ---------------------------------------------------------------------------

  private center(x: number, y: number, z: number): Vec2 {
    const w = gridToScreen(x, y, z, this.iso);
    return this.cam.worldToScreen(w.x, w.y);
  }

  private poly(points: Vec2[]): void {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(points[0].x, points[0].y);
    for (let i = 1; i < points.length; i++) ctx.lineTo(points[i].x, points[i].y);
    ctx.closePath();
    ctx.fill();
  }

  private diamondPath(cx: number, cy: number, halfW: number, halfH: number): void {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(cx, cy - halfH);
    ctx.lineTo(cx + halfW, cy);
    ctx.lineTo(cx, cy + halfH);
    ctx.lineTo(cx - halfW, cy);
    ctx.closePath();
  }

  private diamond(cx: number, cy: number, halfW: number, halfH: number): void {
    this.diamondPath(cx, cy, halfW, halfH);
    this.ctx.fill();
  }

  private blob(cx: number, cy: number, rx: number, ry: number): void {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.ellipse(cx, cy, rx, ry, 0, 0, Math.PI * 2);
    ctx.fill();
  }

  private roundedColumn(cx: number, cy: number, r: number, h: number): void {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(cx - r, cy);
    ctx.lineTo(cx - r * 0.5, cy - h);
    ctx.lineTo(cx + r * 0.5, cy - h);
    ctx.lineTo(cx + r, cy);
    ctx.closePath();
    ctx.fill();
    this.blob(cx, cy, r, r * 0.5);
  }
}
