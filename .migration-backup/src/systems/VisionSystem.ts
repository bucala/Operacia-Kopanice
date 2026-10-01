import { Actor } from '@/components/Actor';
import { Faction, areHostile } from '@/components/Faction';
import { FACING_VECTORS, Position } from '@/components/Position';
import { Vision } from '@/components/Vision';
import type { Entity } from '@/core/ecs/Entity';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import { Res } from '@/core/resources';
import { computeFov } from '@/map/Fov';
import type { TileMap } from '@/map/TileMap';

/**
 * Computes each viewer's set of visible tiles via height-aware raycasting and
 * flags whether any hostile actor is currently in view. To stay cheap, a
 * viewer's FoV is only recomputed when its tile or facing changes.
 */
export class VisionSystem implements System {
  readonly name = 'vision';
  private readonly cache = new Map<Entity, string>();

  update(world: World, _ctx: FrameContext): void {
    const map = world.resource<TileMap>(Res.Map);

    for (const e of world.query(Vision, Position)) {
      const vision = world.must(e, Vision);
      const pos = world.must(e, Position);
      const gx = Math.round(pos.gx);
      const gy = Math.round(pos.gy);
      const key = `${gx},${gy},${pos.facing},${vision.range}`;

      if (this.cache.get(e) !== key) {
        this.cache.set(e, key);
        const f = FACING_VECTORS[pos.facing];
        vision.visible = computeFov(map, {
          ox: gx,
          oy: gy,
          fdx: f.dx,
          fdy: f.dy,
          range: vision.range,
          coneHalfAngle: vision.coneHalfAngle,
        });
      }

      vision.seesTarget = this.seesAnyHostile(world, e, vision.visible);
    }
  }

  private seesAnyHostile(world: World, viewer: Entity, visible: Set<string>): boolean {
    const myFaction = world.get(viewer, Faction);
    if (!myFaction) return false;
    for (const other of world.query(Actor, Position, Faction)) {
      if (other === viewer) continue;
      const otherActor = world.must(other, Actor);
      if (!otherActor.alive) continue;
      const otherFaction = world.must(other, Faction);
      if (!areHostile(myFaction.team, otherFaction.team)) continue;
      const op = world.must(other, Position);
      if (visible.has(`${Math.round(op.gx)},${Math.round(op.gy)}`)) return true;
    }
    return false;
  }
}
