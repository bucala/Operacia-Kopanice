import { test } from 'node:test';
import assert from 'node:assert/strict';
import { findPath } from '@/map/Pathfinding';
import { makeTestMap } from './helpers';

test('finds a direct path on open ground', () => {
  const map = makeTestMap(['=====', '=====', '====='].map((r) => r));
  const path = findPath(map, { x: 0, y: 1 }, { x: 4, y: 1 });
  assert.ok(path.length > 0);
  assert.deepEqual(path[path.length - 1], { x: 4, y: 1 });
});

test('returns empty path when the goal is unreachable', () => {
  const map = makeTestMap(['=#=', '=#=', '=#=']);
  const path = findPath(map, { x: 0, y: 1 }, { x: 2, y: 1 });
  assert.equal(path.length, 0);
});

test('returns empty path when standing on the goal', () => {
  const map = makeTestMap(['===', '===']);
  assert.equal(findPath(map, { x: 1, y: 0 }, { x: 1, y: 0 }).length, 0);
});

test('routes around a wall obstacle', () => {
  // Wall blocks the straight line; path must detour and never enter a wall.
  const map = makeTestMap([
    '=====',
    '==#==',
    '=====',
  ]);
  const path = findPath(map, { x: 0, y: 1 }, { x: 4, y: 1 });
  assert.ok(path.length > 0);
  assert.ok(path.every((n) => !(n.x === 2 && n.y === 1)));
});

test('prefers the cheap road over expensive snow', () => {
  // Middle row is snow (cost 5); the road rows above/below are far cheaper,
  // so the optimal path should leave the straight snow line.
  const map = makeTestMap([
    '=====',
    '.....',
    '=====',
  ]);
  const path = findPath(map, { x: 0, y: 1 }, { x: 4, y: 1 });
  assert.ok(path.length > 0);
  assert.ok(
    path.some((n) => n.y === 0 || n.y === 2),
    'expected the path to use a road row instead of staying on snow',
  );
});
