import type { MapFile } from '@/map/TileMap';
import type { TilesFile } from '@/map/tiles';
import type { AssetBundle, AudioFile, SpritesFile } from './types';

const BASE = 'assets';

async function fetchJson<T>(path: string): Promise<T> {
  const res = await fetch(`${BASE}/${path}`);
  if (!res.ok) throw new Error(`Failed to load asset '${path}': ${res.status}`);
  return (await res.json()) as T;
}

/**
 * Loads every JSON asset the game needs. Because all art and audio are defined
 * as data (no binaries), this is the single place the engine reaches out for
 * content; swapping in a different map is just a different filename.
 */
export async function loadAssets(mapName = 'kopanice'): Promise<AssetBundle> {
  const [tiles, sprites, audio, map, manifest] = await Promise.all([
    fetchJson<TilesFile>('tiles.json'),
    fetchJson<SpritesFile>('sprites.json'),
    fetchJson<AudioFile>('audio.json'),
    fetchJson<MapFile>(`maps/${mapName}.json`),
    fetchJson<{ available: string[] }>('sprites/manifest.json').catch(() => ({ available: [] })),
  ]);
  return { tiles, sprites, audio, map, images: manifest.available ?? [] };
}
