import type { FirebaseConfig } from './sync/FirebaseSync';

export interface AppConfig {
  firebase: FirebaseConfig | null;
  assistantEndpoint: string | null;
  saveSlot: string;
  autosaveSeconds: number;
}

function env(): Partial<ImportMetaEnv> {
  // import.meta.env is provided by Vite; guard for non-bundled (test) contexts.
  try {
    return import.meta.env ?? {};
  } catch {
    return {};
  }
}

/** Resolves integration configuration from Vite env vars (see .env.example). */
export function readConfig(): AppConfig {
  const e = env();
  const firebase: FirebaseConfig | null =
    e.VITE_FIREBASE_API_KEY && e.VITE_FIREBASE_DATABASE_URL
      ? {
          apiKey: e.VITE_FIREBASE_API_KEY,
          authDomain: e.VITE_FIREBASE_AUTH_DOMAIN ?? '',
          databaseURL: e.VITE_FIREBASE_DATABASE_URL,
          projectId: e.VITE_FIREBASE_PROJECT_ID ?? '',
        }
      : null;

  return {
    firebase,
    assistantEndpoint: e.VITE_ASSISTANT_ENDPOINT ?? null,
    saveSlot: e.VITE_SAVE_SLOT ?? 'default',
    autosaveSeconds: 8,
  };
}
