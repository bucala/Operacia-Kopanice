import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import { Events, type ActorKilledEvent } from '@/core/events';
import { logTo, playerEntity } from '@/core/resources';
import { captureSnapshot } from '@/integrations/sync/snapshot';
import type { SyncProvider } from '@/integrations/sync/types';

/**
 * Periodically persists the game state (player position, health, inventory,
 * disguise, and enemy status) through the configured {@link SyncProvider} —
 * Firebase Realtime Database in the cloud, or local storage offline. Also saves
 * immediately when the player dies so a session always ends persisted.
 */
export class SyncSystem implements System {
  readonly name = 'sync';
  private elapsed = 0;
  private saving = false;

  constructor(
    private readonly provider: SyncProvider,
    private readonly slot: string,
    private readonly intervalSeconds: number,
  ) {}

  init(world: World): void {
    world.events.on<ActorKilledEvent>(Events.ActorKilled, (e) => {
      if (e.victim === playerEntity(world)) void this.flush(world, true);
    });
  }

  update(world: World, ctx: FrameContext): void {
    this.elapsed += ctx.dt;
    if (this.elapsed >= this.intervalSeconds) {
      this.elapsed = 0;
      void this.flush(world, false);
    }
  }

  private async flush(world: World, announce: boolean): Promise<void> {
    if (this.saving) return;
    this.saving = true;
    try {
      await this.provider.save(this.slot, captureSnapshot(world));
      if (announce) logTo(world, `Stav uložený (${this.provider.name}).`, 'info');
    } catch (err) {
      logTo(world, `Uloženie zlyhalo: ${(err as Error).message}`, 'warn');
    } finally {
      this.saving = false;
    }
  }
}
