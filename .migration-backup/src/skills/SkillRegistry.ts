import type { Skill } from './types';

/**
 * Central registry of skill hooks. The game registers built-in skills at
 * startup; additional skills can be added at runtime, which is how the Skill
 * System stays open for extension without touching the core loop.
 */
export class SkillRegistry {
  private readonly skills = new Map<string, Skill>();

  register(skill: Skill): this {
    if (this.skills.has(skill.id)) {
      throw new Error(`Skill '${skill.id}' is already registered`);
    }
    this.skills.set(skill.id, skill);
    return this;
  }

  get(id: string): Skill | undefined {
    return this.skills.get(id);
  }

  has(id: string): boolean {
    return this.skills.has(id);
  }

  all(): readonly Skill[] {
    return [...this.skills.values()];
  }
}
