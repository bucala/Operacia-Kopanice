import Anthropic from '@anthropic-ai/sdk';

/**
 * Serverless tactical-assistant endpoint (Vercel Node function).
 *
 * The browser game (src/integrations/assistant/ClaudeAssistant.ts) POSTs
 * `{ context, prompt }` here; this function forwards the prompt to the Claude
 * API and returns `{ text }`. The API key stays server-side, so it is never
 * shipped to the browser. Point the game at this endpoint with
 * VITE_ASSISTANT_ENDPOINT=/api/assistant.
 *
 * Required env: ANTHROPIC_API_KEY. Optional: ANTHROPIC_MODEL (defaults to a
 * fast model well-suited to a single-sentence hint; set it to a more capable
 * model for richer advice).
 */

const client = new Anthropic(); // reads ANTHROPIC_API_KEY from the environment

const MODEL = process.env.ANTHROPIC_MODEL ?? 'claude-haiku-4-5';

const SYSTEM_PROMPT =
  'Si taktický poradca v izometrickej stealth hre Operácia Kopanice. ' +
  'Na základe stavu hry daj jednu krátku, konkrétnu radu (max 1 veta) po slovensky.';

// Minimal structural request/response types (Vercel Node runtime).
interface Req {
  method?: string;
  body?: { context?: unknown; prompt?: string } | string;
}
interface Res {
  status(code: number): Res;
  json(data: unknown): void;
}

export default async function handler(req: Req, res: Res): Promise<void> {
  if (req.method !== 'POST') {
    res.status(405).json({ error: 'method not allowed' });
    return;
  }

  try {
    const body = typeof req.body === 'string' ? JSON.parse(req.body) : (req.body ?? {});
    const prompt: string = body.prompt ?? JSON.stringify(body.context ?? {});

    const message = await client.messages.create({
      model: MODEL,
      max_tokens: 256,
      system: SYSTEM_PROMPT,
      messages: [{ role: 'user', content: prompt }],
    });

    const text = message.content
      .filter((block): block is Anthropic.TextBlock => block.type === 'text')
      .map((block) => block.text)
      .join(' ')
      .trim();

    res.status(200).json({ text });
  } catch (err) {
    res.status(500).json({ error: (err as Error).message ?? 'assistant error' });
  }
}
