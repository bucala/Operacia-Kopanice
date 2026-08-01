import { type Component, kind } from '@/core/ecs/Component';

/** Living-being stats shared by the player and enemies. */
export interface Actor extends Component {
  type: 'actor';
  name: string;
  hp: number;
  maxHp: number;
  alive: boolean;
  /** True if the actor is the human-controlled protagonist. */
  controllable: boolean;
}

export const Actor = kind<Actor>('actor');

export function makeActor(name: string, maxHp = 100, controllable = false): Actor {
  return { type: 'actor', name, hp: maxHp, maxHp, alive: true, controllable };
}
