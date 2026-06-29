import { makeActor } from '@/components/Actor';
import { makeAI } from '@/components/AIComp';
import { makeFaction } from '@/components/Faction';
import { makeInventory } from '@/components/Inventory';
import { makeMovement } from '@/components/Movement';
import { type Facing, makePosition } from '@/components/Position';
import { makeRender } from '@/components/Render';
import { makeSkills } from '@/components/Skills';
import { makeVision } from '@/components/Vision';
import type { Entity } from '@/core/ecs/Entity';
import type { World } from '@/core/ecs/World';
import type { Vec2 } from '@/core/math/Vec2';
import type { MapEntity } from '@/map/TileMap';

/** Creates the player-controlled agent and returns its entity id. */
export function spawnPlayer(world: World, x: number, y: number, facing: Facing): Entity {
  const e = world.createEntity();
  world.add(e, makePosition(x, y, facing));
  world.add(e, makeMovement(3.2));
  world.add(e, makeRender('agent', 'actor'));
  // Player sees in all directions (situational awareness drives fog-of-war).
  world.add(e, makeVision(7, Math.PI));
  world.add(e, makeFaction('player'));
  world.add(e, makeActor('Agent', 100, true));
  world.add(e, makeSkills(['knife', 'disguise', 'stone']));
  world.add(
    e,
    makeInventory(30, [
      { id: 'stone', name: 'Kameň', qty: 4, weight: 0.4 },
      { id: 'knife', name: 'Nôž', qty: 1, weight: 0.6 },
    ]),
  );
  return e;
}

/** Creates a patrolling guard from a map entity definition. */
export function spawnGuard(world: World, def: MapEntity): Entity {
  const e = world.createEntity();
  const facing = (def.facing as Facing) ?? 'S';
  world.add(e, makePosition(def.x, def.y, facing));
  world.add(e, makeMovement(2.6));
  world.add(e, makeRender('guard', 'actor'));
  // Guards see in a forward cone (45° half-angle = 90° field of view).
  world.add(e, makeVision(8, Math.PI / 4));
  world.add(e, makeFaction('enemy'));
  world.add(e, makeActor(def.name ?? 'Stráž', 60, false));
  const patrol: Vec2[] = (def.patrol ?? [[def.x, def.y]]).map(([x, y]) => ({ x, y }));
  world.add(e, makeAI(patrol));
  return e;
}
