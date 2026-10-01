# Confirmed Object Scale and Platform Targets

Approved by the user on 2026-09-22. One grid cell is 180 cm.
Dimensions describe an occupancy envelope, not a command to stretch a mesh.
Keep scale uniform, include protruding geometry and snap the occupancy origin
to the grid. Rotate footprints with the model; test swept space for moving units.

| Object | Cells | Envelope (m) |
| --- | --- | --- |
| Log cabin | 4 x 4 | 7.2 x 7.2 |
| Larger house | 6 x 5 | 10.8 x 9 |
| Barn | 8 x 5 | 14.4 x 9 |
| Passenger car | 3 x 2 | 5.4 x 3.6 |
| Truck | 5 x 2 | 9 x 3.6 |
| Medium tank | 5 x 3 | 9 x 5.4 |
| Heavy tank | 6 x 3 | 10.8 x 5.4 |
| Fighter aircraft | 7 x 6 | 12.6 x 10.8 |
| Transport aircraft | 18 x 11 | 32.4 x 19.8 |

Future mission budgets: 48 x 48 cells (86.4 m square) and 96 x 96
(172.8 m square). These are approved authoring targets, not yet authored
large missions. The three current UE tutorial missions remain 11 x 9.
Exact historical models must be checked individually against their dimensions.

## Shared Placement Rules

`Source/OperaciaKopanice/Public/Demo/OKObjectLayout.h` is the runtime footprint
catalog for all nine approved object categories. The current cabin collision
cells and imported cabin/car mesh scales use this catalog.

- `FPlacement::Origin` is the minimum occupied cell after rotation, not an asset
  pivot. `QuarterTurns` accepts signed multiples of 90 degrees. Camera rotation
  at 45 degrees does not rotate logical object footprints.
- `Contains` uses exclusive upper bounds; adjacent rectangles may touch without
  overlapping. `Fits` rejects invalid dimensions and placement outside the map.
- `CanPlace` checks map bounds, existing object footprints and reserved cells.
  Future mission authors must reserve river cells, spawn/objective cells and
  route corridors. It does not prove route connectivity or swept vehicle motion.
- `UniformScale` preserves mesh proportions inside the local unrotated envelope.
  Empty or non-finite mesh bounds return zero, never an infinite actor scale.
- Map budgets of 48 and 96 cells are tested, but the current scene still uses
  its original 11 x 9 board. Background scenery remains outside that board.

Validation: `OperaciaKopanice.Demo.ObjectLayout` passed in UE 5.8 alongside
`ActionContracts` and `AllMissionsSolvable`. The standalone `layout_tests.cpp`
also covers every footprint at both map sizes, negative rotations, overlaps,
reserved cells, invalid bounds and exact occupied-cell counts. Existing mission
solutions remain 22, 24 and 27 turns.

## River

The user approved an original implementation without buying external assets.
The linked 80.lv article describes a paid River Generator with a free executable
demo, not freely redistributable source content. No third-party river assets,
hotfix binaries or marketplace configuration have been downloaded or committed.

The authored river channel and curved water strip now span the full 120 m
terrain, from Y=-5100 to Y=6900 cm. Exterior hills no longer fill the channel.
Generation asserts every centerline sample lies below the water. Flow is a
time-driven material, not physical fluid simulation. Shore rocks avoid all three
bridge locations; small rocks remain in the blocked river column.

## Platforms

| Target | Current path | Remaining release gate |
| --- | --- | --- |
| Windows UE | Native editor-hosted 3D demo, mouse/keyboard | Packaged executable, manual input and performance testing |
| Web / Windows browser | Existing 2D GO game in artifacts/operacia-kopanice | Broader browser compatibility and performance testing |
| Android browser | Same GO game, pointer taps and two-finger zoom | Real-device testing |
| Android native UE | Touch input routed to HUD/cell commands | Android SDK/NDK installation, mobile assets/rendering profile, APK packaging, real-device testing |

The current UE platform validation reports Android SDK invalid (r27c expected).
Do not describe this commit as a tested native Android release. Nanite/Lumen
desktop artwork is not automatically a mobile performance profile.
The web game and native UE game currently have separate renderers and mission
rules; the new 3D scene has not been ported to a browser build.

## Verification (2026-09-23)

- UE Win64 Editor build and rendered smoke test passed. The smoke exercises
  all three mission solutions, undo/restart, camera projection and object sizes.
- Web: 59 tests passed, TypeScript validation and production build passed.
- Chrome browser checked at 1280 x 720, 390 x 844 and 844 x 390. Screenshots
  show the game and controls; portrait canvas pixels are nonblank and the page
  has no horizontal overflow. Mission start, wait and undo were exercised;
  no page errors were reported.
- Tap cancellation, pinch zoom and listener cleanup have automated unit coverage.
  Viewport emulation does not constitute a real Android touch-device test.
- Visual quality remains a prototype: imported model bases, repeated buildings
  and character animation still need further art work to match the reference.
