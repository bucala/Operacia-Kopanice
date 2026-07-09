import type { AIComp, AIState } from '@/components/AIComp';
import type { Movement } from '@/components/Movement';
import type { Position } from '@/components/Position';
import { cellOf, facingFromDelta } from '@/components/Position';
import type { Vision } from '@/components/Vision';
import type { Entity } from '@/core/ecs/Entity';
import type { World } from '@/core/ecs/World';
import { manhattan, type Vec2 } from '@/core/math/Vec2';
import { findPath } from '@/map/Pathfinding';
import type { TileMap } from '@/map/TileMap';
import { type FsmState, FiniteStateMachine } from './FSM';

/** Per-agent context assembled by the AISystem each tick. */
export interface EnemyCtx {
  world: World;
  map: TileMap;
  self: Entity;
  name: string;
  ai: AIComp;
  pos: Position;
  vision: Vision;
  movement: Movement;
  player: Entity;
  playerPos: Position;
  /** Strength of current visual perception of the player (0 = none). */
  perception: number;
  /** True every `n`-th frame; used to throttle expensive repaths. */
  frameMod(n: number): boolean;
  log: (message: string, level?: 'info' | 'warn' | 'alert') => void;
  emitSound: (name: string, gx: number, gy: number, loudness: number) => void;
}

// Tuning constants for the suspicion model.
const SUSPECT_THRESHOLD = 0.35;
const ALERT_THRESHOLD = 1.0;
const MAX_SUSPICION = 1.3;
const SUSPICION_DECAY = 0.25; // per second when not perceiving
const INVESTIGATE_PATIENCE = 6; // seconds
const ALERT_PATIENCE = 4; // seconds after losing sight

function repath(ctx: EnemyCtx, goal: Vec2): void {
  ctx.movement.path = findPath(ctx.map, { x: ctx.pos.gx, y: ctx.pos.gy }, goal);
  ctx.movement.segmentT = 0;
}

function faceToward(ctx: EnemyCtx, gx: number, gy: number): void {
  const dx = gx - ctx.pos.gx;
  const dy = gy - ctx.pos.gy;
  if (dx !== 0 || dy !== 0) ctx.pos.facing = facingFromDelta(dx, dy);
}

/** PATROL: walk a fixed loop; promote to SUSPICIOUS as suspicion builds. */
const patrol: FsmState<EnemyCtx> = {
  name: 'PATROL',
  onEnter(ctx) {
    ctx.ai.speedScale = 1;
    ctx.ai.target = null;
  },
  update(ctx, dt) {
    if (ctx.perception > 0) {
      ctx.ai.suspicion = Math.min(MAX_SUSPICION, ctx.ai.suspicion + ctx.perception * dt);
      ctx.ai.lastKnown = { x: ctx.playerPos.gx, y: ctx.playerPos.gy };
    } else {
      ctx.ai.suspicion = Math.max(0, ctx.ai.suspicion - SUSPICION_DECAY * dt);
    }
    if (ctx.ai.suspicion >= SUSPECT_THRESHOLD) return 'SUSPICIOUS';

    // Follow the looping patrol route.
    if (ctx.ai.patrol.length === 0) return;
    if (ctx.movement.moving || ctx.movement.path.length > 0) return;
    if (ctx.ai.waitTimer > 0) {
      ctx.ai.waitTimer -= dt;
      return;
    }
    const wp = ctx.ai.patrol[ctx.ai.patrolIndex];
    if (manhattan(cellOf(ctx.pos), wp) === 0) {
      ctx.ai.patrolIndex = (ctx.ai.patrolIndex + 1) % ctx.ai.patrol.length;
      ctx.ai.waitTimer = 1.2;
    } else {
      repath(ctx, wp);
    }
    return;
  },
};

/** SUSPICIOUS: investigate the last seen/heard spot; escalate or calm down. */
const suspicious: FsmState<EnemyCtx> = {
  name: 'SUSPICIOUS',
  onEnter(ctx) {
    ctx.ai.speedScale = 1.15;
    ctx.ai.patience = INVESTIGATE_PATIENCE;
    ctx.emitSound('suspicious', ctx.pos.gx, ctx.pos.gy, 4);
    ctx.log(`${ctx.name}: „Čo to bolo?“`, 'warn');
  },
  update(ctx, dt) {
    if (ctx.perception > 0) {
      ctx.ai.suspicion = Math.min(MAX_SUSPICION, ctx.ai.suspicion + ctx.perception * dt);
      ctx.ai.lastKnown = { x: ctx.playerPos.gx, y: ctx.playerPos.gy };
      ctx.ai.patience = INVESTIGATE_PATIENCE;
      faceToward(ctx, ctx.playerPos.gx, ctx.playerPos.gy);
      if (ctx.ai.suspicion >= ALERT_THRESHOLD) return 'ALERT';
    } else {
      ctx.ai.suspicion = Math.max(0, ctx.ai.suspicion - SUSPICION_DECAY * 0.5 * dt);
      ctx.ai.patience -= dt;
    }

    // Walk to the last known location and look around.
    if (ctx.ai.lastKnown && !ctx.movement.moving && ctx.movement.path.length === 0) {
      if (manhattan(cellOf(ctx.pos), ctx.ai.lastKnown) <= 1) {
        ctx.ai.lastKnown = null; // arrived; nothing here
      } else {
        repath(ctx, ctx.ai.lastKnown);
      }
    }

    if (ctx.ai.patience <= 0 && ctx.perception === 0) {
      ctx.ai.suspicion = 0;
      ctx.ai.lastKnown = null;
      return 'PATROL';
    }
    return;
  },
  onExit(ctx) {
    ctx.movement.path = [];
  },
};

/** ALERT: actively chase the player; fall back to searching when sight is lost. */
const alert: FsmState<EnemyCtx> = {
  name: 'ALERT',
  onEnter(ctx) {
    ctx.ai.speedScale = 1.7;
    ctx.ai.target = ctx.player;
    ctx.ai.patience = ALERT_PATIENCE;
    ctx.ai.suspicion = MAX_SUSPICION;
    ctx.emitSound('alert', ctx.pos.gx, ctx.pos.gy, 9);
    ctx.log(`${ctx.name}: „Votrelec! Poplach!“`, 'alert');
  },
  update(ctx, dt) {
    if (ctx.perception > 0) {
      ctx.ai.lastKnown = { x: ctx.playerPos.gx, y: ctx.playerPos.gy };
      ctx.ai.patience = ALERT_PATIENCE;
      faceToward(ctx, ctx.playerPos.gx, ctx.playerPos.gy);
    } else {
      ctx.ai.patience -= dt;
      if (ctx.ai.patience <= 0) {
        ctx.ai.suspicion = SUSPECT_THRESHOLD + 0.1;
        return 'SUSPICIOUS';
      }
    }

    // Pursue the last known player position; repath periodically as it moves.
    const goal = ctx.ai.lastKnown;
    if (goal) {
      const close = manhattan(cellOf(ctx.pos), goal) <= 1;
      if (!close && (ctx.movement.path.length === 0 || ctx.frameMod(8))) {
        repath(ctx, goal);
      }
    }
    return;
  },
  onExit(ctx) {
    ctx.ai.target = null;
  },
};

/** Singleton FSM shared by all enemies (state data lives on each AIComp). */
export const enemyFsm = new FiniteStateMachine<EnemyCtx>([patrol, suspicious, alert]);

export const ENEMY_STATES: readonly AIState[] = ['PATROL', 'SUSPICIOUS', 'ALERT'];
