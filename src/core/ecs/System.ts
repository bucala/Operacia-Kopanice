import type { World } from './World';

/** Timing data passed to every system each frame. */
export interface FrameContext {
  /** Seconds elapsed since the previous frame (clamped). */
  dt: number;
  /** Total seconds since the game started. */
  elapsed: number;
  /** Monotonic frame counter. */
  frame: number;
}

/**
 * A System encapsulates one slice of game behaviour. Systems are registered
 * with the {@link Scheduler} and invoked in order every tick.
 */
export interface System {
  readonly name: string;
  /** Optional one-time setup once the World is fully populated. */
  init?(world: World): void;
  /** Per-frame update. */
  update(world: World, ctx: FrameContext): void;
}

/** Runs systems in a fixed order each frame. */
export class Scheduler {
  private readonly systems: System[] = [];

  add(system: System): this {
    this.systems.push(system);
    return this;
  }

  init(world: World): void {
    for (const s of this.systems) s.init?.(world);
  }

  update(world: World, ctx: FrameContext): void {
    for (const s of this.systems) s.update(world, ctx);
  }

  list(): readonly System[] {
    return this.systems;
  }
}
