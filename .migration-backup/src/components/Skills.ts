import { type Component, kind } from '@/core/ecs/Component';

/**
 * Skill loadout for an entity. The actual behaviour of each skill lives in the
 * SkillRegistry as a code hook (see src/skills); this component only tracks what
 * is equipped, what is selected, and per-skill cooldowns and transient state.
 */
export interface Skills extends Component {
  type: 'skills';
  equipped: string[];
  selected: string;
  /** Remaining cooldown seconds per skill id. */
  cooldowns: Record<string, number>;
  /** True while a disguise is active (set by disguiseAction). */
  disguised: boolean;
  /** Seconds of disguise remaining. */
  disguiseTimer: number;
}

export const Skills = kind<Skills>('skills');

export function makeSkills(equipped: string[]): Skills {
  return {
    type: 'skills',
    equipped: [...equipped],
    selected: equipped[0] ?? '',
    cooldowns: {},
    disguised: false,
    disguiseTimer: 0,
  };
}
