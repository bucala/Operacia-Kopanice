# Changelog

## Unreleased

### Unreal Demo

- Make the new missions' forest functional: instanced firs have narrow physical
  trunk/nav proxies, rocks block movement and sight, and shrubs provide
  height-bounded crouch/prone concealment without blocking paths.
- Preserve patrol, objective and service clearings; show a live KRYT portrait badge.
  Standing immediately removes concealment; new maps no longer inherit Mission 1's
  virtual cover point. Add native forest collision, sight and cover regressions,
  plus a real queued approach into shrub cover during each campaign playthrough.
- Add two authored 86.4 m real-time missions: retrieve the forest courier documents
  and sabotage a command post, each with three live patrols and two-member extraction.
- Integrate budgeted Nanite Winterwood cabin and Olive Command Caravan derivatives
  from supplied Blender files; reuse instanced forest, rocks, cars, paths and cover.
- Replace the small pause panel with main/pause, mission selection, briefing,
  results/next-operation, graphics/camera/interface settings and restart/quit confirmations.
- Persist completed missions, objective-marker visibility and camera sensitivity;
  retain active pause and queued commands when leaving menu. Add objective camera focus.
- Wait for complete navigation construction before planning pause on larger maps;
  block early pause/menu input from freezing construction. Add rendered campaign/menu
  fixtures including real level travel with live patrols.
- Keep forest instances on visible actors and save Nanite/instancing material usage
  flags; fail rendered smoke runs when the engine substitutes the default material.
- Validate complete agent-specific navigation routes before accepting movement,
  guard/body approaches or contextual interactions. Reject inaccessible water,
  interiors, elevated targets and disconnected banks without cancelling prior orders.
- Check appended routes from the last queued destination and recheck navigation
  once when execution starts; retain later orders if a target becomes unreachable.
- Require vertical reach and line of sight for body pickup as well as takedown,
  preventing attachments through walls; add rendered navigation/reach regressions.
- Make desktop controls Commandos-like stealth RTT: left-click/drag selects units,
  right-click issues contextual movement/takedown/body orders and approaches TNT,
  detonator or body cover before interacting. Retain separate touch-tap behavior.
- Add double-right-click running with paused/Shift queue promotion, Ctrl+A living
  party selection, Tab specialist cycling, S stop and Escape/right-click target cancel.
  Preserve crouch/prone during ordinary movement and isolate skills to the active unit.
- Add arrow-key camera pan and suppress stale pointer deltas on camera drag start;
  provide contextual cursor/path previews and rendered RTT input regression checks.
- Block bridge sabotage until both living party members have completely cleared
  the span onto the east bank. Rejected detonations preserve the bridge, NavMesh
  route and noise state; the objective now reports the party's crossing progress.
- Replan Mission 1 patrols with authored watch directions and dwell times. The west
  guard faces away from the party for 12 seconds, watches the bridge for 2 seconds,
  then turns north before returning. The east guard patrols the southern bank.
- Add a live-AI mission fixture covering a 40-second safe start, TNT collection,
  timed bridge crossing, sabotage and extraction without disabling guards.
- Add lightweight 14-bone derivatives of the supplied Partisan and Officer models
  for all party members and guards, normalized to 180 cm with shared textures.
- Drive limb animation from real-time movement and smooth stance changes. Apply
  pelvis offsets in component centimetres to prevent FBX scale/axis amplification;
  cap cadence and test lateral stability and vertical bob over a complete cycle.
- Blend snow into road edges, add shallow-water shoreline colour and wood snow dust.
- Make menu labels clickable, add mission restart, hover feedback, inventory/order
  icons, real-time status and armed-ability feedback.

- Persist real-time view-cone, path-preview, stepped-camera and graphics-quality
  preferences between launches in local GameUserSettings.ini.
- Display named graphics presets instead of numeric quality levels; turning off
  path preview immediately clears the hover route.
- Keep integration smoke fixtures isolated from player preference files.

### Added

- Added mouse-wheel/trackpad-pinch zoom for the game board, plus discoverable −/+ buttons in the top bar; zoom is clamped to a min/max range around each level's fit-to-screen view and survives window resize.
- Restored CI (`.github/workflows/ci.yml`): typecheck, unit tests, and a production build now run on every push/PR for `@workspace/operacia-kopanice`. There was no CI at all after the pnpm workspace migration.
- Restored `test/progress.test.ts` (level unlocking, best-turn persistence), which was dropped during the same migration even though `progress.ts` stayed in production.
- Restored and rewrote `docs/GO-DESIGN.md` and `docs/ASSETS.md` for the current game: 8 levels, village decorations, officer alerts, and the generator/stone/bell distraction mechanics (the old copies only described the original 3-level version and a JSON-manifest sprite pipeline that no longer exists).
- Documented the workspace layout and the real-time engine's removal in `replit.md` ("Where things live", "Architecture decisions", "Gotchas" — previously placeholders).

### Removed

- Removed the original real-time isometric stealth engine (`src/systems`, `src/game`, `src/ai`, `src/skills`, `src/integrations`, `src/map`, the ECS `src/components` and `src/core/ecs`) — ~9,000 lines, unreachable from the GO game's entry point since the July turn-based transformation and never wired back in. Still available in git history if a real-time mode is revisited.
- Removed the unused shadcn/ui component library, hooks, and scaffold page (55 files) and ~30 unused npm dependencies (Radix, Tailwind, React Query, react-hook-form, wouter, and others) — none were imported by the shipped game; only `react`/`react-dom` (mounting the vanilla-DOM `GoApp`) were ever used.
- Removed the sprite/tile/audio/map JSON manifests under `public/assets/` that only the removed engine read; the GO renderer has always loaded its sprite PNGs directly.
- Removed the menu subtitle ("Ťahová taktická hádanka · v štýle Lara Croft GO") under the logo.

### Changed

- Removed the main menu logo's baked-in dark background (luminance-keyed to transparent) so it sits directly on the menu panel instead of inside its own boxed rectangle; dropped the now-redundant `.brand` background/border and gave the mark an alpha-aware drop-shadow instead of a box-shadow. Also cropped and downscaled the source PNG (4.1 MB → 328 KB) to its actual display size.
- Reworked red text in the HUD: locked level names in the menu no longer clash with the gold "zamknuté" badge on the same row (now share the same muted tone); the win/lose outcome icons gained a soft colour-matched glow instead of sitting flat.
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
