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
 * The task is a single, latency-sensitive one-sentence hint, so it's a plain
 * `messages.create` — no thinking, no streaming, a tight `max_tokens`, and a
 * fast default model. Set `ANTHROPIC_MODEL` to a more capable model for richer
 * advice. Required env: ANTHROPIC_API_KEY.
 */

// A fast, low-cost model is the right tool for a single-sentence hint; override
// with ANTHROPIC_MODEL for higher-quality advice.
const MODEL = process.env.ANTHROPIC_MODEL ?? 'claude-haiku-4-5';

const SYSTEM_PROMPT =
  'Si taktický poradca v izometrickej stealth hre Operácia Kopanice. ' +
  'Na základe stavu hry daj jednu krátku, konkrétnu radu (max 1 veta) po slovensky.';

// Lazily constructed and memoised across warm invocations.
let cached: Anthropic | null = null;
function getClient(): Anthropic {
  if (!cached) {
    // Bound latency for an interactive hint: at most one retry, 10s ceiling.
    cached = new Anthropic({ maxRetries: 1, timeout: 10_000 });
  }
  return cached;
}

// The SDK resolves the key lazily (at request time), so pre-flight the realistic
// serverless credentials to return a clean 503 rather than a generic 500.
function isConfigured(): boolean {
  return Boolean(process.env.ANTHROPIC_API_KEY || process.env.ANTHROPIC_AUTH_TOKEN);
}

// Minimal structural request/response types (Vercel Node runtime).
interface Req {
  method?: string;
  headers?: Record<string, string | string[] | undefined>;
  body?: { context?: unknown; prompt?: string } | string;
}
interface Res {
  status(code: number): Res;
  json(data: unknown): void;
}

/**
 * Optional abuse guard: when ASSISTANT_ALLOWED_ORIGINS is set (comma-separated),
 * only requests from those origins are served — otherwise this public endpoint
 * could be used to proxy the API key's quota. Unset = no enforcement.
 */
function originAllowed(req: Req): boolean {
  const allow = process.env.ASSISTANT_ALLOWED_ORIGINS;
  if (!allow) return true;
  const origin = req.headers?.origin;
  const value = Array.isArray(origin) ? origin[0] : origin;
  return !!value && allow.split(',').map((s) => s.trim()).includes(value);
}

export default async function handler(req: Req, res: Res): Promise<void> {
  if (req.method !== 'POST') {
    res.status(405).json({ error: 'method not allowed' });
    return;
  }
  if (!originAllowed(req)) {
    res.status(403).json({ error: 'forbidden' });
    return;
  }

  if (!isConfigured()) {
    // No credentials — the game falls back to its offline advisor.
    res.status(503).json({ error: 'assistant not configured' });
    return;
  }

  const body = parseBody(req.body);
  const prompt = body.prompt ?? JSON.stringify(body.context ?? {});

  try {
    const message = await getClient().messages.create({
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
    // Typed chain, most specific first. The client treats any non-200 as a
    // signal to fall back locally, so exact codes are a nicety, not load-bearing.
    if (err instanceof Anthropic.APIConnectionError) {
      res.status(504).json({ error: 'assistant upstream timeout' });
    } else if (err instanceof Anthropic.APIError) {
      res.status(err.status ?? 502).json({ error: err.message });
    } else {
      console.error('assistant error', err);
      res.status(500).json({ error: 'assistant error' });
    }
  }
}

function parseBody(raw: Req['body']): { context?: unknown; prompt?: string } {
  if (typeof raw === 'string') {
    try {
      return JSON.parse(raw) as { context?: unknown; prompt?: string };
    } catch {
      return {};
    }
  }
  return raw ?? {};
}
