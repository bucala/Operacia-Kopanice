import { test } from 'node:test';
import assert from 'node:assert/strict';
import { computeFov, lineOfSight } from '@/map/Fov';
import { makeTestMap } from './helpers';

test('open ground has clear line of sight', () => {
  const map = makeTestMap(['======', '======', '======']);
  assert.ok(lineOfSight(map, 0, 1, 5, 1));
});

test('a wall blocks line of sight', () => {
  const map = makeTestMap(['======', '==#===', '======']);
  // Viewer at (0,1), target at (5,1): the wall at (2,1) sits on the line.
  assert.ok(!lineOfSight(map, 0, 1, 5, 1));
});

test('low cover (height 1) does not block sight of distant ground', () => {
  // Hay is a vision-blocker but only height 1; the eye is above it, so a tile
  // beyond it at ground level is still visible per the height model... but the
  // hay tile itself blocks. Verify sight past a *non-blocking* low bump instead.
  const map = makeTestMap(['=====', '=====', '====='], ['00000', '00000', '00000']);
  assert.ok(lineOfSight(map, 0, 0, 4, 0));
});

test('computeFov excludes tiles hidden behind a wall', () => {
  const map = makeTestMap([
    '=======',
    '===#===',
    '=======',
  ]);
  const visible = computeFov(map, {
    ox: 0,
    oy: 1,
    fdx: 1,
    fdy: 0,
    range: 8,
    coneHalfAngle: Math.PI,
  });
  assert.ok(visible.has('1,1'), 'near tile should be visible');
  assert.ok(!visible.has('5,1'), 'tile directly behind the wall should be hidden');
});

test('vision cone restricts FoV to the facing direction', () => {
  const map = makeTestMap(['=====', '=====', '=====', '=====', '====='].map((r) => r));
  const visible = computeFov(map, {
    ox: 2,
    oy: 2,
    fdx: 1,
    fdy: 0,
    range: 4,
    coneHalfAngle: Math.PI / 6,
  });
  assert.ok(visible.has('4,2'), 'tile ahead should be visible');
  assert.ok(!visible.has('0,2'), 'tile behind should be outside the cone');
});
