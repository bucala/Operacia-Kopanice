import type { TilesFile } from '@/map/tiles';
import type { MapFile } from '@/map/TileMap';

export type WaveKind = 'noise' | 'sine' | 'square' | 'triangle' | 'sawtooth';

export interface SoundDef {
  wave: WaveKind;
  freq: number;
  duration: number;
  gain: number;
  category: string;
}

export interface OcclusionConfig {
  perBlocker: number;
  perHeight: number;
  minGain: number;
  maxDistance: number;
}

export interface AudioFile {
  sounds: Record<string, SoundDef>;
  occlusion: OcclusionConfig;
}

/** Optional reference to a high-fidelity image used in place of procedural art. */
export interface ImageRef {
  /** Path under public/assets, e.g. "sprites/agent.png". */
  src: string;
  /** Optional sub-rectangle [x, y, w, h] for slicing a spritesheet. */
  rect?: [number, number, number, number];
  /** Pivot within the drawn image, 0..1 (where it sits on the tile). */
  anchorX: number;
  anchorY: number;
  /** Drawn width as a multiple of one tile width (aspect ratio preserved). */
  scale: number;
}

/**
 * A character sprite. Drawn procedurally as a shaded isometric volume, OR — when
 * `image` is set and the file has loaded — as the supplied high-fidelity render.
 * The procedural fields stay as the fallback and drive HUD-bar placement.
 */
export interface SpriteDef {
  kind: 'character';
  bodyHeight: number;
  radius: number;
  palette: Record<string, string>;
  image?: ImageRef;
}

/** A purely image-based prop (building, crate, decoration). No procedural form. */
export interface PropDef extends ImageRef {
  /** Coarse draw order relative to other props/tiles on the same cell. */
  layer?: 'object' | 'overlay';
}

export interface SpritesFile {
  sprites: Record<string, SpriteDef>;
  /** Image-only props referenced by map placements and tile decorations. */
  props?: Record<string, PropDef>;
}

/** Everything loaded from the JSON asset files at startup. */
export interface AssetBundle {
  tiles: TilesFile;
  sprites: SpritesFile;
  audio: AudioFile;
  map: MapFile;
  /** Sprite/prop image paths known to exist (from sprites/manifest.json). */
  images: string[];
}
