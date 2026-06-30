import type { AIState } from '@/components/AIComp';
import type { ItemStack } from '@/components/Inventory';
import type { Facing } from '@/components/Position';

/** A serialisable snapshot of the game state synced to the cloud DB. */
export interface GameSnapshot {
  /** Schema version, for forward migration. */
  version: number;
  /** Unix ms when the snapshot was produced. */
  savedAt: number;
  map: string;
  player: {
    gx: number;
    gy: number;
    facing: Facing;
    hp: number;
    alive: boolean;
    disguised: boolean;
    inventory: ItemStack[];
    skills: string[];
  };
  enemies: { name: string; gx: number; gy: number; state: AIState; alive: boolean }[];
}

/**
 * Pluggable persistence backend. Implementations include cloud (Firebase
 * Realtime Database) and a local fallback, all behind this one interface so the
 * game never depends on a specific provider.
 */
export interface SyncProvider {
  readonly name: string;
  /** Persist the snapshot for the given save slot. */
  save(slot: string, snapshot: GameSnapshot): Promise<void>;
  /** Load a snapshot, or null if none exists. */
  load(slot: string): Promise<GameSnapshot | null>;
  /** Subscribe to remote changes (e.g. another device). Returns an unsubscribe. */
  subscribe?(slot: string, onChange: (snapshot: GameSnapshot) => void): () => void;
}
