import { test } from 'node:test';
import assert from 'node:assert/strict';
import { FiniteStateMachine, type FsmState } from '@/ai/FSM';

interface Ctx {
  trail: string[];
  trigger: boolean;
}

const a: FsmState<Ctx> = {
  name: 'A',
  onEnter: (c) => c.trail.push('enterA'),
  onExit: (c) => c.trail.push('exitA'),
  update: (c) => (c.trigger ? 'B' : undefined),
};
const b: FsmState<Ctx> = {
  name: 'B',
  onEnter: (c) => c.trail.push('enterB'),
  update: () => undefined,
};

test('stays in state when update returns nothing', () => {
  const fsm = new FiniteStateMachine([a, b]);
  const ctx: Ctx = { trail: [], trigger: false };
  assert.equal(fsm.step('A', ctx, 0.1), 'A');
  assert.deepEqual(ctx.trail, []);
});

test('transitions and fires exit/enter hooks', () => {
  const fsm = new FiniteStateMachine([a, b]);
  const ctx: Ctx = { trail: [], trigger: true };
  const next = fsm.step('A', ctx, 0.1);
  assert.equal(next, 'B');
  assert.deepEqual(ctx.trail, ['exitA', 'enterB']);
});

test('throws on unknown state', () => {
  const fsm = new FiniteStateMachine([a, b]);
  assert.throws(() => fsm.step('Z', { trail: [], trigger: false }, 0.1));
});
