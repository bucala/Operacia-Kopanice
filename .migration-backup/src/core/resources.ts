import type { Entity } from './ecs/Entity';
import { Events, type LogEvent } from './events';
import type { World } from './ecs/World';

/** String keys for World resources (singletons), centralised to avoid typos. */
export const Res = {
  Map: 'map',
  Camera: 'camera',
  Input: 'input',
  Iso: 'iso',
  Skills: 'skillRegistry',
  Assets: 'assets',
  Rng: 'rng',
  Player: 'player',
  Log: 'log',
  Settings: 'settings',
} as const;

/** Runtime-tweakable rendering/debug flags. */
export interface Settings {
  showVision: boolean;
  showGrid: boolean;
  paused: boolean;
}

/** Rolling on-screen action log. */
export class GameLog {
  private readonly lines: { message: string; level: LogEvent['level'] }[] = [];

  constructor(
    world: World,
    private readonly max = 8,
  ) {
    world.events.on<LogEvent>(Events.Log, (e) => this.push(e.message, e.level));
  }

  push(message: string, level: LogEvent['level'] = 'info'): void {
    this.lines.push({ message, level });
    while (this.lines.length > this.max) this.lines.shift();
  }

  recent(): readonly { message: string; level: LogEvent['level'] }[] {
    return this.lines;
  }
}

/** Convenience helper used throughout systems to log to the HUD. */
export function logTo(world: World, message: string, level: LogEvent['level'] = 'info'): void {
  world.events.emit<LogEvent>(Events.Log, { message, level });
}

export function playerEntity(world: World): Entity {
  return world.resource<Entity>(Res.Player);
}
