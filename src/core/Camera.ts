import type { Vec2 } from './math/Vec2';
import { gridToScreen, type IsoConfig } from './math/iso';

/** Pans/zooms the isometric world and converts world↔screen coordinates. */
export class Camera {
  /** World-space pixel position kept centred on screen. */
  target: Vec2 = { x: 0, y: 0 };
  zoom = 1;

  constructor(
    public viewportWidth: number,
    public viewportHeight: number,
    private readonly iso: IsoConfig,
  ) {}

  /** Smoothly follow a grid position (call each frame). */
  followGrid(gx: number, gy: number, z = 0, lerp = 0.12): void {
    const p = gridToScreen(gx, gy, z, this.iso);
    this.target.x += (p.x - this.target.x) * lerp;
    this.target.y += (p.y - this.target.y) * lerp;
  }

  /** World pixel → screen pixel. */
  worldToScreen(wx: number, wy: number): Vec2 {
    return {
      x: (wx - this.target.x) * this.zoom + this.viewportWidth / 2,
      y: (wy - this.target.y) * this.zoom + this.viewportHeight / 2,
    };
  }

  /** Screen pixel → world pixel. */
  screenToWorld(sx: number, sy: number): Vec2 {
    return {
      x: (sx - this.viewportWidth / 2) / this.zoom + this.target.x,
      y: (sy - this.viewportHeight / 2) / this.zoom + this.target.y,
    };
  }
}
