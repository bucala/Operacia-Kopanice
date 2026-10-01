import { Render } from '@/components/Render';
import { Skills } from '@/components/Skills';
import type { Entity } from '@/core/ecs/Entity';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import { Events, type SkillUsedEvent, type SoundEvent } from '@/core/events';
import { logTo, Res } from '@/core/resources';
import type { SkillRegistry } from '@/skills/SkillRegistry';
import type { SkillContext, SkillResult, SkillTarget } from '@/skills/types';
import type { TileMap } from '@/map/TileMap';

/**
 * Owns skill cooldowns and the disguise timer, and exposes {@link trigger} —
 * the single entrypoint other systems (input, AI) use to fire a skill hook.
 */
export class SkillSystem implements System {
  readonly name = 'skill';

  update(world: World, ctx: FrameContext): void {
    for (const e of world.query(Skills)) {
      const skills = world.must(e, Skills);
      for (const id of Object.keys(skills.cooldowns)) {
        skills.cooldowns[id] = Math.max(0, skills.cooldowns[id] - ctx.dt);
      }
      if (skills.disguised) {
        skills.disguiseTimer -= ctx.dt;
        if (skills.disguiseTimer <= 0) this.dropDisguise(world, e, skills);
      }
    }
  }

  /** Attempt to use a skill. Returns whether it fired and, if not, why. */
  trigger(world: World, caster: Entity, skillId: string, target: SkillTarget): SkillResult {
    const registry = world.resource<SkillRegistry>(Res.Skills);
    const skill = registry.get(skillId);
    if (!skill) return { ok: false, reason: `Neznáma schopnosť '${skillId}'` };

    const skills = world.get(caster, Skills);
    if (skills && !skills.equipped.includes(skillId)) {
      return { ok: false, reason: 'Schopnosť nie je vybavená' };
    }
    if (skills && (skills.cooldowns[skillId] ?? 0) > 0) {
      return { ok: false, reason: 'Schopnosť sa nabíja' };
    }

    const map = world.resource<TileMap>(Res.Map);
    const skillCtx: SkillContext = {
      world,
      map,
      caster,
      target,
      log: (message, level) => logTo(world, message, level),
      emitSound: (name, gx, gy, loudness) =>
        world.events.emit<SoundEvent>(Events.Sound, { name, gx, gy, loudness, source: caster }),
    };

    const check = skill.canUse(skillCtx);
    if (!check.ok) {
      world.events.emit<SoundEvent>(Events.Sound, {
        name: 'blocked',
        gx: target.gx,
        gy: target.gy,
        loudness: 0,
        source: caster,
      });
      return check;
    }

    skill.execute(skillCtx);
    if (skills) {
      skills.cooldowns[skillId] = skill.cooldown;
      // Overtly hostile acts blow the agent's cover.
      if (skillId === 'knife' && skills.disguised) this.dropDisguise(world, caster, skills);
    }
    world.events.emit<SkillUsedEvent>(Events.SkillUsed, {
      caster,
      skillId,
      gx: target.gx,
      gy: target.gy,
    });
    return { ok: true };
  }

  private dropDisguise(world: World, e: Entity, skills: Skills): void {
    skills.disguised = false;
    skills.disguiseTimer = 0;
    const render = world.get(e, Render);
    if (render) render.sprite = 'agent';
    logTo(world, 'Prestrojenie sa minulo.', 'info');
  }
}
