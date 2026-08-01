import type { Component, ComponentKind } from './Component';
import type { Entity } from './Entity';
import { EventBus } from './EventBus';

/**
 * The World owns all entities, their components, and shared "resources"
 * (singletons such as the tile map, input state, and timing). Systems read and
 * mutate the World each tick.
 */
export class World {
  private nextId = 0;
  private readonly living = new Set<Entity>();
  /** componentType -> (entity -> component data) */
  private readonly stores = new Map<string, Map<Entity, Component>>();
  /** Arbitrary singletons keyed by a string name. */
  private readonly resources = new Map<string, unknown>();

  readonly events = new EventBus();

  // --- Entities --------------------------------------------------------------

  createEntity(): Entity {
    const e = this.nextId++;
    this.living.add(e);
    return e;
  }

  destroyEntity(e: Entity): void {
    if (!this.living.has(e)) return;
    this.living.delete(e);
    for (const store of this.stores.values()) store.delete(e);
  }

  isAlive(e: Entity): boolean {
    return this.living.has(e);
  }

  entities(): Iterable<Entity> {
    return this.living;
  }

  // --- Components ------------------------------------------------------------

  add<T extends Component>(e: Entity, component: T): T {
    if (!this.living.has(e)) {
      throw new Error(`Cannot add '${component.type}' to dead/unknown entity ${e}`);
    }
    let store = this.stores.get(component.type);
    if (!store) {
      store = new Map();
      this.stores.set(component.type, store);
    }
    store.set(e, component);
    return component;
  }

  get<T extends Component>(e: Entity, kind: ComponentKind<T>): T | undefined {
    return this.stores.get(kind.type)?.get(e) as T | undefined;
  }

  /** Like {@link get} but throws if the component is missing. */
  must<T extends Component>(e: Entity, kind: ComponentKind<T>): T {
    const c = this.get(e, kind);
    if (!c) throw new Error(`Entity ${e} is missing component '${kind.type}'`);
    return c;
  }

  has<T extends Component>(e: Entity, kind: ComponentKind<T>): boolean {
    return this.stores.get(kind.type)?.has(e) ?? false;
  }

  remove<T extends Component>(e: Entity, kind: ComponentKind<T>): void {
    this.stores.get(kind.type)?.delete(e);
  }

  /** All entities that have every one of the given component kinds. */
  *query(...kinds: ComponentKind<Component>[]): Generator<Entity> {
    if (kinds.length === 0) return;
    // Iterate the smallest store for efficiency.
    let smallest = this.stores.get(kinds[0].type);
    for (const k of kinds) {
      const s = this.stores.get(k.type);
      if (!s) return; // a required component has zero instances
      if (!smallest || s.size < smallest.size) smallest = s;
    }
    if (!smallest) return;
    outer: for (const e of smallest.keys()) {
      for (const k of kinds) {
        if (!this.stores.get(k.type)?.has(e)) continue outer;
      }
      yield e;
    }
  }

  /** Convenience: collect a query into an array (safe to mutate during). */
  collect(...kinds: ComponentKind<Component>[]): Entity[] {
    return [...this.query(...kinds)];
  }

  // --- Resources (singletons) ------------------------------------------------

  setResource<T>(name: string, value: T): void {
    this.resources.set(name, value);
  }

  resource<T>(name: string): T {
    if (!this.resources.has(name)) {
      throw new Error(`Resource '${name}' has not been registered`);
    }
    return this.resources.get(name) as T;
  }

  tryResource<T>(name: string): T | undefined {
    return this.resources.get(name) as T | undefined;
  }
}
