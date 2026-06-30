import { type Component, kind } from '@/core/ecs/Component';

/**
 * Field-of-view definition for an entity. The VisionSystem casts rays across a
 * cone (or full circle) honouring the map's height field to compute which tiles
 * are visible. The result is cached in `visible` as packed "x,y" keys.
 */
export interface Vision extends Component {
  type: 'vision';
  /** Sight radius in tiles. */
  range: number;
  /** Half-angle of the vision cone in radians (Math.PI = full 360°). */
  coneHalfAngle: number;
  /** Tiles currently visible this frame ("gx,gy" keys). */
  visible: Set<string>;
  /** True if this entity can currently see a hostile of interest. */
  seesTarget: boolean;
}

export const Vision = kind<Vision>('vision');

export function makeVision(range = 8, coneHalfAngle = Math.PI / 4): Vision {
  return {
    type: 'vision',
    range,
    coneHalfAngle,
    visible: new Set<string>(),
    seesTarget: false,
  };
}
