import type { World } from '@/core/ecs/World';
import type { Entity } from '@/core/ecs/Entity';
import type { TileMap } from '@/map/TileMap';

/** Where a skill is being aimed. */
export interface SkillTarget {
  gx: number;
  gy: number;
  /** The entity occupying the target tile, if any. */
  entity: Entity | null;
}

/** Everything a skill hook needs to read state and cause effects. */
export interface SkillContext {
  world: World;
  map: TileMap;
  caster: Entity;
  target: SkillTarget;
  /** Append a line to the on-screen action log. */
  log: (message: string, level?: 'info' | 'warn' | 'alert') => void;
  /** Emit a positional sound (also heard by enemy AI). */
  emitSound: (name: string, gx: number, gy: number, loudness: number) => void;
}

export interface SkillResult {
  ok: boolean;
  reason?: string;
}

/**
 * A Skill is a code-defined hook. Gameplay calls {@link canUse} to validate and
 * {@link execute} to apply effects. New abilities are added simply by
 * registering another object that satisfies this interface.
 */
export interface Skill {
  id: string;
  name: string;
  /** Short glyph shown in the HUD. */
  glyph: string;
  /** Cooldown in seconds after a successful use. */
  cooldown: number;
  /** Maximum targeting range in tiles (0 = self). */
  range: number;
  targeting: 'self' | 'tile' | 'entity';
  description: string;
  canUse(ctx: SkillContext): SkillResult;
  execute(ctx: SkillContext): void;
}
