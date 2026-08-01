import type { GameSnapshot, SyncProvider } from './types';

/**
 * Offline persistence backed by localStorage (with an in-memory fallback for
 * non-browser environments such as tests). Used automatically when no Firebase
 * configuration is present, so saving/loading always works out of the box.
 */
export class LocalSync implements SyncProvider {
  readonly name = 'local';
  private readonly memory = new Map<string, string>();

  private key(slot: string): string {
    return `kopanice:save:${slot}`;
  }

  private get store(): Storage | null {
    return typeof localStorage !== 'undefined' ? localStorage : null;
  }

  async save(slot: string, snapshot: GameSnapshot): Promise<void> {
    const data = JSON.stringify(snapshot);
    if (this.store) this.store.setItem(this.key(slot), data);
    else this.memory.set(this.key(slot), data);
  }

  async load(slot: string): Promise<GameSnapshot | null> {
    const data = this.store ? this.store.getItem(this.key(slot)) : this.memory.get(this.key(slot));
    return data ? (JSON.parse(data) as GameSnapshot) : null;
  }
}
