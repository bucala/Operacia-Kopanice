import { Render } from '@/components/Render';
import { Skills } from '@/components/Skills';
import type { Skill, SkillContext, SkillResult } from './types';

/** How long a disguise lasts, in seconds. */
const DISGUISE_DURATION = 12;

/**
 * disguiseAction — don an enemy uniform.
 *
 * While disguised, the AISystem treats the agent as far less suspicious: guards
 * gain suspicion much more slowly and tolerate the agent at medium range. The
 * disguise is dropped automatically when it times out or when the agent performs
 * an overtly hostile act (handled where those events are raised).
 */
export const disguiseAction: Skill = {
  id: 'disguise',
  name: 'Prestrojenie',
  glyph: '🎭',
  cooldown: 18,
  range: 0,
  targeting: 'self',
  description: 'Oblečieš si uniformu stráže — hliadky sú dočasne menej ostražité.',

  canUse(ctx: SkillContext): SkillResult {
    const skills = ctx.world.get(ctx.caster, Skills);
    if (!skills) return { ok: false, reason: 'Bez schopností' };
    if (skills.disguised) return { ok: false, reason: 'Už si prestrojený' };
    return { ok: true };
  },

  execute(ctx: SkillContext): void {
    const skills = ctx.world.must(ctx.caster, Skills);
    skills.disguised = true;
    skills.disguiseTimer = DISGUISE_DURATION;

    const render = ctx.world.get(ctx.caster, Render);
    if (render) render.sprite = 'agent_disguised';

    ctx.emitSound('disguise', ctx.target.gx, ctx.target.gy, 1.5);
    ctx.log('Prestrojený za stráž. Hliadky sú menej ostražité.', 'info');
  },
};
