/**
 * Tactical assistant endpoint — forwards the game's hint request to Claude.
 *
 * The browser game (src/integrations/assistant/ClaudeAssistant.ts) POSTs
 * { context, prompt } here; this route forwards the prompt to the Claude API
 * and returns { text }. The API key stays server-side and is never shipped to
 * the browser.
 *
 * Required env: ANTHROPIC_API_KEY
 * Optional env: ANTHROPIC_MODEL (default: claude-haiku-4-5)
 *
 * Point the game at this endpoint with VITE_ASSISTANT_ENDPOINT=/api/assistant
 */

import { Router } from 'express';
import Anthropic from '@anthropic-ai/sdk';

const router = Router();

const MODEL = process.env.ANTHROPIC_MODEL ?? 'claude-haiku-4-5';

const SYSTEM_PROMPT =
  'Si taktický poradca v izometrickej stealth hre Operácia Kopanice. ' +
  'Na základe stavu hry daj jednu krátku, konkrétnu radu (max 1 veta) po slovensky.';

let cachedClient: Anthropic | null = null;
function getClient(): Anthropic {
  if (!cachedClient) {
    cachedClient = new Anthropic({ maxRetries: 1, timeout: 10_000 });
  }
  return cachedClient;
}

function isConfigured(): boolean {
  return Boolean(process.env.ANTHROPIC_API_KEY || process.env.ANTHROPIC_AUTH_TOKEN);
}

/**
 * Origin allowlist check using exact URL-origin comparison.
 *
 * When ASSISTANT_ALLOWED_ORIGINS is set (comma-separated list of allowed
 * origins, e.g. "https://myapp.replit.app,https://myapp.repl.co"), only
 * requests whose `Origin` header is an exact case-insensitive match for one
 * of those origins are served.  An "origin" is scheme + host + port (no
 * path), so we parse both sides with `new URL()` and compare `.origin`.
 *
 * When ASSISTANT_ALLOWED_ORIGINS is NOT set the endpoint rejects all
 * requests.  Set it to "*" only in controlled private deployments where you
 * are certain the API key cannot be abused.
 */
function originAllowed(origin: string | undefined): boolean {
  const allow = process.env.ASSISTANT_ALLOWED_ORIGINS;
  // No allow-list configured → deny everything by default.
  if (!allow) return false;
  // Explicit wildcard — opt-in only; document the risk in your deployment.
  if (allow.trim() === '*') return true;
  if (!origin) return false;

  let requestOrigin: string;
  try {
    requestOrigin = new URL(origin).origin.toLowerCase();
  } catch {
    return false; // Malformed origin header → deny.
  }

  const allowed = allow.split(',').map((s) => {
    try {
      return new URL(s.trim()).origin.toLowerCase();
    } catch {
      return ''; // Skip malformed entries.
    }
  });

  return allowed.includes(requestOrigin);
}

router.post('/assistant', async (req, res) => {
  const origin = req.headers.origin;
  if (!originAllowed(origin)) {
    res.status(403).json({ error: 'Origin not allowed' });
    return;
  }

  if (!isConfigured()) {
    res.status(503).json({ error: 'Assistant not configured (missing ANTHROPIC_API_KEY)' });
    return;
  }

  const body = req.body as { context?: unknown; prompt?: string };
  const prompt = typeof body.prompt === 'string' ? body.prompt.slice(0, 1000) : '';
  if (!prompt) {
    res.status(400).json({ error: 'prompt is required' });
    return;
  }

  try {
    const message = await getClient().messages.create({
      model: MODEL,
      max_tokens: 120,
      system: SYSTEM_PROMPT,
      messages: [{ role: 'user', content: prompt }],
    });

    const textBlock = message.content.find((b) => b.type === 'text');
    const text = textBlock?.type === 'text' ? textBlock.text : '';
    res.json({ text });
  } catch (err) {
    req.log.error({ err }, 'assistant request failed');
    res.status(502).json({ error: 'Upstream assistant request failed' });
  }
});

export default router;
