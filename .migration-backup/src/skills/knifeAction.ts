import { Actor } from '@/components/Actor';
import { Faction, areHostile } from '@/components/Faction';
import { Position, cellOf } from '@/components/Position';
import { chebyshev } from '@/core/math/Vec2';
import { Events, type ActorKilledEvent } from '@/core/events';
import type { Skill, SkillContext, SkillResult } from './types';

/**
 * knifeAction — a silent, adjacent takedown.
 *
 * Eliminates a hostile actor on or next to the target tile. It is near-silent
 * (a small noise so a very close guard might still notice), making it the
 * stealth tool for clearing patrols one by one.
 */
export const knifeAction: Skill = {
  id: 'knife',
  name: 'Nôž',
  glyph: '🗡',
  cooldown: 1.0,
  range: 1,
  targeting: 'entity',
  description: 'Tichá likvidácia priľahlého nepriateľa. Takmer nehlučná.',

  canUse(ctx: SkillContext): SkillResult {
    const casterPos = ctx.world.get(ctx.caster, Position);
    const casterFaction = ctx.world.get(ctx.caster, Faction);
    if (!casterPos || !casterFaction) return { ok: false, reason: 'Útočník chýba' };

    const victim = ctx.target.entity;
    if (victim === null || victim === ctx.caster) {
      return { ok: false, reason: 'Žiadny cieľ' };
    }
    const victimPos = ctx.world.get(victim, Position);
    const victimFaction = ctx.world.get(victim, Faction);
    const victimActor = ctx.world.get(victim, Actor);
    if (!victimPos || !victimFaction || !victimActor) {
      return { ok: false, reason: 'Neplatný cieľ' };
    }
    if (!victimActor.alive) return { ok: false, reason: 'Cieľ je už mŕtvy' };
    if (!areHostile(casterFaction.team, victimFaction.team)) {
      return { ok: false, reason: 'Cieľ nie je nepriateľ' };
    }
    if (chebyshev(cellOf(casterPos), cellOf(victimPos)) > this.range) {
      return { ok: false, reason: 'Príliš ďaleko — musíš byť vedľa cieľa' };
    }
    return { ok: true };
  },

  execute(ctx: SkillContext): void {
    const victim = ctx.target.entity!;
    const victimActor = ctx.world.must(victim, Actor);
    const victimPos = ctx.world.must(victim, Position);

    victimActor.hp = 0;
    victimActor.alive = false;

    ctx.emitSound('knife', victimPos.gx, victimPos.gy, 2.5);
    ctx.log(`Tichá likvidácia: ${victimActor.name}.`, 'warn');
    ctx.world.events.emit<ActorKilledEvent>(Events.ActorKilled, {
      victim,
      killer: ctx.caster,
    });
  },
};
