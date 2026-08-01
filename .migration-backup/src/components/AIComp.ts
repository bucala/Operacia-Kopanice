import { type Component, kind } from '@/core/ecs/Component';
import type { Entity } from '@/core/ecs/Entity';
import type { Vec2 } from '@/core/math/Vec2';

/** The three high-level behaviour states of the enemy FSM. */
export type AIState = 'PATROL' | 'SUSPICIOUS' | 'ALERT';

/**
 * Per-enemy AI memory and configuration. The AISystem reads/writes this as the
 * blackboard for the finite-state machine (Patrol → Suspicious → Alert).
 */
export interface AIComp extends Component {
  type: 'ai';
  state: AIState;
  /** Looping patrol waypoints in grid space. */
  patrol: Vec2[];
  patrolIndex: number;
  /** Last position where the target (player) was seen or heard. */
  lastKnown: Vec2 | null;
  /** Currently engaged hostile entity, if any. */
  target: Entity | null;
  /** Accumulated suspicion 0..1; crossing thresholds drives state changes. */
  suspicion: number;
  /** Seconds remaining before the agent gives up investigating / de-alerts. */
  patience: number;
  /** Seconds the agent waits at each patrol waypoint. */
  waitTimer: number;
  /** Tiles-per-second multiplier applied to base speed in this state. */
  speedScale: number;
}

export const AIComp = kind<AIComp>('ai');

export function makeAI(patrol: Vec2[]): AIComp {
  return {
    type: 'ai',
    state: 'PATROL',
    patrol,
    patrolIndex: 0,
    lastKnown: null,
    target: null,
    suspicion: 0,
    patience: 0,
    waitTimer: 0,
    speedScale: 1,
  };
}
