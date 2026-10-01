import { Actor } from '@/components/Actor';
import { AIComp } from '@/components/AIComp';
import { Faction, areHostile } from '@/components/Faction';
import { Movement } from '@/components/Movement';
import { Position, cellOf } from '@/components/Position';
import { Skills } from '@/components/Skills';
import { Vision } from '@/components/Vision';
import type { Entity } from '@/core/ecs/Entity';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import {
  Events,
  type ActorKilledEvent,
  type AIStateChangedEvent,
  type SoundEvent,
} from '@/core/events';
import { chebyshev, clamp } from '@/core/math/Vec2';
import { logTo, playerEntity, Res } from '@/core/resources';
import { type EnemyCtx, enemyFsm } from '@/ai/enemyFsm';
import type { TileMap } from '@/map/TileMap';

/** Damage dealt per second while a guard is adjacent to the player in ALERT. */
const ALERT_DPS = 22;

/**
 * Drives every enemy through the Patrol → Suspicious → Alert FSM. It computes
 * visual perception (modulated by the player's disguise), feeds heard sounds
 * into each agent's memory so they investigate distractions, and applies damage
 * when an alerted guard closes to melee range.
 */
export class AISystem implements System {
  readonly name = 'ai';
  private soundQueue: SoundEvent[] = [];

  init(world: World): void {
    world.events.on<SoundEvent>(Events.Sound, (s) => this.soundQueue.push(s));
  }

  update(world: World, ctx: FrameContext): void {
    const map = world.resource<TileMap>(Res.Map);
    const player = playerEntity(world);
    const playerPos = world.get(player, Position);
    const playerActor = world.get(player, Actor);
    if (!playerPos || !playerActor) return;

    const disguised = world.get(player, Skills)?.disguised ?? false;

    this.processSounds(world, player, disguised);

    for (const e of world.query(AIComp, Position, Vision, Movement, Faction, Actor)) {
      const actor = world.must(e, Actor);
      if (!actor.alive) continue;

      const ai = world.must(e, AIComp);
      const pos = world.must(e, Position);
      const vision = world.must(e, Vision);
      const movement = world.must(e, Movement);

      const perception = playerActor.alive
        ? this.perceive(vision, pos, playerPos, vision.range, disguised)
        : 0;

      const enemyCtx: EnemyCtx = {
        world,
        map,
        self: e,
        name: actor.name,
        ai,
        pos,
        vision,
        movement,
        player,
        playerPos,
        perception,
        frameMod: (n) => ctx.frame % n === 0,
        log: (m, level) => logTo(world, m, level),
        emitSound: (name, gx, gy, loudness) =>
          world.events.emit<SoundEvent>(Events.Sound, { name, gx, gy, loudness, source: e }),
      };

      const prev = ai.state;
      const nextState = enemyFsm.step(ai.state, enemyCtx, ctx.dt);
      if (nextState !== prev) {
        ai.state = nextState as typeof ai.state;
        world.events.emit<AIStateChangedEvent>(Events.AIStateChanged, {
          entity: e,
          from: prev,
          to: ai.state,
        });
      }

      if (ai.state === 'ALERT' && playerActor.alive) {
        this.tryAttack(world, e, pos, player, playerPos, playerActor, ctx.dt);
      }
    }
  }

  /** Visual perception strength in suspicion-per-second; 0 when unseen. */
  private perceive(
    vision: Vision,
    pos: Position,
    playerPos: Position,
    range: number,
    disguised: boolean,
  ): number {
    const key = `${Math.round(playerPos.gx)},${Math.round(playerPos.gy)}`;
    if (!vision.visible.has(key)) return 0;
    const dist = chebyshev(cellOf(pos), cellOf(playerPos));
    const closeness = clamp(1 - dist / range, 0, 1);
    if (disguised) {
      // A uniform fools guards unless they get right up close.
      return dist <= 2 ? 0.18 * (1 + closeness) : 0;
    }
    return 0.7 + 1.1 * closeness;
  }

  /** Feed buffered sounds into nearby hostile agents as investigation cues. */
  private processSounds(world: World, player: Entity, disguised: boolean): void {
    if (this.soundQueue.length === 0) return;
    const queue = this.soundQueue;
    this.soundQueue = [];

    for (const e of world.query(AIComp, Position, Faction, Actor)) {
      const actor = world.must(e, Actor);
      if (!actor.alive) continue;
      const faction = world.must(e, Faction);
      const pos = world.must(e, Position);
      const ai = world.must(e, AIComp);

      for (const snd of queue) {
        if (snd.source === e) continue;
        if (snd.loudness <= 0) continue; // silent cues (e.g. UI "blocked") never alert

        // Only sounds made by a hostile (or the player) draw attention.
        const srcFaction = snd.source !== null ? world.get(snd.source, Faction) : null;
        const fromHostile =
          snd.source === player || (srcFaction && areHostile(faction.team, srcFaction.team));
        if (!fromHostile) continue;

        const dist = Math.hypot(snd.gx - pos.gx, snd.gy - pos.gy);
        if (dist > snd.loudness) continue;

        let bump = 0.45 * (1 - dist / snd.loudness);
        if (disguised && snd.source === player) bump *= 0.4;
        ai.suspicion = Math.min(1.3, ai.suspicion + bump);
        ai.lastKnown = { x: snd.gx, y: snd.gy };
      }
    }
  }

  private tryAttack(
    world: World,
    attacker: Entity,
    pos: Position,
    player: Entity,
    playerPos: Position,
    playerActor: Actor,
    dt: number,
  ): void {
    if (chebyshev(cellOf(pos), cellOf(playerPos)) > 1) return;
    playerActor.hp -= ALERT_DPS * dt;
    if (playerActor.hp <= 0) {
      playerActor.hp = 0;
      playerActor.alive = false;
      logTo(world, 'Misia zlyhala — agent bol dolapený.', 'alert');
      world.events.emit<ActorKilledEvent>(Events.ActorKilled, {
        victim: player,
        killer: attacker,
      });
    }
  }
}
