/**
 * Level progression + best-score persistence for the GO puzzle.
 *
 * The pure helpers (`recordWin`, `isCompleted`, `isUnlocked`, `firstPlayable`)
 * are unit-tested; only `loadProgress`/`saveProgress` touch `localStorage`, and
 * both are guarded so the game still runs where storage is unavailable.
 */
export interface Progress {
  /** Fewest turns used to clear each completed level, keyed by level index. */
  best: Record<number, number>;
}

const STORAGE_KEY = 'operacia-kopanice/go/progress';

export function emptyProgress(): Progress {
  return { best: {} };
}

export function loadProgress(): Progress {
  try {
    const raw = globalThis.localStorage?.getItem(STORAGE_KEY);
    if (!raw) return emptyProgress();
    const parsed = JSON.parse(raw) as Partial<Progress> | null;
    if (parsed && typeof parsed === 'object' && parsed.best && typeof parsed.best === 'object') {
      return { best: { ...parsed.best } };
    }
  } catch {
    // Corrupt or unavailable storage — fall back to a clean slate.
  }
  return emptyProgress();
}

export function saveProgress(p: Progress): void {
  try {
    globalThis.localStorage?.setItem(STORAGE_KEY, JSON.stringify(p));
  } catch {
    // Storage disabled (private mode, etc.) — progress just won't persist.
  }
}

/** A new Progress recording a clear of `index` in `turns` (keeps the best). */
export function recordWin(p: Progress, index: number, turns: number): Progress {
  const prev = p.best[index];
  const best = prev === undefined ? turns : Math.min(prev, turns);
  return { best: { ...p.best, [index]: best } };
}

export function isCompleted(p: Progress, index: number): boolean {
  return Object.prototype.hasOwnProperty.call(p.best, index);
}

/** Level 0 is always open; every other level unlocks when its predecessor clears. */
export function isUnlocked(p: Progress, index: number): boolean {
  return index === 0 || isCompleted(p, index - 1);
}

export function bestTurns(p: Progress, index: number): number | null {
  return isCompleted(p, index) ? p.best[index] : null;
}

/** The level the "Play" button should jump to: first unlocked-but-unbeaten. */
export function firstPlayable(p: Progress, count: number): number {
  for (let i = 0; i < count; i++) {
    if (isUnlocked(p, i) && !isCompleted(p, i)) return i;
  }
  return count - 1; // everything cleared — offer the last level
}
