import { type Component, kind } from '@/core/ecs/Component';

/** Render layers control coarse draw order independent of depth sorting. */
export type RenderLayer = 'ground' | 'object' | 'actor' | 'overlay';

/**
 * Visual representation of an entity. `sprite` references an id in the sprite
 * atlas (see assets/sprites.json); the RenderSystem draws it procedurally with
 * isometric shading for a sense of depth and volume.
 */
export interface Render extends Component {
  type: 'render';
  sprite: string;
  layer: RenderLayer;
  /** Multiplicative tint, e.g. for disguises or damage flashes (#rrggbb or null). */
  tint: string | null;
  /** Uniform scale factor. */
  scale: number;
  /** Vertical bob offset in pixels (e.g. while walking). */
  bob: number;
  /** 0 = invisible, 1 = fully visible (driven by fog-of-war). */
  alpha: number;
  /** When true, the entity is drawn dimmed if outside current vision. */
  fogged: boolean;
}

export const Render = kind<Render>('render');

export function makeRender(sprite: string, layer: RenderLayer = 'actor'): Render {
  return {
    type: 'render',
    sprite,
    layer,
    tint: null,
    scale: 1,
    bob: 0,
    alpha: 1,
    fogged: layer !== 'actor',
  };
}
