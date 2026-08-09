# Operácia Kopanice

Turn-based stealth puzzle set in a snowy Slovak mountain village during the SNP period. Each mission keeps the GO-style rhythm: make one careful move, let the guards react, and reach the exit unseen.

See [`docs/GO-DESIGN.md`](docs/GO-DESIGN.md) for the rules, level list, and module map, and [`docs/ASSETS.md`](docs/ASSETS.md) for the sprite pipeline.

## Running locally

From the workspace root:

```bash
pnpm --filter @workspace/operacia-kopanice run dev
```

The artifact-managed workflow supplies its required `PORT` and `BASE_PATH` values.

## Checks

```bash
pnpm --filter @workspace/operacia-kopanice run typecheck
pnpm --filter @workspace/operacia-kopanice run test
pnpm --filter @workspace/operacia-kopanice run build
```

## Village board

The active GO board uses declarative terrain and decorative objects. Snow, roads, planks, mud, houses, trees, rocks, crates, and fences are rendered separately from the deterministic movement and guard logic. Houses and trees are solid by default; individual decorations can opt into or out of movement and sight blocking with explicit collision flags.

## Visual shell

The game uses a shared dark tactical interface: brass framing, geometric linework, the supplied Operácia Kopanice logo, and consistent level, HUD, enemy, and control panels. The full supplied artwork lives in `public/brand/operacia-kopanice-logo.png`.

Enemy portrait cards are interactive during a mission: selecting a card briefly rings live guards of that type on the board, and each card shows the maximum sight range in cells for that type. The strip collapses into a compact horizontal mobile panel so it does not compete with the top HUD.

Undo stores a complete turn-boundary snapshot, including player position, guard routes and facing, gates, terminals, turn count, and outcome phase. Reset reloads only the active level, so saved completion and unlock progress remain unchanged. The HUD buttons are also available through `U`/`Z` and `R`; holding those keys does not repeat the recovery action.

Officers (the existing sentry slot) can now detect the player on the outermost cell of their sight beam and alert nearby infantry (the existing patrol slot) within two Manhattan cells. Alerted infantry reverses its deterministic patrol direction, shows an amber warning marker on the board, and marks its portrait card as on alert; the normal inner sight cells remain lethal.

Distractions are authored per level as walkable-cell objects. The generator is activated with `E` or by clicking it while standing on its cell, consumes exactly one turn, redirects every living guard within its configured Manhattan range to its configured direction for that response, and then becomes visibly spent. Its state is included in undo/reset snapshots.

Branding uses the supplied logo artwork at `public/brand/operacia-kopanice-logo.png`. The same artwork is composed into a square `public/icon.png` for the browser favicon, Apple touch icon, and installable application icon; `public/manifest.webmanifest` exposes it to supported installs.