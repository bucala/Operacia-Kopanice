import { Camera } from '@/core/Camera';
import { depthKey, gridToScreen, type IsoConfig } from '@/core/math/iso';
import type { Vec2 } from '@/core/math/Vec2';
import { GoGrid } from './model/grid';
import { key } from './model/logic';
import { DIR_VEC, type Dir, type GoState } from './model/types';

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
}

interface DrawItem {
  depth: number;
  draw: () => void;
}

const COL = {
  floorTop: '#39434f',
  floorLeft: '#232b34',
  floorRight: '#2c353f',
  edge: 'rgba(8,12,18,0.55)',
  exitTop: '#2f9c63',
  exitGlow: '#59e39b',
  wallTop: '#5c6675',
  wallLeft: '#333b46',
  wallRight: '#434d5a',
  gateBar: '#c58a3a',
  gateFrame: 'rgba(197,138,58,0.5)',
  terminal: '#39c6d6',
  player: '#4aa3ff',
  playerDark: '#2c6fb8',
  guard: '#f2554e',
  guardDark: '#a83732',
  danger: 'rgba(242,85,78,0.34)',
  legal: '#8fe0ff',
};

/**
 * Isometric renderer for the GO puzzle. Reuses the engine's iso projection and
 * camera, but paints a GO-flavoured board: raised platform nodes, red "lit"
 * danger tiles, guard facing arrows, a glowing exit, gates/terminals, and the
 * cyan legal-move markers the player clicks.
 */
export class GoRenderer {
  constructor(
    private readonly ctx: CanvasRenderingContext2D,
    private readonly canvas: HTMLCanvasElement,
    private readonly cam: Camera,
    private readonly iso: IsoConfig,
  ) {}

  /** Fit the whole board on screen with a margin (called per level / resize). */
  frameBoard(grid: GoGrid): void {
    let minX = Infinity;
    let minY = Infinity;
    let maxX = -Infinity;
    let maxY = -Infinity;
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
    const marginX = this.iso.tileWidth;
    const marginY = this.iso.tileHeight * 3; // headroom for raised walls/figures
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

    // Player.
    items.push({
      depth: depthKey(m.player.x, m.player.y, 0.6),
      draw: () => this.drawFigure(m.player.x, m.player.y, m.player.facing, COL.player, COL.playerDark, 1),
    });
    // Guards.
    for (const g of m.guards) {
      if (g.fade <= 0.01) continue;
      items.push({
        depth: depthKey(g.x, g.y, 0.6),
        draw: () => this.drawFigure(g.x, g.y, g.facing, COL.guard, COL.guardDark, g.fade, true),
      });
    }

    items.sort((a, b) => a.depth - b.depth);
    for (const it of items) it.draw();

    // Overlays on top of the board.
    for (const cell of m.legal) this.drawLegalMarker(cell.x, cell.y);
    if (m.hover) this.drawHover(m.hover.x, m.hover.y);
  }

  private clear(): void {
    const { width, height } = this.canvas;
    const g = this.ctx.createLinearGradient(0, 0, 0, height);
    g.addColorStop(0, '#0a0d12');
    g.addColorStop(1, '#12171f');
    this.ctx.fillStyle = g;
    this.ctx.fillRect(0, 0, width, height);
  }

  private center(x: number, y: number, z: number): Vec2 {
    const w = gridToScreen(x, y, z, this.iso);
    return this.cam.worldToScreen(w.x, w.y);
  }

  private drawCell(
    x: number,
    y: number,
    kind: string,
    gateOpen: boolean | undefined,
    isGate: boolean,
    isTerminal: boolean,
    lit: boolean,
  ): void {
    const halfW = (this.iso.tileWidth / 2) * this.cam.zoom;
    const halfH = (this.iso.tileHeight / 2) * this.cam.zoom;
    const closedGate = isGate && !gateOpen;
    const isWall = kind === 'wall' || closedGate;
    const z = isWall ? 1.4 : 0.35; // every node is a slightly raised platform

    const top = this.center(x, y, z);
    const base = this.center(x, y, 0);

    let topColor = COL.floorTop;
    let leftColor = COL.floorLeft;
    let rightColor = COL.floorRight;
    if (kind === 'exit') topColor = COL.exitTop;
    if (kind === 'wall') {
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

    // Danger tint sits on the walkable top surface.
    if (lit && !isWall) {
      this.ctx.fillStyle = COL.danger;
      this.diamond(top.x, top.y, halfW, halfH);
    }

    if (kind === 'exit') this.drawExitMark(top, halfW, halfH);
    if (isGate && gateOpen) this.drawOpenGate(top, halfW, halfH);
    if (closedGate) this.drawGateBars(top, halfW, halfH);
    if (isTerminal) this.drawTerminal(top, halfW, halfH);
  }

  private drawExitMark(top: Vec2, halfW: number, halfH: number): void {
    this.ctx.strokeStyle = COL.exitGlow;
    this.ctx.lineWidth = 2;
    this.diamondPath(top.x, top.y, halfW * 0.6, halfH * 0.6);
    this.ctx.stroke();
    this.ctx.fillStyle = COL.exitGlow;
    this.ctx.font = `bold ${Math.round(halfH * 0.9)}px ui-monospace, monospace`;
    this.ctx.textAlign = 'center';
    this.ctx.textBaseline = 'middle';
    this.ctx.fillText('▲', top.x, top.y - halfH * 0.1);
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
    this.ctx.fillStyle = '#12303a';
    this.ctx.fillRect(top.x - w / 2, top.y - h, w, h);
    this.ctx.fillStyle = COL.terminal;
    this.ctx.fillRect(top.x - w / 2 + 2, top.y - h + 2, w - 4, h * 0.4);
  }

  private drawFigure(
    x: number,
    y: number,
    facing: Dir,
    color: string,
    dark: string,
    alpha: number,
    isGuard = false,
  ): void {
    const zoom = this.cam.zoom;
    const s = this.center(x, y, 0.35);
    const r = 8 * zoom;
    const h = 20 * zoom;
    const prev = this.ctx.globalAlpha;
    this.ctx.globalAlpha = alpha;

    // Shadow.
    this.ctx.fillStyle = 'rgba(0,0,0,0.35)';
    this.blob(s.x, s.y + 2 * zoom, r * 1.1, r * 0.5);

    // Facing arrow on the ground (points where the figure looks).
    const v = DIR_VEC[facing];
    const fx = (v.dx - v.dy) * 0.5;
    const fy = (v.dx + v.dy) * 0.5;
    this.ctx.fillStyle = isGuard ? COL.guard : COL.legal;
    this.poly([
      { x: s.x + fx * r * 2.2, y: s.y + fy * r * 1.1 },
      { x: s.x + (fx * 1.0 - fy * 0.7) * r, y: s.y + (fy * 1.0 + fx * 0.7) * r * 0.5 },
      { x: s.x + (fx * 1.0 + fy * 0.7) * r, y: s.y + (fy * 1.0 - fx * 0.7) * r * 0.5 },
    ]);

    // Body column + head.
    this.ctx.fillStyle = dark;
    this.roundedColumn(s.x, s.y, r, h);
    this.ctx.fillStyle = color;
    this.roundedColumn(s.x - r * 0.15, s.y, r * 0.82, h);
    this.ctx.fillStyle = color;
    this.blob(s.x, s.y - h, r * 0.6, r * 0.6);

    this.ctx.globalAlpha = prev;
  }

  private drawLegalMarker(x: number, y: number): void {
    const halfW = (this.iso.tileWidth / 2) * this.cam.zoom;
    const halfH = (this.iso.tileHeight / 2) * this.cam.zoom;
    const c = this.center(x, y, 0.36);
    this.ctx.strokeStyle = COL.legal;
    this.ctx.lineWidth = 2;
    this.diamondPath(c.x, c.y, halfW * 0.5, halfH * 0.5);
    this.ctx.stroke();
  }

  private drawHover(x: number, y: number): void {
    const halfW = (this.iso.tileWidth / 2) * this.cam.zoom;
    const halfH = (this.iso.tileHeight / 2) * this.cam.zoom;
    const c = this.center(x, y, 0.36);
    this.ctx.strokeStyle = '#ffffff';
    this.ctx.lineWidth = 1.5;
    this.diamondPath(c.x, c.y, halfW, halfH);
    this.ctx.stroke();
  }

  // --- primitives ------------------------------------------------------------

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
