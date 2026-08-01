import { Actor } from '@/components/Actor';
import { AIComp } from '@/components/AIComp';
import { Inventory } from '@/components/Inventory';
import { Position, cellOf } from '@/components/Position';
import { Skills } from '@/components/Skills';
import type { World } from '@/core/ecs/World';
import { chebyshev } from '@/core/math/Vec2';
import { playerEntity, Res } from '@/core/resources';
import type { TileMap } from '@/map/TileMap';

/** Structured read of the tactical situation, shared with the remote assistant. */
export interface TacticalContext {
  playerTile: { x: number; y: number };
  playerHp: number;
  disguised: boolean;
  terrainUnderfoot: string;
  terrainNoise: number;
  stones: number;
  nearestEnemy: { name: string; dist: number; state: string } | null;
  anyAlert: boolean;
  anySuspicious: boolean;
}

/** Builds a {@link TacticalContext} from the live World. */
export function readTacticalContext(world: World): TacticalContext {
  const map = world.resource<TileMap>(Res.Map);
  const player = playerEntity(world);
  const pos = world.must(player, Position);
  const actor = world.must(player, Actor);
  const skills = world.get(player, Skills);
  const inv = world.get(player, Inventory);
  const px = Math.round(pos.gx);
  const py = Math.round(pos.gy);
  const tile = map.tileAt(px, py);

  let nearest: TacticalContext['nearestEnemy'] = null;
  let anyAlert = false;
  let anySuspicious = false;
  for (const e of world.query(AIComp, Position, Actor)) {
    if (!world.must(e, Actor).alive) continue;
    const ai = world.must(e, AIComp);
    if (ai.state === 'ALERT') anyAlert = true;
    if (ai.state === 'SUSPICIOUS') anySuspicious = true;
    const ep = world.must(e, Position);
    const dist = chebyshev(cellOf(pos), cellOf(ep));
    if (!nearest || dist < nearest.dist) {
      nearest = { name: world.must(e, Actor).name, dist, state: ai.state };
    }
  }

  return {
    playerTile: { x: px, y: py },
    playerHp: Math.round(actor.hp),
    disguised: skills?.disguised ?? false,
    terrainUnderfoot: tile.name,
    terrainNoise: tile.noise,
    stones: inv?.items.find((s) => s.id === 'stone')?.qty ?? 0,
    nearestEnemy: nearest,
    anyAlert,
    anySuspicious,
  };
}

/**
 * A local, rule-based tactical advisor. It serves as the offline fallback for
 * the Claude assistant and as a fast first response, turning the tactical
 * context into a concrete stealth hint.
 */
export function localAdvice(ctx: TacticalContext): string {
  if (ctx.playerHp <= 30) {
    return 'Máš málo zdravia — okamžite preruš kontakt a skry sa za prekážku.';
  }
  if (ctx.anyAlert) {
    return ctx.disguised
      ? 'Si odhalený aj v prestrojení. Použi kameň (3) ako odlákanie a zmizni z dohľadu.'
      : 'Poplach! Zalom za roh, aby si prerušil priamu viditeľnosť, a zmení smer.';
  }
  if (ctx.nearestEnemy && ctx.nearestEnemy.dist <= 1 && !ctx.disguised) {
    return 'Stráž je hneď vedľa — tichá likvidácia nožom (1), kým ťa nezbadá.';
  }
  if (ctx.anySuspicious) {
    return 'Stráž niečo začula. Zostaň v tichu (vyhni sa snehu) a počkaj, kým sa upokojí.';
  }
  if (ctx.terrainNoise >= 1.3) {
    return `Stojíš na hlučnom povrchu (${ctx.terrainUnderfoot}). Drž sa cesty — pohyb je tichší a rýchlejší.`;
  }
  if (!ctx.disguised && ctx.nearestEnemy && ctx.nearestEnemy.dist <= 4) {
    return 'Stráž je blízko. Zváž prestrojenie (2), aby si prešiel popri hliadke.';
  }
  if (ctx.stones > 0) {
    return 'Kľud. Kameňom (3) môžeš odlákať hliadku z trasy a prejsť bezpečne.';
  }
  return 'Čisté. Postupuj po ceste a sleduj zorné polia hliadok (F).';
}
