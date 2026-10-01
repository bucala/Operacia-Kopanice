import { Actor } from '@/components/Actor';
import { AIComp } from '@/components/AIComp';
import { Faction } from '@/components/Faction';
import { Movement } from '@/components/Movement';
import { Position } from '@/components/Position';
import { Render } from '@/components/Render';
import { Skills } from '@/components/Skills';
import { Vision } from '@/components/Vision';
import type { Entity } from '@/core/ecs/Entity';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import type { Camera } from '@/core/Camera';
import { depthKey, gridToScreen, type IsoConfig } from '@/core/math/iso';
import type { Vec2 } from '@/core/math/Vec2';
import { playerEntity, Res, type Settings } from '@/core/resources';
import type { AssetBundle, ImageRef, PropDef, SpriteDef } from '@/assets/types';
import type { ImageStore } from '@/assets/images';
import type { TileDef } from '@/map/tiles';
import type { TileMap } from '@/map/TileMap';

interface DrawItem {
  depth: number;
  draw: () => void;
}

const STATE_GLYPH: Record<string, string> = {
  PATROL: '',
  SUSPICIOUS: '?',
  ALERT: '!',
};
const STATE_COLOR: Record<string, string> = {
  PATROL: '#7bd88f',
  SUSPICIOUS: '#f2c14e',
  ALERT: '#f25c5c',
};

/**
 * Renders the world in isometric projection: a depth-sorted pass draws ground
 * diamonds, extruded tiles/props (for volume), and procedurally-shaded
 * characters, followed by overlay passes for fog-of-war, vision cones, the
 * planned path, and the hover cursor.
 */
export class RenderSystem implements System {
  readonly name = 'render';
  private readonly explored = new Set<string>();
  /** Image-only prop definitions for the current frame (from the sprite atlas). */
  private props: Record<string, PropDef> = {};
  /** Set by the InputSystem so the renderer can preview the planned path. */
  hoverTile: Vec2 | null = null;
  pathPreview: Vec2[] = [];

  constructor(
    private readonly ctx: CanvasRenderingContext2D,
    private readonly canvas: HTMLCanvasElement,
    private readonly images: ImageStore,
  ) {}

  update(world: World, frame: FrameContext): void {
    const map = world.resource<TileMap>(Res.Map);
    const iso = world.resource<IsoConfig>(Res.Iso);
    const cam = world.resource<Camera>(Res.Camera);
    const settings = world.resource<Settings>(Res.Settings);
    const assets = world.resource<AssetBundle>(Res.Assets);
    this.props = assets.sprites.props ?? {};
    const player = playerEntity(world);
    const playerVision = world.get(player, Vision);
    const visible = playerVision?.visible ?? new Set<string>();

    this.clear();
    const items: DrawItem[] = [];

    // --- Tiles -------------------------------------------------------------
    map.forEach((x, y, tile, elevation) => {
      const seen = visible.has(`${x},${y}`);
      if (seen) this.explored.add(`${x},${y}`);
      const known = seen || this.explored.has(`${x},${y}`);
      items.push({
        depth: depthKey(x, y, 0),
        draw: () => this.drawTile(cam, iso, x, y, tile, elevation, seen, known),
      });
    });

    // --- Entities ----------------------------------------------------------
    for (const e of world.query(Position, Render)) {
      const pos = world.must(e, Position);
      const render = world.must(e, Render);
      const gxr = Math.round(pos.fx);
      const gyr = Math.round(pos.fy);
      const isPlayer = e === player;
      const seen = isPlayer || visible.has(`${gxr},${gyr}`);
      if (!seen && !this.explored.has(`${gxr},${gyr}`)) continue;
      const actor = world.get(e, Actor);
      // Hide live enemies that aren't currently visible; show corpses if explored.
      if (!isPlayer && !seen && actor?.alive) continue;

      const elevation = map.heightAt(gxr, gyr);
      items.push({
        depth: depthKey(pos.fx, pos.fy, elevation + 0.5),
        draw: () => this.drawEntity(world, cam, iso, assets, e, pos, render, frame, seen),
      });
    }

    items.sort((a, b) => a.depth - b.depth);
    for (const it of items) it.draw();

    // --- Overlays ----------------------------------------------------------
    if (settings.showVision) this.drawVisionOverlay(world, cam, iso, map);
    if (settings.showGrid) this.drawGrid(cam, iso, map);
    this.drawPath(cam, iso);
    this.drawHover(cam, iso, map);
  }

  private clear(): void {
    const { width, height } = this.canvas;
    const g = this.ctx.createLinearGradient(0, 0, 0, height);
    g.addColorStop(0, '#0a0d12');
    g.addColorStop(1, '#10161f');
    this.ctx.fillStyle = g;
    this.ctx.fillRect(0, 0, width, height);
  }

  private tileCenter(cam: Camera, iso: IsoConfig, x: number, y: number, z: number): Vec2 {
    const w = gridToScreen(x, y, z, iso);
    return cam.worldToScreen(w.x, w.y);
  }

  private drawTile(
    cam: Camera,
    iso: IsoConfig,
    x: number,
    y: number,
    tile: TileDef,
    elevation: number,
    seen: boolean,
    known: boolean,
  ): void {
    if (!known) return;
    const halfW = (iso.tileWidth / 2) * cam.zoom;
    const halfH = (iso.tileHeight / 2) * cam.zoom;
    const dim = seen ? 1 : 0.42;

    // Prism height: walls use full elevation; decor props get a short base and
    // contribute their volume via the decoration shape on top.
    const prismZ = tile.decor && tile.decor !== 'block' ? Math.min(1, elevation) : elevation;
    const top = this.tileCenter(cam, iso, x, y, prismZ);
    const base = this.tileCenter(cam, iso, x, y, 0);

    if (prismZ > 0) {
      // Left face.
      this.ctx.fillStyle = shade(tile.colors.left, dim);
      this.poly([
        { x: top.x - halfW, y: top.y },
        { x: top.x, y: top.y + halfH },
        { x: base.x, y: base.y + halfH },
        { x: base.x - halfW, y: base.y },
      ]);
      // Right face.
      this.ctx.fillStyle = shade(tile.colors.right, dim);
      this.poly([
        { x: top.x, y: top.y + halfH },
        { x: top.x + halfW, y: top.y },
        { x: base.x + halfW, y: base.y },
        { x: base.x, y: base.y + halfH },
      ]);
    }

    // Top diamond.
    this.ctx.fillStyle = shade(tile.colors.top, dim);
    this.diamond(top.x, top.y, halfW, halfH);
    // Subtle facet edge for readability.
    this.ctx.strokeStyle = `rgba(8,12,18,${seen ? 0.25 : 0.12})`;
    this.ctx.lineWidth = 1;
    this.ctx.stroke();

    if (tile.decor && tile.decor !== 'block') {
      // Prefer a high-fidelity decoration image; fall back to procedural shapes.
      const ref = this.props[`decor_${tile.decor}`];
      const drawn = ref ? this.drawImageAt(ref, top.x, top.y, cam.zoom, dim, iso) : null;
      if (drawn === null) this.drawDecor(tile.decor, top, halfW, halfH, dim);
    }
  }

  private drawDecor(
    decor: NonNullable<TileDef['decor']>,
    top: Vec2,
    halfW: number,
    halfH: number,
    dim: number,
  ): void {
    const ctx = this.ctx;
    if (decor === 'conifer') {
      ctx.fillStyle = shade('#5b3d24', dim);
      ctx.fillRect(top.x - halfW * 0.08, top.y - halfH * 0.4, halfW * 0.16, halfH * 1.4);
      for (let i = 0; i < 3; i++) {
        const ty = top.y - halfH * (0.2 + i * 1.0);
        const w = halfW * (1.0 - i * 0.22);
        ctx.fillStyle = shade(i === 2 ? '#3a7a44' : '#2f5d34', dim);
        this.poly([
          { x: top.x, y: ty - halfH * 1.5 },
          { x: top.x + w, y: ty },
          { x: top.x - w, y: ty },
        ]);
      }
    } else if (decor === 'boulder') {
      ctx.fillStyle = shade('#7f7d79', dim);
      this.blob(top.x, top.y - halfH * 0.5, halfW * 0.85, halfH * 1.1);
      ctx.fillStyle = shade('#9a9894', dim);
      this.blob(top.x - halfW * 0.2, top.y - halfH * 0.8, halfW * 0.45, halfH * 0.6);
    } else if (decor === 'haystack') {
      ctx.fillStyle = shade('#b7973e', dim);
      this.blob(top.x, top.y - halfH * 0.3, halfW * 0.95, halfH * 1.0);
      ctx.fillStyle = shade('#cdab4c', dim);
      this.blob(top.x, top.y - halfH * 0.6, halfW * 0.6, halfH * 0.6);
    }
  }

  private drawEntity(
    world: World,
    cam: Camera,
    iso: IsoConfig,
    assets: AssetBundle,
    e: Entity,
    pos: Position,
    render: Render,
    frame: FrameContext,
    seen: boolean,
  ): void {
    // Image-only props (buildings, crates) drawn purely from their picture.
    const propDef = assets.sprites.props?.[render.sprite];
    if (propDef) {
      const ps = this.tileCenter(cam, iso, pos.fx, pos.fy, 0);
      this.drawImageAt(propDef, ps.x, ps.y, cam.zoom, seen ? 1 : 0.55, iso);
      return;
    }

    const def = assets.sprites.sprites[render.sprite];
    if (!def) return;
    const elevation = world.resource<TileMap>(Res.Map).heightAt(
      Math.round(pos.fx),
      Math.round(pos.fy),
    );
    const screen = this.tileCenter(cam, iso, pos.fx, pos.fy, elevation);
    const actor = world.get(e, Actor);
    const alive = actor?.alive ?? true;
    const dim = seen ? 1 : 0.5;

    if (!alive) {
      this.drawCorpse(screen, cam.zoom, dim);
      return;
    }

    const mv = world.get(e, Movement);
    const bob = mv?.moving ? Math.sin(frame.elapsed * 10) * 1.5 * cam.zoom : 0;
    const cy = screen.y - bob;

    // Prefer the supplied character image; fall back to the procedural figure.
    let headTop: number;
    const imgH = def.image ? this.drawImageCharacter(def.image, screen.x, cy, cam.zoom, dim, iso) : null;
    if (imgH !== null && def.image) {
      headTop = cy - def.image.anchorY * imgH;
    } else {
      this.drawCharacter(screen, cam.zoom, def, pos, render, bob, dim);
      headTop = cy - (def.bodyHeight + def.radius * 1.4) * cam.zoom;
    }

    // Enemy state indicator + suspicion bar.
    const ai = world.get(e, AIComp);
    if (ai) this.drawAIIndicator(screen, cam.zoom, headTop, ai);

    // Player HP bar.
    if (e === playerEntity(world) && actor) {
      this.drawHealthBar(screen, cam.zoom, headTop, actor.hp / actor.maxHp);
      const skills = world.get(e, Skills);
      if (skills?.disguised) {
        this.ctx.fillStyle = '#d8c27a';
        this.ctx.font = `${10 * cam.zoom}px ui-monospace, monospace`;
        this.ctx.textAlign = 'center';
        this.ctx.fillText('🎭', screen.x, headTop - 12 * cam.zoom);
      }
    }
  }

  /**
   * Draws an image sprite anchored on the tile point (x, y). Returns the drawn
   * height in pixels, or null if the image has not loaded yet (caller falls
   * back). Honours optional spritesheet slicing via `ref.rect`.
   */
  private drawImageAt(
    ref: ImageRef,
    x: number,
    y: number,
    zoom: number,
    alpha: number,
    iso: IsoConfig,
  ): number | null {
    const img = this.images.ready(ref.src);
    if (!img) return null;
    const sw = ref.rect ? ref.rect[2] : img.naturalWidth;
    const sh = ref.rect ? ref.rect[3] : img.naturalHeight;
    if (!sw || !sh) return null;
    const w = ref.scale * iso.tileWidth * zoom;
    const h = w * (sh / sw);
    const dx = x - ref.anchorX * w;
    const dy = y - ref.anchorY * h;
    const prev = this.ctx.globalAlpha;
    this.ctx.globalAlpha = alpha;
    if (ref.rect) {
      this.ctx.drawImage(img, ref.rect[0], ref.rect[1], sw, sh, dx, dy, w, h);
    } else {
      this.ctx.drawImage(img, dx, dy, w, h);
    }
    this.ctx.globalAlpha = prev;
    return h;
  }

  /** Image-backed character: ground shadow + the figure. Null if not loaded. */
  private drawImageCharacter(
    ref: ImageRef,
    x: number,
    cy: number,
    zoom: number,
    dim: number,
    iso: IsoConfig,
  ): number | null {
    if (!this.images.ready(ref.src)) return null;
    this.ctx.fillStyle = 'rgba(0,0,0,0.32)';
    this.blob(x, cy + 2 * zoom, 11 * zoom, 5 * zoom);
    return this.drawImageAt(ref, x, cy, zoom, dim, iso);
  }

  private drawCharacter(
    screen: Vec2,
    zoom: number,
    def: SpriteDef,
    pos: Position,
    render: Render,
    bob: number,
    dim: number,
  ): void {
    const ctx = this.ctx;
    const r = def.radius * zoom * render.scale;
    const h = def.bodyHeight * zoom * render.scale;
    const cx = screen.x;
    const cy = screen.y - bob;
    const body = render.tint ?? def.palette.body;

    // Soft shadow on the ground.
    ctx.fillStyle = 'rgba(0,0,0,0.32)';
    this.blob(cx, screen.y + 2 * zoom, r * 1.1, r * 0.55);

    // Body — a rounded prism for a sense of volume.
    ctx.fillStyle = shade(def.palette.bodyDark, dim);
    this.roundedColumn(cx, cy, r, h, 0.5);
    ctx.fillStyle = shade(body, dim);
    this.roundedColumn(cx - r * 0.18, cy, r * 0.82, h, 0.5);

    // Head.
    const headY = cy - h;
    ctx.fillStyle = shade(def.palette.headDark, dim);
    this.blob(cx, headY, r * 0.62, r * 0.62);
    ctx.fillStyle = shade(def.palette.head, dim);
    this.blob(cx - r * 0.12, headY - r * 0.12, r * 0.45, r * 0.45);

    // Facing nub (a shoulder marker pointing where the agent looks).
    const f = facingScreenVec(pos.facing);
    ctx.fillStyle = shade(def.palette.accent, dim);
    this.blob(cx + f.x * r * 0.9, cy - h * 0.5 + f.y * r * 0.5, r * 0.28, r * 0.28);

    ctx.strokeStyle = `rgba(8,12,18,${0.6 * dim})`;
    ctx.lineWidth = 1;
  }

  private drawCorpse(screen: Vec2, zoom: number, dim: number): void {
    const ctx = this.ctx;
    ctx.fillStyle = shade('#5a3030', dim);
    this.blob(screen.x, screen.y, 11 * zoom, 6 * zoom);
    ctx.strokeStyle = shade('#2a1414', dim);
    ctx.lineWidth = 2 * zoom;
    ctx.beginPath();
    ctx.moveTo(screen.x - 6 * zoom, screen.y - 3 * zoom);
    ctx.lineTo(screen.x + 6 * zoom, screen.y + 3 * zoom);
    ctx.moveTo(screen.x + 6 * zoom, screen.y - 3 * zoom);
    ctx.lineTo(screen.x - 6 * zoom, screen.y + 3 * zoom);
    ctx.stroke();
  }

  /** `topY` is the y just above the sprite's head (works for image or procedural). */
  private drawAIIndicator(screen: Vec2, zoom: number, topY: number, ai: AIComp): void {
    const ctx = this.ctx;
    const barY = topY - 6 * zoom;
    const w = 22 * zoom;
    const frac = Math.min(1, ai.suspicion / 1.0);
    ctx.fillStyle = 'rgba(0,0,0,0.5)';
    ctx.fillRect(screen.x - w / 2, barY, w, 3 * zoom);
    ctx.fillStyle = STATE_COLOR[ai.state];
    ctx.fillRect(screen.x - w / 2, barY, w * frac, 3 * zoom);

    const glyph = STATE_GLYPH[ai.state];
    if (glyph) {
      ctx.fillStyle = STATE_COLOR[ai.state];
      ctx.font = `bold ${13 * zoom}px ui-monospace, monospace`;
      ctx.textAlign = 'center';
      ctx.fillText(glyph, screen.x, barY - 4 * zoom);
    }
  }

  private drawHealthBar(screen: Vec2, zoom: number, topY: number, frac: number): void {
    const ctx = this.ctx;
    const w = 26 * zoom;
    const barY = topY - 6 * zoom;
    ctx.fillStyle = 'rgba(0,0,0,0.5)';
    ctx.fillRect(screen.x - w / 2, barY, w, 3 * zoom);
    ctx.fillStyle = frac > 0.5 ? '#7bd88f' : frac > 0.25 ? '#f2c14e' : '#f25c5c';
    ctx.fillRect(screen.x - w / 2, barY, w * Math.max(0, frac), 3 * zoom);
  }

  private drawVisionOverlay(
    world: World,
    cam: Camera,
    iso: IsoConfig,
    map: TileMap,
  ): void {
    const halfW = (iso.tileWidth / 2) * cam.zoom;
    const halfH = (iso.tileHeight / 2) * cam.zoom;
    for (const e of world.query(Vision, Faction)) {
      const faction = world.must(e, Faction);
      if (faction.team !== 'enemy') continue;
      const ai = world.get(e, AIComp);
      const color = ai ? STATE_COLOR[ai.state] : '#f2c14e';
      const vision = world.must(e, Vision);
      this.ctx.globalAlpha = 0.12;
      this.ctx.fillStyle = color;
      for (const key of vision.visible) {
        const [x, y] = key.split(',').map(Number);
        const c = this.tileCenter(cam, iso, x, y, map.heightAt(x, y));
        this.diamond(c.x, c.y, halfW, halfH);
      }
      this.ctx.globalAlpha = 1;
    }
  }

  private drawGrid(cam: Camera, iso: IsoConfig, map: TileMap): void {
    const halfW = (iso.tileWidth / 2) * cam.zoom;
    const halfH = (iso.tileHeight / 2) * cam.zoom;
    this.ctx.strokeStyle = 'rgba(140,170,200,0.16)';
    this.ctx.lineWidth = 1;
    for (let y = 0; y < map.height; y++) {
      for (let x = 0; x < map.width; x++) {
        const c = this.tileCenter(cam, iso, x, y, 0);
        this.diamondPath(c.x, c.y, halfW, halfH);
        this.ctx.stroke();
      }
    }
  }

  private drawPath(cam: Camera, iso: IsoConfig): void {
    if (this.pathPreview.length === 0) return;
    this.ctx.fillStyle = 'rgba(143,208,255,0.5)';
    const halfW = (iso.tileWidth / 2) * cam.zoom * 0.35;
    const halfH = (iso.tileHeight / 2) * cam.zoom * 0.35;
    for (const p of this.pathPreview) {
      const c = this.tileCenter(cam, iso, p.x, p.y, 0);
      this.diamond(c.x, c.y, halfW, halfH);
    }
  }

  private drawHover(cam: Camera, iso: IsoConfig, map: TileMap): void {
    if (!this.hoverTile) return;
    const { x, y } = this.hoverTile;
    if (!map.inBounds(x, y)) return;
    const halfW = (iso.tileWidth / 2) * cam.zoom;
    const halfH = (iso.tileHeight / 2) * cam.zoom;
    const c = this.tileCenter(cam, iso, x, y, map.heightAt(x, y));
    this.ctx.strokeStyle = map.isWalkable(x, y) ? '#8fd0ff' : '#f25c5c';
    this.ctx.lineWidth = 2;
    this.diamondPath(c.x, c.y, halfW, halfH);
    this.ctx.stroke();
  }

  // --- Primitive shape helpers ---------------------------------------------

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

  private roundedColumn(cx: number, cy: number, r: number, h: number, taper: number): void {
    const ctx = this.ctx;
    ctx.beginPath();
    ctx.moveTo(cx - r, cy);
    ctx.lineTo(cx - r * taper, cy - h);
    ctx.lineTo(cx + r * taper, cy - h);
    ctx.lineTo(cx + r, cy);
    ctx.closePath();
    ctx.fill();
    this.blob(cx, cy, r, r * 0.5);
  }
}

/** Multiply a #rrggbb colour toward black for fog dimming. */
function shade(hex: string, factor: number): string {
  if (factor >= 1) return hex;
  const n = parseInt(hex.slice(1), 16);
  const r = Math.round(((n >> 16) & 0xff) * factor);
  const g = Math.round(((n >> 8) & 0xff) * factor);
  const b = Math.round((n & 0xff) * factor);
  return `rgb(${r},${g},${b})`;
}

/** Screen-space direction of a facing, used for the shoulder marker. */
function facingScreenVec(facing: Position['facing']): Vec2 {
  const map: Record<Position['facing'], Vec2> = {
    N: { x: 0, y: -1 },
    NE: { x: 1, y: -0.5 },
    E: { x: 1, y: 0 },
    SE: { x: 1, y: 0.5 },
    S: { x: 0, y: 1 },
    SW: { x: -1, y: 0.5 },
    W: { x: -1, y: 0 },
    NW: { x: -1, y: -0.5 },
  };
  return map[facing];
}
