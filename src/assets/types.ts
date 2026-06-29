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

export interface SpriteDef {
  kind: 'character';
  bodyHeight: number;
  radius: number;
  palette: Record<string, string>;
}

export interface SpritesFile {
  sprites: Record<string, SpriteDef>;
}

/** Everything loaded from the JSON asset files at startup. */
export interface AssetBundle {
  tiles: TilesFile;
  sprites: SpritesFile;
  audio: AudioFile;
  map: MapFile;
}
