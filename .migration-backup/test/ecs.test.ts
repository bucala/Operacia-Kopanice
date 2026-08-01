import { test } from 'node:test';
import assert from 'node:assert/strict';
import { World } from '@/core/ecs/World';
import { Position, makePosition } from '@/components/Position';
import { Actor, makeActor } from '@/components/Actor';

test('add/get/has/remove components', () => {
  const world = new World();
  const e = world.createEntity();
  world.add(e, makePosition(3, 4, 'N'));
  assert.ok(world.has(e, Position));
  assert.equal(world.get(e, Position)?.gx, 3);
  world.remove(e, Position);
  assert.ok(!world.has(e, Position));
});

test('query returns only entities with all components', () => {
  const world = new World();
  const a = world.createEntity();
  world.add(a, makePosition(0, 0));
  world.add(a, makeActor('A'));
  const b = world.createEntity();
  world.add(b, makePosition(1, 1)); // no Actor

  const matches = [...world.query(Position, Actor)];
  assert.deepEqual(matches, [a]);
});

test('destroyEntity removes all of its components', () => {
  const world = new World();
  const e = world.createEntity();
  world.add(e, makePosition(0, 0));
  world.add(e, makeActor('A'));
  world.destroyEntity(e);
  assert.ok(!world.isAlive(e));
  assert.equal([...world.query(Position)].length, 0);
});

test('event bus delivers and unsubscribes', () => {
  const world = new World();
  let count = 0;
  const off = world.events.on<number>('ping', (n) => (count += n));
  world.events.emit('ping', 2);
  world.events.emit('ping', 3);
  off();
  world.events.emit('ping', 10);
  assert.equal(count, 5);
});

test('resources throw when missing and return when set', () => {
  const world = new World();
  assert.throws(() => world.resource('nope'));
  world.setResource('answer', 42);
  assert.equal(world.resource<number>('answer'), 42);
});
