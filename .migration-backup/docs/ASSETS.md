# Art assets — adding the high-fidelity images

The game renders **procedurally by default** (shaded isometric shapes), and
upgrades to high-fidelity images automatically when the matching PNG is present
and listed in the manifest. Nothing breaks if a file is missing — it just keeps
the procedural look. This lets you drop in real art incrementally.

## How it works

1. Put a PNG under `public/assets/sprites/` (transparent background, isometric).
2. Add its path to the manifest, or just run:
   ```bash
   node scripts/genmanifest.mjs
   ```
   which scans the folder and rewrites `public/assets/sprites/manifest.json`.
3. `npm run dev` — the renderer now draws that image. Tune placement
   (`anchorX/anchorY`, `scale`) in `public/assets/sprites.json`.

`anchorX/anchorY` are 0..1 pivots: the point of the image that sits on the tile
(roughly the feet for a character, the base centre for a building). `scale` is
the drawn width measured in tile-widths (one tile = 64 px), aspect ratio
preserved. `rect: [x, y, w, h]` optionally slices one sprite out of a sheet.

## Expected files (map of the supplied assets → filenames)

| Supplied asset | Save as | Used for | Notes |
|---|---|---|---|
| Slovak partisan / sapper character | `sprites/agent.png` | The player | Crop tight; feet at the bottom. |
| (recoloured / uniform variant) | `sprites/agent_disguised.png` | Disguised player | Optional — falls back to a red-tinted procedural figure. |
| (enemy guard render) | `sprites/guard.png` | Enemy guards | Optional — falls back to the procedural red figure. |
| Snow-covered log cottage | `sprites/cottage.png` | The objective building | Placed over tile `(17,15)`; tune `scale`/anchor to cover the footprint. |
| Open TNT/dynamite crate | `sprites/tnt_crate.png` | The goal cache | Placed at `(17,16)`. |
| Winter trees/rocks/bushes sheet | `sprites/decor/conifer.png`, `sprites/decor/boulder.png`, `sprites/decor/haystack.png` | Tile decorations (`tree`/`rock`/`hay`) | Crop the individual props out of the sheet into one PNG each, **or** point a single `src` at the sheet and add `rect` slices in `sprites.json`. |

Prop placement lives in the map (`scripts/genmap.mjs` → `props`), and decoration
mapping is by tile `decor` field → prop id `decor_<decor>` (see
`sprites.json` and `RenderSystem.drawTile`).

## Tips

- Keep characters ~1.4–1.6 tile-widths tall (`scale`), buildings ~4.
- Transparent PNGs read best; a tight crop makes anchoring predictable.
- After adding/removing files, re-run `genmanifest.mjs` (CI does this too).
