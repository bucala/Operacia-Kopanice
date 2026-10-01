import type { AppConfig } from '../config';
import { FirebaseSync } from './FirebaseSync';
import { LocalSync } from './LocalSync';
import type { SyncProvider } from './types';

export * from './types';
export { LocalSync } from './LocalSync';
export { FirebaseSync } from './FirebaseSync';
export { captureSnapshot, applySnapshot } from './snapshot';

/**
 * Chooses the persistence backend: cloud (Firebase Realtime Database) when it
 * is configured, otherwise the local fallback. The rest of the game depends
 * only on the {@link SyncProvider} interface.
 */
export function createSync(config: AppConfig): SyncProvider {
  if (config.firebase) return new FirebaseSync(config.firebase);
  return new LocalSync();
}
