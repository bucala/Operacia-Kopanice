import { type Component, kind } from '@/core/ecs/Component';
import type { Vec2 } from '@/core/math/Vec2';

/** One of 8 facing directions, used for vision cones and sprite orientation. */
export type Facing = 'N' | 'NE' | 'E' | 'SE' | 'S' | 'SW' | 'W' | 'NW';

export const FACING_VECTORS: Record<Facing, { dx: number; dy: number }> = {
  N: { dx: 0, dy: -1 },
  NE: { dx: 1, dy: -1 },
  E: { dx: 1, dy: 0 },
  SE: { dx: 1, dy: 1 },
  S: { dx: 0, dy: 1 },
  SW: { dx: -1, dy: 1 },
  W: { dx: -1, dy: 0 },
  NW: { dx: -1, dy: -1 },
};

/**
 * Logical grid position plus a separate floating-point visual position. The
 * logical tile (gx, gy) is what gameplay (pathfinding, vision, collisions) uses;
 * (fx, fy) is interpolated by the MovementSystem for smooth on-screen motion.
 */
export interface Position extends Component {
  type: 'position';
  gx: number;
  gy: number;
  /** Interpolated visual coordinates (default to gx, gy). */
  fx: number;
  fy: number;
  facing: Facing;
}

export const Position = kind<Position>('position');

export function makePosition(gx: number, gy: number, facing: Facing = 'S'): Position {
  return { type: 'position', gx, gy, fx: gx, fy: gy, facing };
}

/** The integer grid cell of a position as a plain {x, y} vector. */
export function cellOf(p: { gx: number; gy: number }): Vec2 {
  return { x: Math.round(p.gx), y: Math.round(p.gy) };
}

export function facingFromDelta(dx: number, dy: number): Facing {
  const sx = Math.sign(dx);
  const sy = Math.sign(dy);
  const table: Record<string, Facing> = {
    '0,-1': 'N',
    '1,-1': 'NE',
    '1,0': 'E',
    '1,1': 'SE',
    '0,1': 'S',
    '-1,1': 'SW',
    '-1,0': 'W',
    '-1,-1': 'NW',
  };
  return table[`${sx},${sy}`] ?? 'S';
}
