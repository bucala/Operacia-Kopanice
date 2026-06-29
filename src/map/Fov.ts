import type { TileMap } from './TileMap';

/** Eye height added above the tile a viewer stands on. */
const EYE_HEIGHT = 1.6;
/** Tolerance so a viewer can see the top surface of an equal-height obstacle. */
const CLEARANCE = 0.5;

export interface FovParams {
  ox: number;
  oy: number;
  /** Facing unit vector; used only when coneHalfAngle < π. */
  fdx: number;
  fdy: number;
  range: number;
  coneHalfAngle: number;
}

/**
 * Height-aware line of sight between two tiles. The sight line is treated as a
 * 3D segment from the viewer's eye to the target's top surface; an intermediate
 * tile occludes only if its height rises above the interpolated line there. So
 * a wall hides what's behind it, but the player can still see over a low hay
 * bale into the distance — obstacles respect the grid's height map.
 */
export function lineOfSight(
  map: TileMap,
  ox: number,
  oy: number,
  tx: number,
  ty: number,
): boolean {
  if (ox === tx && oy === ty) return true;
  const eyeH = map.heightAt(ox, oy) + EYE_HEIGHT;
  const targetH = map.heightAt(tx, ty);

  const dx = tx - ox;
  const dy = ty - oy;
  const dist = Math.hypot(dx, dy);
  const samples = Math.ceil(dist / 0.1);

  let prevX = ox;
  let prevY = oy;
  for (let i = 1; i < samples; i++) {
    const t = i / samples;
    const cx = Math.round(ox + dx * t);
    const cy = Math.round(oy + dy * t);
    if (cx === prevX && cy === prevY) continue;
    if (cx === tx && cy === ty) break;
    prevX = cx;
    prevY = cy;

    const lineH = eyeH + (targetH - eyeH) * t;
    const obstacleH = map.heightAt(cx, cy);
    if (map.blocksVision(cx, cy)) {
      if (obstacleH > lineH - CLEARANCE) return false;
    } else if (obstacleH > lineH + CLEARANCE) {
      return false;
    }
  }
  return true;
}

/**
 * Computes the set of visible tiles for a viewer, honouring sight range, an
 * optional vision cone, and height-based occlusion. Returns packed "x,y" keys.
 */
export function computeFov(map: TileMap, p: FovParams): Set<string> {
  const visible = new Set<string>();
  visible.add(`${p.ox},${p.oy}`);

  const flen = Math.hypot(p.fdx, p.fdy) || 1;
  const nfx = p.fdx / flen;
  const nfy = p.fdy / flen;
  const cosLimit = Math.cos(p.coneHalfAngle);
  const fullCircle = p.coneHalfAngle >= Math.PI;
  const r = Math.ceil(p.range);

  for (let oy = p.oy - r; oy <= p.oy + r; oy++) {
    for (let ox = p.ox - r; ox <= p.ox + r; ox++) {
      if (!map.inBounds(ox, oy)) continue;
      const dx = ox - p.ox;
      const dy = oy - p.oy;
      const d = Math.hypot(dx, dy);
      if (d > p.range || d === 0) continue;

      if (!fullCircle) {
        const dot = (dx * nfx + dy * nfy) / d;
        if (dot < cosLimit) continue; // outside the cone
      }
      if (lineOfSight(map, p.ox, p.oy, ox, oy)) {
        visible.add(`${ox},${oy}`);
      }
    }
  }
  return visible;
}
