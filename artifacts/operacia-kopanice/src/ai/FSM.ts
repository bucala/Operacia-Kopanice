/**
 * A tiny, reusable finite-state machine.
 *
 * State *data* lives outside the machine (in our case on the AIComp component),
 * so a single FSM instance is shared by every agent. {@link step} runs the
 * current state's update, and if it returns a different state name, fires the
 * corresponding onExit/onEnter hooks and returns the new name.
 */
export interface FsmState<C> {
  readonly name: string;
  onEnter?(ctx: C): void;
  /** Return the next state's name to transition, or void/same to stay. */
  update(ctx: C, dt: number): string | void;
  onExit?(ctx: C): void;
}

export class FiniteStateMachine<C> {
  private readonly states = new Map<string, FsmState<C>>();

  constructor(states: FsmState<C>[]) {
    for (const s of states) this.states.set(s.name, s);
  }

  has(name: string): boolean {
    return this.states.has(name);
  }

  /** Advance one tick from `current`, returning the (possibly new) state name. */
  step(current: string, ctx: C, dt: number): string {
    const state = this.states.get(current);
    if (!state) throw new Error(`FSM has no state '${current}'`);
    const next = state.update(ctx, dt);
    if (next && next !== current) {
      if (!this.states.has(next)) throw new Error(`FSM has no state '${next}'`);
      state.onExit?.(ctx);
      this.states.get(next)!.onEnter?.(ctx);
      return next;
    }
    return current;
  }
}
