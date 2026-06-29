import type { GameSnapshot, SyncProvider } from './types';

export interface FirebaseConfig {
  apiKey: string;
  authDomain: string;
  databaseURL: string;
  projectId: string;
}

/**
 * Cloud sync against the Firebase Realtime Database.
 *
 * The Firebase SDK is loaded lazily from the CDN on first use, so it is only
 * fetched when cloud sync is actually configured — the core game has no build
 * dependency on Firebase. Configure via the VITE_FIREBASE_* env vars (see
 * .env.example) and saves/loads will go to `saves/<slot>` in your database,
 * with optional realtime subscription for cross-device play.
 */
export class FirebaseSync implements SyncProvider {
  readonly name = 'firebase';
  private dbPromise: Promise<FirebaseDb> | null = null;

  constructor(private readonly config: FirebaseConfig) {}

  private async db(): Promise<FirebaseDb> {
    if (!this.dbPromise) {
      this.dbPromise = (async () => {
        // URLs held in variables so the bundler/TS treat these as runtime-only
        // dynamic imports (no build-time resolution of the Firebase CDN SDK).
        const appUrl = 'https://www.gstatic.com/firebasejs/10.12.0/firebase-app.js';
        const dbUrl = 'https://www.gstatic.com/firebasejs/10.12.0/firebase-database.js';
        const appMod = (await import(/* @vite-ignore */ appUrl)) as {
          initializeApp(config: FirebaseConfig): unknown;
        };
        const dbMod = (await import(/* @vite-ignore */ dbUrl)) as FirebaseDbModule;
        const app = appMod.initializeApp(this.config);
        const database = dbMod.getDatabase(app);
        return { dbMod, database };
      })();
    }
    return this.dbPromise;
  }

  async save(slot: string, snapshot: GameSnapshot): Promise<void> {
    const { dbMod, database } = await this.db();
    await dbMod.set(dbMod.ref(database, `saves/${slot}`), snapshot);
  }

  async load(slot: string): Promise<GameSnapshot | null> {
    const { dbMod, database } = await this.db();
    const snap = await dbMod.get(dbMod.ref(database, `saves/${slot}`));
    return snap.exists() ? (snap.val() as GameSnapshot) : null;
  }

  subscribe(slot: string, onChange: (snapshot: GameSnapshot) => void): () => void {
    let unsub = () => {};
    void this.db().then(({ dbMod, database }) => {
      unsub = dbMod.onValue(dbMod.ref(database, `saves/${slot}`), (snap: FirebaseSnapshot) => {
        if (snap.exists()) onChange(snap.val() as GameSnapshot);
      });
    });
    return () => unsub();
  }
}

// Minimal structural types for the dynamically-imported SDK surface we use.
interface FirebaseSnapshot {
  exists(): boolean;
  val(): unknown;
}
interface FirebaseDbModule {
  getDatabase(app: unknown): unknown;
  ref(db: unknown, path: string): unknown;
  set(ref: unknown, value: unknown): Promise<void>;
  get(ref: unknown): Promise<FirebaseSnapshot>;
  onValue(ref: unknown, cb: (snap: FirebaseSnapshot) => void): () => void;
}
interface FirebaseDb {
  dbMod: FirebaseDbModule;
  database: unknown;
}
