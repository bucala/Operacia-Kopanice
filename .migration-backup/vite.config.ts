import { defineConfig } from 'vite';
import { fileURLToPath, URL } from 'node:url';

// Build configuration for the Operácia Kopanice game core.
// Outputs a static bundle that can be deployed directly to Vercel.
export default defineConfig({
  base: './',
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  build: {
    target: 'es2022',
    outDir: 'dist',
    sourcemap: true,
  },
  server: {
    port: 5173,
    open: false,
  },
});
