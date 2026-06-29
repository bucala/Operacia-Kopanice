import type { Vec2 } from './Vec2';

/**
 * Isometric projection math.
 *
 * The world is a 2D grid (gridX, gridY). We project it to screen space using a
 * 2:1 "diamond" isometric projection. Tile height (z) lifts a tile vertically on
 * screen, which is what gives the map its sense of depth/volume.
 */
export interface IsoConfig {
  /** Full width of a tile diamond in pixels. */
  tileWidth: number;
  /** Full height of a tile diamond in pixels (typically tileWidth / 2). */
  tileHeight: number;
  /** Vertical screen offset, in pixels, per unit of grid height (z). */
  heightStep: number;
}

export const defaultIso: IsoConfig = {
  tileWidth: 64,
  tileHeight: 32,
  heightStep: 16,
};

/** Grid coordinates (+ optional height z) → screen-space pixel position. */
export function gridToScreen(
  gridX: number,
  gridY: number,
  z = 0,
  cfg: IsoConfig = defaultIso,
): Vec2 {
  const halfW = cfg.tileWidth / 2;
  const halfH = cfg.tileHeight / 2;
  return {
    x: (gridX - gridY) * halfW,
    y: (gridX + gridY) * halfH - z * cfg.heightStep,
  };
}

/**
 * Screen-space pixel position → grid coordinates (flat, z=0).
 * Used for picking the tile under the mouse cursor.
 */
export function screenToGrid(
  screenX: number,
  screenY: number,
  cfg: IsoConfig = defaultIso,
): Vec2 {
  const halfW = cfg.tileWidth / 2;
  const halfH = cfg.tileHeight / 2;
  const gx = (screenX / halfW + screenY / halfH) / 2;
  const gy = (screenY / halfH - screenX / halfW) / 2;
  return { x: gx, y: gy };
}

/**
 * Painter's-algorithm depth key. Tiles/entities with a larger key are drawn
 * later (in front). Height contributes so that tall tiles still sort sanely.
 */
export function depthKey(gridX: number, gridY: number, z = 0): number {
  return (gridX + gridY) * 16 + z;
}
