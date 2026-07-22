import type { FrameContext, System } from '@/core/ecs/System';
import type { World } from '@/core/ecs/World';
import { Events, type AIStateChangedEvent } from '@/core/events';
import { logTo } from '@/core/resources';
import { readTacticalContext } from '@/integrations/assistant/Advisor';
import type { ClaudeAssistant } from '@/integrations/assistant/ClaudeAssistant';

/** Minimum seconds between assistant hints, to avoid spamming the log. */
const COOLDOWN = 4;

/**
 * Surfaces tactical advice from the Claude Code assistant. It requests a hint
 * automatically when a guard first goes ALERT, and on demand (the `h` key emits
 * an AssistantRequest event). Replies are rate-limited and tagged by source
 * (remote Claude vs. local fallback).
 */
export class AssistantSystem implements System {
  readonly name = 'assistant';
  private cooldown = 0;
  private pending = false;

  constructor(private readonly assistant: ClaudeAssistant) {}

  init(world: World): void {
    world.events.on<AIStateChangedEvent>(Events.AIStateChanged, (e) => {
      if (e.to === 'ALERT') this.request(world);
    });
    world.events.on(Events.AssistantRequest, () => this.request(world, true));
  }

  update(_world: World, ctx: FrameContext): void {
    if (this.cooldown > 0) this.cooldown -= ctx.dt;
  }

  private request(world: World, force = false): void {
    if (this.pending) return;
    if (!force && this.cooldown > 0) return;
    this.cooldown = COOLDOWN;
    this.pending = true;

    const ctx = readTacticalContext(world);
    void this.assistant
      .advise(ctx)
      .then((reply) => {
        const tag = reply.source === 'claude' ? 'Claude' : 'Poradca';
        logTo(world, `${tag}: ${reply.text}`, 'info');
      })
      .finally(() => {
        this.pending = false;
      });
  }
}
