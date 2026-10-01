#!/usr/bin/env node
/**
 * Rebuilds public/assets/sprites/manifest.json by scanning the sprites folder
 * for image files. Run this after dropping new art:
 *
 *   node scripts/genmanifest.mjs
 *
 * The renderer only loads images listed in the manifest, so this keeps the
 * console free of 404s for art that hasn't been added yet. Paths are stored
 * relative to public/assets (e.g. "sprites/agent.png").
 */
import { readdirSync, writeFileSync, statSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve, join, relative } from 'node:path';

const here = dirname(fileURLToPath(import.meta.url));
const assetsDir = resolve(here, '..', 'public', 'assets');
const spritesDir = join(assetsDir, 'sprites');
const IMAGE_RE = /\.(png|jpe?g|webp|gif)$/i;

/** Recursively collect image files under a directory. */
function walk(dir) {
  const out = [];
  for (const name of readdirSync(dir)) {
    const full = join(dir, name);
    if (statSync(full).isDirectory()) out.push(...walk(full));
    else if (IMAGE_RE.test(name)) out.push(full);
  }
  return out;
}

let files = [];
try {
  files = walk(spritesDir);
} catch {
  // sprites/ may not exist yet — that's fine, the manifest is just empty.
}

const available = files
  .map((f) => relative(assetsDir, f).split('\\').join('/'))
  .sort();

const out = join(spritesDir, 'manifest.json');
mkdirSync(spritesDir, { recursive: true }); // ensure the folder exists on a fresh tree
const json = {
  $schema: '../schema/manifest.schema.json',
  description:
    'Allowlist of sprite/prop image files that actually exist under public/assets. The renderer only loads images listed here. Regenerate with `node scripts/genmanifest.mjs`. See docs/ASSETS.md.',
  available,
};
writeFileSync(out, JSON.stringify(json, null, 2) + '\n');
console.log(`Wrote ${out} (${available.length} image${available.length === 1 ? '' : 's'})`);
for (const a of available) console.log(`  · ${a}`);
