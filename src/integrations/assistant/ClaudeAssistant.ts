import { localAdvice, type TacticalContext } from './Advisor';

export interface AssistantReply {
  text: string;
  /** 'claude' when answered by the remote model, 'local' for the fallback. */
  source: 'claude' | 'local';
}

/**
 * Bridge to Claude Code acting as an in-game tactical assistant.
 *
 * When an endpoint is configured (VITE_ASSISTANT_ENDPOINT — a small serverless
 * function that forwards the prompt to the Claude API), the assistant POSTs the
 * structured tactical context and returns Claude's advice. With no endpoint, or
 * on any error, it transparently falls back to the deterministic local advisor,
 * so the assistant is always available.
 *
 * The serverless function is expected to accept `{ context, prompt }` and reply
 * with `{ text }`. Keeping the API key server-side avoids shipping secrets to
 * the browser.
 */
export class ClaudeAssistant {
  constructor(private readonly endpoint: string | null) {}

  get isRemote(): boolean {
    return this.endpoint !== null;
  }

  async advise(ctx: TacticalContext): Promise<AssistantReply> {
    if (!this.endpoint) return { text: localAdvice(ctx), source: 'local' };
    // Abort a slow endpoint so a hung request can't stall the in-game advisor.
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 6000);
    try {
      const res = await fetch(this.endpoint, {
        method: 'POST',
        headers: { 'content-type': 'application/json' },
        body: JSON.stringify({ context: ctx, prompt: buildPrompt(ctx) }),
        signal: controller.signal,
      });
      if (!res.ok) throw new Error(`assistant HTTP ${res.status}`);
      const data = (await res.json()) as { text?: string };
      if (!data.text) throw new Error('assistant returned no text');
      return { text: data.text, source: 'claude' };
    } catch {
      // Timeout, network, or endpoint failure → graceful offline advice.
      return { text: localAdvice(ctx), source: 'local' };
    } finally {
      clearTimeout(timer);
    }
  }
}

/** The natural-language prompt sent to Claude alongside the structured context. */
export function buildPrompt(ctx: TacticalContext): string {
  return [
    'Si taktický poradca v izometrickej stealth hre Operácia Kopanice.',
    'Na základe stavu daj jednu krátku, konkrétnu radu (max 1 veta) po slovensky.',
    `Stav: ${JSON.stringify(ctx)}`,
  ].join('\n');
}
