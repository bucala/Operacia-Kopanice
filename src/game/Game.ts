import { Actor } from '@/components/Actor';
import { AIComp } from '@/components/AIComp';
import { Position } from '@/components/Position';
import { Skills } from '@/components/Skills';
import { Scheduler } from '@/core/ecs/System';
import { World } from '@/core/ecs/World';
import { Camera } from '@/core/Camera';
import { Input } from '@/core/Input';
import { gridToScreen, type IsoConfig } from '@/core/math/iso';
import { Rng } from '@/core/math/rng';
import { GameLog, logTo, Res, type Settings } from '@/core/resources';
import { SkillRegistry } from '@/skills/SkillRegistry';
import { knifeAction } from '@/skills/knifeAction';
import { disguiseAction } from '@/skills/disguiseAction';
import { stoneAction } from '@/skills/stoneAction';
import { TileMap } from '@/map/TileMap';
import { TilePalette } from '@/map/tiles';
import { loadAssets } from '@/assets/loader';
import type { AssetBundle } from '@/assets/types';
import { ImageStore } from '@/assets/images';
import { readConfig, type AppConfig } from '@/integrations/config';
import { ClaudeAssistant } from '@/integrations/assistant/ClaudeAssistant';
import { applySnapshot } from '@/integrations/sync/snapshot';
import { createSync } from '@/integrations/sync';
import type { SyncProvider } from '@/integrations/sync/types';
import { AISystem } from '@/systems/AISystem';
import { AssistantSystem } from '@/systems/AssistantSystem';
import { AudioSystem } from '@/systems/AudioSystem';
import { InputSystem } from '@/systems/InputSystem';
import { MovementSystem } from '@/systems/MovementSystem';
import { RenderSystem } from '@/systems/RenderSystem';
import { SkillSystem } from '@/systems/SkillSystem';
import { SyncSystem } from '@/systems/SyncSystem';
import { VisionSystem } from '@/systems/VisionSystem';
import { spawnGuard, spawnPlayer, spawnProp } from './spawn';

/** Snapshot of state the HUD needs each frame. */
export interface HudModel {
  mapName: string;
  hp: number;
  maxHp: number;
  alive: boolean;
  disguised: boolean;
  selected: string;
  cooldowns: { id: string; remaining: number }[];
  alertLevel: 'PATROL' | 'SUSPICIOUS' | 'ALERT';
  guardsAlive: number;
  paused: boolean;
  won: boolean;
  objective: string;
  syncName: string;
  assistant: 'claude' | 'local';
  log: { message: string; level: string }[];
}

/**
 * Top-level game controller: loads assets, builds the ECS world, wires the
 * system schedule, and runs the fixed-order update/render loop. Also owns the
 * win/lose checks, camera follow, and reset.
 */
export class Game {
  private readonly ctx2d: CanvasRenderingContext2D;
  private readonly input = new Input();
  private readonly images = new ImageStore();

  private world!: World;
  private scheduler!: Scheduler;

  private assets!: AssetBundle;
  private config!: AppConfig;
  private sync!: SyncProvider;
  private assistant!: ClaudeAssistant;

  private elapsed = 0;
  private frame = 0;
  private last = 0;
  private running = false;
  private won = false;
  private pendingReset = false;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly onHud?: (model: HudModel) => void,
  ) {
    const ctx = canvas.getContext('2d');
    if (!ctx) throw new Error('2D canvas context unavailable');
    this.ctx2d = ctx;
  }

  async start(): Promise<void> {
    this.config = readConfig();
    this.assets = await loadAssets();
    this.preloadImages();
    this.sync = createSync(this.config);
    this.assistant = new ClaudeAssistant(this.config.assistantEndpoint);
    this.input.attach(this.canvas);

    this.buildWorld();
    await this.loadSave();

    this.running = true;
    this.last = performance.now();
    requestAnimationFrame(this.loop);
  }

  resize(width: number, height: number): void {
    this.canvas.width = width;
    this.canvas.height = height;
    if (this.world) {
      const cam = this.world.resource<Camera>(Res.Camera);
      cam.viewportWidth = width;
      cam.viewportHeight = height;
    }
  }

  private buildWorld(): void {
    const world = new World();
    const palette = new TilePalette(this.assets.tiles.tiles);
    const map = new TileMap(this.assets.map, palette);
    const iso: IsoConfig = {
      tileWidth: this.assets.map.tileSize.w,
      tileHeight: this.assets.map.tileSize.h,
      heightStep: this.assets.map.tileSize.heightStep,
    };

    world.setResource(Res.Map, map);
    world.setResource(Res.Iso, iso);
    world.setResource(Res.Input, this.input);
    world.setResource(Res.Assets, this.assets);
    world.setResource(Res.Rng, new Rng(0xc0ffee));
    world.setResource<Settings>(Res.Settings, {
      showVision: true,
      showGrid: false,
      paused: false,
    });
    const camera = new Camera(this.canvas.width, this.canvas.height, iso);
    world.setResource(Res.Camera, camera);
    world.setResource(Res.Log, new GameLog(world));

    const registry = new SkillRegistry();
    registry.register(knifeAction).register(disguiseAction).register(stoneAction);
    world.setResource(Res.Skills, registry);

    // Spawn entities from the map definition.
    let player = -1;
    for (const ent of map.entities) {
      if (ent.kind === 'player') {
        player = spawnPlayer(world, ent.x, ent.y, (ent.facing as never) ?? 'S');
      } else {
        spawnGuard(world, ent);
      }
    }
    if (player < 0) throw new Error('Map has no player entity');
    world.setResource(Res.Player, player);

    // Centre the camera on the player immediately.
    const pos = world.must(player, Position);
    const ws = gridToScreen(pos.gx, pos.gy, 0, iso);
    camera.target = { x: ws.x, y: ws.y };

    // Visual props (buildings, crates) — rendered when their art is present.
    for (const prop of map.props) {
      spawnProp(world, prop.sprite, prop.x, prop.y);
    }

    // Build the system schedule (fixed update order).
    const renderSystem = new RenderSystem(this.ctx2d, this.canvas, this.images);
    const audio = new AudioSystem();
    const skillSystem = new SkillSystem();
    const inputSystem = new InputSystem({
      skills: skillSystem,
      render: renderSystem,
      onGesture: () => audio.resume(),
      requestReset: () => {
        this.pendingReset = true;
      },
    });

    const scheduler = new Scheduler();
    scheduler
      .add(inputSystem)
      .add(skillSystem)
      .add(new MovementSystem())
      .add(new VisionSystem())
      .add(new AISystem())
      .add(audio)
      .add(new SyncSystem(this.sync, this.config.saveSlot, this.config.autosaveSeconds))
      .add(new AssistantSystem(this.assistant))
      .add(renderSystem);
    scheduler.init(world);

    this.world = world;
    this.scheduler = scheduler;
    this.won = false;

    logTo(world, `Misia: ${map.name}`, 'info');
    logTo(world, 'Dostaň sa nepozorovane do kamennej chalupy.', 'info');
  }

  /** Kick off loading every referenced sprite/prop image (graceful if absent). */
  private preloadImages(): void {
    this.images.setAvailable(this.assets.images);
    const srcs: string[] = [];
    for (const def of Object.values(this.assets.sprites.sprites)) {
      if (def.image) srcs.push(def.image.src);
    }
    for (const prop of Object.values(this.assets.sprites.props ?? {})) {
      srcs.push(prop.src);
    }
    this.images.preload(srcs);
  }

  private async loadSave(): Promise<void> {
    try {
      const snap = await this.sync.load(this.config.saveSlot);
      if (snap && snap.map === this.assets.map.name) {
        applySnapshot(this.world, snap);
        logTo(this.world, `Načítaný uložený stav (${this.sync.name}).`, 'info');
      }
    } catch (err) {
      logTo(this.world, `Načítanie zlyhalo: ${(err as Error).message}`, 'warn');
    }
  }

  private loop = (t: number): void => {
    if (!this.running) return;
    if (this.pendingReset) {
      this.pendingReset = false;
      this.buildWorld();
    }

    const raw = Math.min(0.05, (t - this.last) / 1000 || 0);
    this.last = t;
    const settings = this.world.resource<Settings>(Res.Settings);
    const dt = settings.paused ? 0 : raw;
    this.elapsed += dt;
    this.frame += 1;

    this.scheduler.update(this.world, { dt, elapsed: this.elapsed, frame: this.frame });

    this.followCamera();
    this.checkObjectives();
    this.onHud?.(this.buildHud());

    requestAnimationFrame(this.loop);
  };

  private followCamera(): void {
    const map = this.world.resource<TileMap>(Res.Map);
    const player = this.world.resource<number>(Res.Player);
    const pos = this.world.get(player, Position);
    const cam = this.world.resource<Camera>(Res.Camera);
    if (pos) cam.followGrid(pos.fx, pos.fy, map.heightAt(Math.round(pos.fx), Math.round(pos.fy)));
  }

  private checkObjectives(): void {
    if (this.won) return;
    const map = this.world.resource<TileMap>(Res.Map);
    const player = this.world.resource<number>(Res.Player);
    const actor = this.world.get(player, Actor);
    const pos = this.world.get(player, Position);
    if (!actor?.alive || !pos) return;
    if (map.tileAt(Math.round(pos.gx), Math.round(pos.gy)).id === 'floor') {
      this.won = true;
      logTo(this.world, 'Misia splnená! Dostal si sa do chalupy.', 'info');
    }
  }

  private buildHud(): HudModel {
    const world = this.world;
    const map = world.resource<TileMap>(Res.Map);
    const settings = world.resource<Settings>(Res.Settings);
    const player = world.resource<number>(Res.Player);
    const actor = world.must(player, Actor);
    const skills = world.get(player, Skills);

    let worst: HudModel['alertLevel'] = 'PATROL';
    let guardsAlive = 0;
    for (const e of world.query(AIComp, Actor)) {
      const a = world.must(e, Actor);
      if (!a.alive) continue;
      guardsAlive += 1;
      const s = world.must(e, AIComp).state;
      if (s === 'ALERT') worst = 'ALERT';
      else if (s === 'SUSPICIOUS' && worst !== 'ALERT') worst = 'SUSPICIOUS';
    }

    const cooldowns = skills
      ? skills.equipped.map((id) => ({ id, remaining: skills.cooldowns[id] ?? 0 }))
      : [];

    return {
      mapName: map.name,
      hp: Math.max(0, Math.round(actor.hp)),
      maxHp: actor.maxHp,
      alive: actor.alive,
      disguised: skills?.disguised ?? false,
      selected: skills?.selected ?? '',
      cooldowns,
      alertLevel: worst,
      guardsAlive,
      paused: settings.paused,
      won: this.won,
      objective: 'Dostaň sa do kamennej chalupy',
      syncName: this.sync.name,
      assistant: this.assistant.isRemote ? 'claude' : 'local',
      log: this.world.resource<GameLog>(Res.Log).recent().map((l) => ({ ...l })),
    };
  }
}
