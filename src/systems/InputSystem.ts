import { Actor } from '@/components/Actor';
import { Movement } from '@/components/Movement';
import { Position } from '@/components/Position';
import { Skills } from '@/components/Skills';
import type { Entity } from '@/core/ecs/Entity';
import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import type { Camera } from '@/core/Camera';
import type { Input } from '@/core/Input';
import { Events } from '@/core/events';
import { type IsoConfig, screenToGrid } from '@/core/math/iso';
import type { Vec2 } from '@/core/math/Vec2';
import { logTo, playerEntity, Res, type Settings } from '@/core/resources';
import { findPath } from '@/map/Pathfinding';
import type { TileMap } from '@/map/TileMap';
import type { RenderSystem } from './RenderSystem';
import type { SkillSystem } from './SkillSystem';

export interface InputHooks {
  skills: SkillSystem;
  render: RenderSystem;
  /** Resume the AudioContext on first gesture. */
  onGesture: () => void;
  requestReset: () => void;
}

/** Maps number keys to the skill they arm/cast. */
const SKILL_KEYS: Record<string, string> = {
  '1': 'knife',
  '2': 'disguise',
  '3': 'stone',
};

/**
 * Translates pointer and keyboard input into game actions: left-click to move
 * (A*), number keys to arm/cast skills, and toggles for the vision/grid/pause
 * overlays. Self-targeted skills fire immediately; tile/entity skills arm an aim
 * mode resolved by the next click.
 */
export class InputSystem implements System {
  readonly name = 'input';
  private armedSkill: string | null = null;

  constructor(private readonly hooks: InputHooks) {}

  update(world: World, _ctx: FrameContext): void {
    const input = world.resource<Input>(Res.Input);
    const cam = world.resource<Camera>(Res.Camera);
    const iso = world.resource<IsoConfig>(Res.Iso);
    const map = world.resource<TileMap>(Res.Map);
    const settings = world.resource<Settings>(Res.Settings);
    const player = playerEntity(world);

    const hover = this.pickTile(input, cam, iso);
    this.hooks.render.hoverTile = hover;

    this.handleKeys(world, input, settings, player);

    const clicks = input.takeClicks();
    if (clicks.length > 0) this.hooks.onGesture();
    for (const click of clicks) {
      const tile = this.pickTile2(click.x, click.y, cam, iso);
      if (click.button === 2) {
        this.armedSkill = null;
        this.commandMove(world, map, player, tile);
      } else if (this.armedSkill) {
        this.castSkill(world, player, tile);
      } else {
        this.commandMove(world, map, player, tile);
      }
    }

    this.updatePathPreview(world, map, player, hover, settings);
  }

  private handleKeys(
    world: World,
    input: Input,
    settings: Settings,
    player: Entity,
  ): void {
    for (const key of input.takeKeys()) {
      this.hooks.onGesture();
      if (key in SKILL_KEYS) {
        this.selectSkill(world, player, SKILL_KEYS[key]);
      } else if (key === 'f') {
        settings.showVision = !settings.showVision;
      } else if (key === 'g') {
        settings.showGrid = !settings.showGrid;
      } else if (key === 'p') {
        settings.paused = !settings.paused;
        logTo(world, settings.paused ? 'Pauza.' : 'Pokračujeme.', 'info');
      } else if (key === 'r') {
        this.hooks.requestReset();
      } else if (key === 'h') {
        world.events.emit(Events.AssistantRequest, {});
      } else if (key === 'escape') {
        this.armedSkill = null;
      }
    }
  }

  private selectSkill(world: World, player: Entity, skillId: string): void {
    const skills = world.get(player, Skills);
    if (!skills || !skills.equipped.includes(skillId)) {
      logTo(world, 'Schopnosť nie je vybavená.', 'warn');
      return;
    }
    skills.selected = skillId;
    const pos = world.must(player, Position);

    // Self-targeted skills fire at once; others enter aim mode.
    const target = { gx: pos.gx, gy: pos.gy, entity: null };
    const registry = world.resource(Res.Skills) as { get(id: string): { targeting: string } | undefined };
    const def = registry.get(skillId);
    if (def?.targeting === 'self') {
      const res = this.hooks.skills.trigger(world, player, skillId, target);
      if (!res.ok && res.reason) logTo(world, res.reason, 'warn');
      this.armedSkill = null;
    } else {
      this.armedSkill = skillId;
      logTo(world, `Zameriavam: ${skillId}. Klikni na cieľ.`, 'info');
    }
  }

  private castSkill(world: World, player: Entity, tile: Vec2): void {
    const skillId = this.armedSkill!;
    this.armedSkill = null;
    const target = {
      gx: tile.x,
      gy: tile.y,
      entity: this.entityAt(world, tile.x, tile.y, player),
    };
    const res = this.hooks.skills.trigger(world, player, skillId, target);
    if (!res.ok && res.reason) logTo(world, res.reason, 'warn');
  }

  private commandMove(world: World, map: TileMap, player: Entity, tile: Vec2): void {
    const actor = world.get(player, Actor);
    if (actor && !actor.alive) return;
    const pos = world.must(player, Position);
    const mv = world.must(player, Movement);
    if (!map.isWalkable(tile.x, tile.y)) {
      logTo(world, 'Tam sa nedá ísť.', 'warn');
      return;
    }
    const path = findPath(map, { x: pos.gx, y: pos.gy }, { x: tile.x, y: tile.y });
    if (path.length === 0) {
      logTo(world, 'Žiadna cesta k cieľu.', 'warn');
      return;
    }
    mv.path = path;
    mv.segmentT = 0;
  }

  private updatePathPreview(
    world: World,
    map: TileMap,
    player: Entity,
    hover: Vec2 | null,
    _settings: Settings,
  ): void {
    if (!hover || this.armedSkill || !map.isWalkable(hover.x, hover.y)) {
      this.hooks.render.pathPreview = [];
      return;
    }
    const pos = world.get(player, Position);
    if (!pos) return;
    this.hooks.render.pathPreview = findPath(
      map,
      { x: pos.gx, y: pos.gy },
      { x: hover.x, y: hover.y },
    );
  }

  private entityAt(world: World, gx: number, gy: number, exclude: Entity): Entity | null {
    for (const e of world.query(Position, Actor)) {
      if (e === exclude) continue;
      const p = world.must(e, Position);
      if (Math.round(p.gx) === gx && Math.round(p.gy) === gy) return e;
    }
    return null;
  }

  private pickTile(input: Input, cam: Camera, iso: IsoConfig): Vec2 | null {
    return this.pickTile2(input.mouseScreen.x, input.mouseScreen.y, cam, iso);
  }

  private pickTile2(sx: number, sy: number, cam: Camera, iso: IsoConfig): Vec2 {
    const world = cam.screenToWorld(sx, sy);
    const g = screenToGrid(world.x, world.y, iso);
    return { x: Math.round(g.x), y: Math.round(g.y) };
  }
}
