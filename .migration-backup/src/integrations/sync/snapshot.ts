import { Actor } from '@/components/Actor';
import { AIComp } from '@/components/AIComp';
import { Faction } from '@/components/Faction';
import { Inventory } from '@/components/Inventory';
import { Position } from '@/components/Position';
import { Render } from '@/components/Render';
import { Skills } from '@/components/Skills';
import type { World } from '@/core/ecs/World';
import { playerEntity, Res } from '@/core/resources';
import type { TileMap } from '@/map/TileMap';
import type { GameSnapshot } from './types';

const SNAPSHOT_VERSION = 1;

/** Reads the live World into a serialisable snapshot. */
export function captureSnapshot(world: World): GameSnapshot {
  const map = world.resource<TileMap>(Res.Map);
  const player = playerEntity(world);
  const pos = world.must(player, Position);
  const actor = world.must(player, Actor);
  const skills = world.get(player, Skills);
  const inv = world.get(player, Inventory);

  const enemies: GameSnapshot['enemies'] = [];
  for (const e of world.query(AIComp, Position, Actor, Faction)) {
    const ai = world.must(e, AIComp);
    const ep = world.must(e, Position);
    const ea = world.must(e, Actor);
    enemies.push({
      name: ea.name,
      gx: Math.round(ep.gx),
      gy: Math.round(ep.gy),
      state: ai.state,
      alive: ea.alive,
    });
  }

  return {
    version: SNAPSHOT_VERSION,
    savedAt: Date.now(),
    map: map.name,
    player: {
      gx: Math.round(pos.gx),
      gy: Math.round(pos.gy),
      facing: pos.facing,
      hp: Math.round(actor.hp),
      alive: actor.alive,
      disguised: skills?.disguised ?? false,
      inventory: inv ? inv.items.map((s) => ({ ...s })) : [],
      skills: skills?.equipped ?? [],
    },
    enemies,
  };
}

/**
 * Applies a snapshot's *player* state (position, health, inventory, disguise)
 * back onto the live World. Enemy AI is intentionally left to re-derive itself
 * from the simulation, so loading only restores the player's persistent data.
 */
export function applySnapshot(world: World, snap: GameSnapshot): void {
  if (snap.version !== SNAPSHOT_VERSION) return;
  const player = playerEntity(world);
  const pos = world.get(player, Position);
  const actor = world.get(player, Actor);
  const skills = world.get(player, Skills);
  const inv = world.get(player, Inventory);

  if (pos) {
    pos.gx = snap.player.gx;
    pos.gy = snap.player.gy;
    pos.fx = snap.player.gx;
    pos.fy = snap.player.gy;
    pos.facing = snap.player.facing;
  }
  if (actor) {
    actor.hp = snap.player.hp;
    actor.alive = snap.player.alive;
  }
  if (inv) inv.items = snap.player.inventory.map((s) => ({ ...s }));
  if (skills) {
    skills.disguised = snap.player.disguised;
    const render = world.get(player, Render);
    if (render) render.sprite = snap.player.disguised ? 'agent_disguised' : 'agent';
  }
}
