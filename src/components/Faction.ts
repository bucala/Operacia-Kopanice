import { type Component, kind } from '@/core/ecs/Component';

export type Team = 'player' | 'enemy' | 'neutral';

/** Team membership, used to decide who is hostile to whom. */
export interface Faction extends Component {
  type: 'faction';
  team: Team;
}

export const Faction = kind<Faction>('faction');

export function makeFaction(team: Team): Faction {
  return { type: 'faction', team };
}

export function areHostile(a: Team, b: Team): boolean {
  if (a === 'neutral' || b === 'neutral') return false;
  return a !== b;
}
