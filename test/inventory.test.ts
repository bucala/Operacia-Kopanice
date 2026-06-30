import { test } from 'node:test';
import assert from 'node:assert/strict';
import { addItem, makeInventory, removeItem, totalWeight } from '@/components/Inventory';
import { BinaryHeap } from '@/core/util/BinaryHeap';

test('adds, stacks, and weighs items', () => {
  const inv = makeInventory(10);
  assert.ok(addItem(inv, { id: 'stone', name: 'Kameň', qty: 2, weight: 1 }));
  assert.ok(addItem(inv, { id: 'stone', name: 'Kameň', qty: 1, weight: 1 }));
  assert.equal(inv.items.length, 1);
  assert.equal(inv.items[0].qty, 3);
  assert.equal(totalWeight(inv), 3);
});

test('rejects items over the weight limit', () => {
  const inv = makeInventory(2);
  assert.ok(!addItem(inv, { id: 'anvil', name: 'Nákova', qty: 1, weight: 5 }));
  assert.equal(inv.items.length, 0);
});

test('removes items and drops empty stacks', () => {
  const inv = makeInventory(10, [{ id: 'stone', name: 'Kameň', qty: 2, weight: 1 }]);
  assert.ok(removeItem(inv, 'stone', 2));
  assert.equal(inv.items.length, 0);
  assert.ok(!removeItem(inv, 'stone', 1));
});

test('binary heap pops in ascending score order', () => {
  const heap = new BinaryHeap<number>((n) => n);
  for (const n of [5, 1, 8, 3, 2, 9, 0]) heap.push(n);
  const out: number[] = [];
  while (heap.size > 0) out.push(heap.pop()!);
  assert.deepEqual(out, [0, 1, 2, 3, 5, 8, 9]);
});
