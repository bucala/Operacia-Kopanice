import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  bestTurns,
  emptyProgress,
  firstPlayable,
  isCompleted,
  isUnlocked,
  loadProgress,
  recordWin,
  saveProgress,
} from '@/go/progress';

test('level 0 is unlocked, later levels start locked', () => {
  const p = emptyProgress();
  assert.ok(isUnlocked(p, 0));
  assert.ok(!isUnlocked(p, 1));
  assert.ok(!isCompleted(p, 0));
});

test('recording a win completes a level and unlocks the next', () => {
  let p = emptyProgress();
  p = recordWin(p, 0, 7);
  assert.ok(isCompleted(p, 0));
  assert.ok(isUnlocked(p, 1));
  assert.equal(bestTurns(p, 0), 7);
});

test('best turns only improve', () => {
  let p = recordWin(emptyProgress(), 0, 9);
  p = recordWin(p, 0, 12); // worse — ignored
  assert.equal(bestTurns(p, 0), 9);
  p = recordWin(p, 0, 5); // better — kept
  assert.equal(bestTurns(p, 0), 5);
});

test('recordWin is pure (does not mutate the input)', () => {
  const p0 = emptyProgress();
  recordWin(p0, 0, 3);
  assert.deepEqual(p0.best, {});
});

test('firstPlayable points at the first unlocked-but-unbeaten level', () => {
  let p = emptyProgress();
  assert.equal(firstPlayable(p, 3), 0);
  p = recordWin(p, 0, 4);
  assert.equal(firstPlayable(p, 3), 1);
  p = recordWin(p, 1, 4);
  p = recordWin(p, 2, 4);
  assert.equal(firstPlayable(p, 3), 2); // all cleared → offer the last
});

test('load/save degrade gracefully without localStorage', () => {
  assert.deepEqual(loadProgress(), emptyProgress());
  assert.doesNotThrow(() => saveProgress(recordWin(emptyProgress(), 0, 1)));
});
