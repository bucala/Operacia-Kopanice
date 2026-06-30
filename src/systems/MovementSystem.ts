import { Actor } from '@/components/Actor';
import { AIComp } from '@/components/AIComp';
import { Faction } from '@/components/Faction';
import { Movement } from '@/components/Movement';
import { Position, facingFromDelta } from '@/components/Position';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import { Events, type SoundEvent } from '@/core/events';
import { Res } from '@/core/resources';
import type { TileMap } from '@/map/TileMap';

/**
 * Advances entities along the paths produced by A*. Movement speed scales
 * inversely with terrain cost (snow and mud slow agents down), facing is
 * derived from travel direction, and entering a tile emits a footstep sound
 * whose loudness depends on the terrain — which is what enemy AI listens for.
 */
export class MovementSystem implements System {
  readonly name = 'movement';

  update(world: World, ctx: FrameContext): void {
    const map = world.resource<TileMap>(Res.Map);

    for (const e of world.query(Movement, Position)) {
      const mv = world.must(e, Movement);
      const pos = world.must(e, Position);
      mv.noise = 0;

      // A dead actor stops where it fell; clear any remaining path.
      const actor = world.get(e, Actor);
      if (actor && !actor.alive) {
        mv.path = [];
        mv.moving = false;
        continue;
      }

      if (mv.path.length === 0) {
        mv.moving = false;
        pos.fx = pos.gx;
        pos.fy = pos.gy;
        continue;
      }
      mv.moving = true;

      const ai = world.get(e, AIComp);
      const speedScale = ai?.speedScale ?? 1;

      // A frame may cross several tiles when speed is high; loop with a budget.
      let budget = mv.baseSpeed * speedScale * ctx.dt;
      while (budget > 0 && mv.path.length > 0) {
        const next = mv.path[0];
        const enterCost = Math.max(0.1, map.moveCost(next.x, next.y));
        // Progress (in segment units) achievable with the remaining budget.
        const stepRate = 1 / enterCost; // tiles per "budget unit" on this terrain
        const remaining = 1 - mv.segmentT;
        const advance = budget * stepRate;

        if (advance < remaining) {
          mv.segmentT += advance;
          budget = 0;
        } else {
          // Arrive at `next`.
          budget -= remaining / stepRate;
          mv.segmentT = 0;
          pos.facing = facingFromDelta(next.x - pos.gx, next.y - pos.gy);
          pos.gx = next.x;
          pos.gy = next.y;
          mv.path.shift();
          this.onTileEntered(world, e, map, next.x, next.y, mv.noise);
          mv.noise = map.noiseAt(next.x, next.y);
        }
      }

      // Interpolate visual position between current tile and the next waypoint.
      if (mv.path.length > 0) {
        const next = mv.path[0];
        pos.fx = pos.gx + (next.x - pos.gx) * mv.segmentT;
        pos.fy = pos.gy + (next.y - pos.gy) * mv.segmentT;
      } else {
        pos.fx = pos.gx;
        pos.fy = pos.gy;
        mv.moving = false;
      }
    }
  }

  private onTileEntered(
    world: World,
    e: number,
    map: TileMap,
    gx: number,
    gy: number,
    _prevNoise: number,
  ): void {
    const tileNoise = map.noiseAt(gx, gy);
    if (tileNoise <= 0) return;
    const faction = world.get(e, Faction);
    const loudness = tileNoise * 3.2; // tiles audible
    world.events.emit<SoundEvent>(Events.Sound, {
      gx,
      gy,
      name: tileNoise > 1.2 ? 'footstep_snow' : 'footstep',
      loudness,
      source: e,
    });
    // Faction is read so future tuning can vary footstep loudness per team.
    void faction;
  }
}
