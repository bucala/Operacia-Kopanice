# Changelog

## Unreleased

### Added

- Restored CI (`.github/workflows/ci.yml`): typecheck, unit tests, and a production build now run on every push/PR for `@workspace/operacia-kopanice`. There was no CI at all after the pnpm workspace migration.
- Restored `test/progress.test.ts` (level unlocking, best-turn persistence), which was dropped during the same migration even though `progress.ts` stayed in production.
- Restored and rewrote `docs/GO-DESIGN.md` and `docs/ASSETS.md` for the current game: 8 levels, village decorations, officer alerts, and the generator/stone/bell distraction mechanics (the old copies only described the original 3-level version and a JSON-manifest sprite pipeline that no longer exists).
- Documented the workspace layout and the real-time engine's removal in `replit.md` ("Where things live", "Architecture decisions", "Gotchas" — previously placeholders).

### Removed

- Removed the original real-time isometric stealth engine (`src/systems`, `src/game`, `src/ai`, `src/skills`, `src/integrations`, `src/map`, the ECS `src/components` and `src/core/ecs`) — ~9,000 lines, unreachable from the GO game's entry point since the July turn-based transformation and never wired back in. Still available in git history if a real-time mode is revisited.
- Removed the unused shadcn/ui component library, hooks, and scaffold page (55 files) and ~30 unused npm dependencies (Radix, Tailwind, React Query, react-hook-form, wouter, and others) — none were imported by the shipped game; only `react`/`react-dom` (mounting the vanilla-DOM `GoApp`) were ever used.
- Removed the sprite/tile/audio/map JSON manifests under `public/assets/` that only the removed engine read; the GO renderer has always loaded its sprite PNGs directly.

### Changed

- Rebuilt the menu and in-mission interface around the dark tactical, brass-framed Operácia Kopanice visual system.
- Added a reusable geometric game logo and a matching browser icon.
- Connected enemy portrait cards to temporary map highlights for live guards and added each type's maximum sight range to the card.
- Reworked the enemy panel as an interactive, brass-accented responsive strip that remains usable on narrow screens.
- Hardened the mission recovery controls: turn-boundary snapshots restore player, guards, gates, and phase; Reset starts the current mission cleanly without touching saved progress.
- Added accessible labels and keyboard-repeat protection for Undo, Reset, Menu, and recovery shortcuts.
- Added deterministic officer-to-infantry alerts: reaching an officer's sight edge reverses nearby patrol routes and surfaces an amber warning on the board and enemy panel.
- Added one-use generator distractions: `E` or standing-cell click consumes one turn and deterministically redirects nearby guards, with clear available/spent board states.
- Replaced the generated menu mark with the supplied Operácia Kopanice logo and derived a matching square PNG application icon.
- Reworked the Operácia Kopanice GO board toward a snowy village composition with larger isometric cells and compact single-cell character sprites.
- Added visual terrain distinctions for snow, roads, planks, and mud.
- Added declarative village decorations for houses, trees, crates, and fences across the existing missions.
- Added tree and rock cover as movement and sight blockers while preserving the existing deterministic guard and undo rules.
- Added explicit collision flags for village decorations, with solid houses and trees defaulting to movement and sight blockers.
- Switched house and village decoration sprites to standard alpha compositing so transparent pixels remain transparent.
- Repositioned the desktop HUD around the active board and introduced a compact mobile arrangement.