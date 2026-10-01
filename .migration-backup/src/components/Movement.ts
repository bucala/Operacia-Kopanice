import { type Component, kind } from '@/core/ecs/Component';
import type { Vec2 } from '@/core/math/Vec2';

/**
 * Holds an active path (produced by A*) and the agent's progress along it.
 * The MovementSystem advances `segmentT` from 0→1 between consecutive nodes,
 * updating Position.fx/fy for smooth motion and snapping gx/gy on arrival.
 */
export interface Movement extends Component {
  type: 'movement';
  /** Remaining waypoints (grid tiles), excluding the tile currently occupied. */
  path: Vec2[];
  /** Interpolation [0,1) along the segment toward path[0]. */
  segmentT: number;
  /** Movement speed in tiles per second on flat road (cost 1). */
  baseSpeed: number;
  /** Set true while a path is being followed. */
  moving: boolean;
  /** Loudness of footsteps this tick (set by the system, read by audio/AI). */
  noise: number;
}

export const Movement = kind<Movement>('movement');

export function makeMovement(baseSpeed = 3): Movement {
  return { type: 'movement', path: [], segmentT: 0, baseSpeed, moving: false, noise: 0 };
}
