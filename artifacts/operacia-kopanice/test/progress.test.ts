/**
 * Level unlocking and best-turn persistence for the GO puzzle.
 *
 * Covers the pure helpers directly and checks that load/save degrade
 * gracefully when `localStorage` is unavailable, since progress.ts is the
 * only piece of shipped state that isn't exercised by go.test.ts.
 */
import { describe, it, expect } from 'vitest';
import {
  bestTurns,
  emptyProgress,
  firstPlayable,
  isCompleted,
  isUnlocked,
  loadProgress,
  recordWin,
  saveProgress,
} from '../src/go/progress';

describe('GO progress', () => {
  it('level 0 is unlocked, later levels start locked', () => {
    const p = emptyProgress();
    expect(isUnlocked(p, 0)).toBe(true);
    expect(isUnlocked(p, 1)).toBe(false);
    expect(isCompleted(p, 0)).toBe(false);
  });

  it('recording a win completes a level and unlocks the next', () => {
    let p = emptyProgress();
    p = recordWin(p, 0, 7);
    expect(isCompleted(p, 0)).toBe(true);
    expect(isUnlocked(p, 1)).toBe(true);
    expect(bestTurns(p, 0)).toBe(7);
  });

  it('best turns only improve', () => {
    let p = recordWin(emptyProgress(), 0, 9);
    p = recordWin(p, 0, 12); // worse — ignored
    expect(bestTurns(p, 0)).toBe(9);
    p = recordWin(p, 0, 5); // better — kept
    expect(bestTurns(p, 0)).toBe(5);
  });

  it('recordWin is pure (does not mutate the input)', () => {
    const p0 = emptyProgress();
    recordWin(p0, 0, 3);
    expect(p0.best).toEqual({});
  });

  it('firstPlayable points at the first unlocked-but-unbeaten level', () => {
    let p = emptyProgress();
    expect(firstPlayable(p, 3)).toBe(0);
    p = recordWin(p, 0, 4);
    expect(firstPlayable(p, 3)).toBe(1);
    p = recordWin(p, 1, 4);
    p = recordWin(p, 2, 4);
    expect(firstPlayable(p, 3)).toBe(2); // all cleared — offer the last
  });

  it('load/save degrade gracefully without localStorage', () => {
    expect(loadProgress()).toEqual(emptyProgress());
    expect(() => saveProgress(recordWin(emptyProgress(), 0, 1))).not.toThrow();
  });
});
