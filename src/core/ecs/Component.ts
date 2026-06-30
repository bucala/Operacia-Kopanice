/**
 * Component model for the ECS.
 *
 * A Component is plain data tagged with a string `type`. Each component module
 * exports both an interface and a same-named `ComponentKind` value (a small
 * descriptor carrying the type tag). The paired value enables fully type-safe
 * lookups: `world.get(entity, Position)` resolves to `Position | undefined`.
 */
export interface Component {
  readonly type: string;
}

/** A typed handle to a component variety, used as the key for world lookups. */
export interface ComponentKind<T extends Component> {
  readonly type: T['type'];
}

/** Helper to declare a component kind descriptor with the right literal type. */
export function kind<T extends Component>(type: T['type']): ComponentKind<T> {
  return { type };
}
