import { Inventory, removeItem } from '@/components/Inventory';
import type { Skill, SkillContext, SkillResult } from './types';

/**
 * stoneAction — throw a stone to create a distraction.
 *
 * Lands a loud noise on the target tile, luring nearby guards into the
 * Suspicious state so they investigate the wrong place. Demonstrates how a skill
 * can consume an inventory item and exploit the AI's sound-investigation logic.
 */
export const stoneAction: Skill = {
  id: 'stone',
  name: 'Kameň',
  glyph: '🪨',
  cooldown: 3,
  range: 9,
  targeting: 'tile',
  description: 'Hodíš kameň na zvolené miesto a odlákaš hliadky za hlukom.',

  canUse(ctx: SkillContext): SkillResult {
    const inv = ctx.world.get(ctx.caster, Inventory);
    if (!inv) return { ok: false, reason: 'Bez inventára' };
    const stones = inv.items.find((s) => s.id === 'stone');
    if (!stones || stones.qty <= 0) return { ok: false, reason: 'Žiadne kamene' };
    if (!ctx.map.inBounds(ctx.target.gx, ctx.target.gy)) {
      return { ok: false, reason: 'Mimo mapy' };
    }
    return { ok: true };
  },

  execute(ctx: SkillContext): void {
    const inv = ctx.world.must(ctx.caster, Inventory);
    removeItem(inv, 'stone', 1);
    ctx.emitSound('footstep', ctx.target.gx, ctx.target.gy, 6);
    ctx.log('Hodený kameň — odláka pozornosť hliadok.', 'info');
  },
};
