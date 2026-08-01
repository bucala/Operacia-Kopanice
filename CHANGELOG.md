# Changelog

## Unreleased

### Changed

- Reworked the Operácia Kopanice GO board toward a snowy village composition with larger isometric cells and compact single-cell character sprites.
- Added visual terrain distinctions for snow, roads, planks, and mud.
- Added declarative village decorations for houses, trees, crates, and fences across the existing missions.
- Added tree and rock cover as movement and sight blockers while preserving the existing deterministic guard and undo rules.
- Added explicit collision flags for village decorations, with solid houses and trees defaulting to movement and sight blockers.
- Switched house and village decoration sprites to standard alpha compositing so transparent pixels remain transparent.
- Repositioned the desktop HUD around the active board and introduced a compact mobile arrangement.