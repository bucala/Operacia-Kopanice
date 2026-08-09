# Art assets

The GO renderer (`src/go/GoRenderer.ts`) draws **procedurally by default**
(shaded isometric shapes) and upgrades to a photo/render PNG wherever one is
listed and loads successfully. A missing or failing image degrades silently
back to the procedural shape — nothing 404s loudly or breaks the board.

> This replaces the old JSON-manifest-gated pipeline (`sprites.json`,
> `sprites/manifest.json`, `scripts/genmanifest.mjs`) that the real-time engine
> used. That pipeline was removed with the rest of the real-time core on
> 2026-08-09 — the GO renderer never read it. If you're looking for those
> files, they're in git history.

## How it works

1. `src/go/SpriteCache.ts` is a tiny preloader: `preload([...srcs])` kicks off
   `Image()` loads for a list of URLs; `get(src)` returns the loaded
   `HTMLImageElement` once ready, or `null` while pending/failed.
2. `GoRenderer.ts` declares the sprite paths it wants in a plain `SPRITE`
   object near the top of the file, under Vite's `BASE_URL`:

   ```ts
   const SPRITE = {
     house1: `${_base}/assets/sprites/house1.png`,
     // ...
   };
   ```

3. Each draw call (`this.sprites.get(SPRITE.playerChar)`, etc.) checks whether
   the image is ready; if not, it falls back to drawing a geometric shape in
   that call site instead. There is no separate anchor/scale config file —
   placement math (drawn width/height, anchor offset) lives inline at each
   `drawImage` call in `GoRenderer.ts`.

## Adding or replacing a sprite

1. Put a PNG under `public/assets/sprites/` (transparent background,
   isometric, roughly matching the existing art's proportions).
2. Add its path to the `SPRITE` object in `GoRenderer.ts` (or point an
   existing entry at the new filename).
3. If it's a new sprite (not a replacement), add a `drawImage` call sized and
   anchored the way the existing ones are — see the house/tree/guard/player
   draw functions for the pattern (drawn size as a function of tile width,
   anchored near the sprite's visual base).
4. `pnpm --filter @workspace/operacia-kopanice run dev` — the renderer now
   draws that image.

See `.agents/memory/canvas-sprite-compositing.md` before adding a new PNG: use
normal `source-over` compositing (the default) for assets with a real alpha
channel — screen blending is not a substitute for background removal and will
leave a pale ghost around photographed props.

## Current sprite set

| File | Used for |
|---|---|
| `sprites/player.png` | The player character ("Odbojár" — the resister) |
| `sprites/guard.png` | Default officer/sentry world sprite |
| `sprites/guard-sniper.png` | Sniper variant world sprite |
| `sprites/guard-machinegunner.png` | Machine-gunner variant world sprite |
| `sprites/guard-officer.png` | Officer portrait in the enemy panel (HUD) |
| `sprites/guard-soldier.png` | Patrol portrait in the enemy panel (HUD) |
| `sprites/house1.png`, `sprites/house2.png` | Village houses (wall nodes + decorations) |
| `sprites/trees.png` | Tree cover decoration |

Branding (`public/brand/operacia-kopanice-logo.png` and derived
`public/icon.png`) is separate from gameplay sprites — see the root
`README.md` for how those are used in the menu and favicon/manifest.

## Tips

- Keep characters roughly the same height as the existing player/guard sprites
  so silhouettes read consistently at the game's small on-screen scale.
- Transparent PNGs read best; a tight crop makes the inline anchor math
  predictable without needing per-sprite config.
- Compress before committing — sprite PNGs are shipped to every player's
  browser on load. Favor dimensions close to the actual on-screen draw size
  over shipping a much larger source image and scaling it down at runtime.
