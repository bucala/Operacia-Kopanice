# Changelog

## Unreleased

### Changed

- Rebuilt the menu and in-mission interface around the dark tactical, brass-framed Operácia Kopanice visual system.
- Added a reusable geometric game logo and a matching browser icon.
- Connected enemy portrait cards to temporary map highlights for live guards and added each type's maximum sight range to the card.
- Reworked the enemy panel as an interactive, brass-accented responsive strip that remains usable on narrow screens.
- Hardened the mission recovery controls: turn-boundary snapshots restore player, guards, gates, and phase; Reset starts the current mission cleanly without touching saved progress.
- Added accessible labels and keyboard-repeat protection for Undo, Reset, Menu, and recovery shortcuts.
- Replaced the generated menu mark with the supplied Operácia Kopanice logo and derived a matching square PNG application icon.
- Reworked the Operácia Kopanice GO board toward a snowy village composition with larger isometric cells and compact single-cell character sprites.
- Added visual terrain distinctions for snow, roads, planks, and mud.
- Added declarative village decorations for houses, trees, crates, and fences across the existing missions.
- Added tree and rock cover as movement and sight blockers while preserving the existing deterministic guard and undo rules.
- Added explicit collision flags for village decorations, with solid houses and trees defaulting to movement and sight blockers.
- Switched house and village decoration sprites to standard alpha compositing so transparent pixels remain transparent.
- Repositioned the desktop HUD around the active board and introduced a compact mobile arrangement.