# Reference HUD: Native Editor Demo

The 2026-09-27 pass replaces the full-width top bar and bottom text toolbar with
corner portraits, a compact mission objective, an activity panel and icon controls.
It does not change tactical rules or provide an Android package or 3D web port.

## Supplied Art

The runtime portrait textures are stored in `Content/Kopanice/ReferenceHUD`.
Their original source images are preserved in the external asset archive. They
are unchanged copies of the user's `png/guard-officer.png` and `png/player.png`.
The HUD samples a head-and-shoulder UV rectangle at draw time. Original source
files remain untouched. The playable role is labelled PARTIZAN, not an invented
selectable infantry unit. Clicking that portrait focuses the camera on the player.
The officer portrait identifies enemy artwork; it is not a second playable unit.

## Controls And Layout

- Four direction arrows move along the existing cardinal grid axes.
- Hand interacts, clock waits and undo reverses a turn.
- Camera controls rotate, tilt, focus and zoom. Grid toggles visibility only.
- Menu retains resume, mission selection, restart, options and quit.
- Hover tooltips name icons. Targets are 44 x 44 pixels with 8 pixel spacing.
- Shorter/narrower windows use smaller or side-by-side portrait frames. Objective
  and message text wraps within measured bounds. Font size is not viewport-scaled.
- Only visible panels/buttons consume gameplay clicks; the former invisible
  full-width top and bottom exclusion strips have been removed.
- The camera explicitly maintains horizontal field of view, avoiding clipped
  board edges in portrait viewports. The launcher accepts dimensions from 480 px.
- Native Sky Atmosphere supplies the sky exposed by tall viewports. Only the
  primary directional light drives it; the existing fill light remains separate.
- A distant snow plane below the carved terrain fills views beyond the authored
  terrain edge. It is decorative, has no collision/navigation role, and does not
  enlarge the 11 x 9 playable grid or add a new river section.

Native HUD layout tests cover 1920 x 1080, 1280 x 720, 960 x 540, 640 x 480 and
480 x 800. These geometry checks are not physical Android device tests.

## Reproduction And Licences

Sixteen Lucide icons are rasterized from upstream SVGs, not hand-drawn substitutes.
The exact upstream commit is stored in `lucide-source.json`; the original vectors
and complete upstream licence are included alongside the generated textures.
See [Lucide licence](https://github.com/lucide-icons/lucide/blob/main/LICENSE).

Run the archived `prepare_reference_hud.cjs` with Node, passing the installed
Sharp module path as argument 1 and the saved upstream SHA as argument 2. Then run
`Content/Python/import_reference_hud.py` through Unreal's Python commandlet.
Import creates `/Game/Kopanice/ReferenceHUD` textures with UI compression,
sRGB enabled, UI LOD group and no mipmaps. Portrait originals must already be
copied into the generated directory when rebuilding from source.

The HUD remains an Unreal Canvas implementation consistent with the demo. A later
production UI can migrate it to UMG without altering deterministic gameplay.

## Verification: 2026-09-27

- Win64 editor build succeeded on UE 5.8.2; texture import reported no errors.
- Final rendered smoke runs passed at 1920 x 1080 and 480 x 800. An earlier
  HUD revision also passed at 960 x 540.
- All three missions completed in 22, 24 and 27 turns, with destruction/undo,
  camera bounds/focus and eight-angle screen-to-grid checks passing.
- HUD geometry passed for five sizes; actual direction, undo, grid and menu
  dispatch passed, together with free clicks outside visible UI.
- The snow backdrop stayed at Z=-200 with collision disabled. Scenery uses an
  explicit NoCollision profile so mesh scaling cannot restore default collision.
- Actual screenshots are `ReferenceHUD-2026-09-27.png` and
  `ReferenceHUD-Portrait-2026-09-27.png` alongside this document.

These checks run in the Windows Unreal editor. Touch event support is not a
physical Android test. Packaged Windows/Android releases and a 3D browser port
remain separate work, as do production snow/roof modelling and animation.
