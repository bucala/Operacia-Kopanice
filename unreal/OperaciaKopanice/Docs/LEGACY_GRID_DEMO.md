# Legacy Grid Mode

Launch with `./Start-Demo.ps1 -LegacyGridDemo` from this project's directory.

# Operacia Kopanice: Winter Demo

Editor-hosted, single-player tactical demo for Unreal Engine 5.8.2 on Windows.
The runtime game mode assembles the scene from committed assets. Blender/Python
are needed only to regenerate artwork, not to play.

## Download and Start

Install Git LFS before cloning, then run `git lfs pull` in the checkout.
Large Unreal, FBX and Blender assets are stored through LFS, not embedded in Git.

Run `./Start-Demo.ps1 -Build` in PowerShell from this directory after pulling
source changes. Later launches can use `./Start-Demo.ps1`.
The launcher builds on a fresh checkout and opens a 1280 x 720 game window.
Override `-EngineRoot` for your UE installation; `-Width 1920 -Height 1080`
selects Full HD. Visual Studio C++ build tools and the Windows SDK are required.
This is not a packaged standalone executable.

Alternatively open OperaciaKopanice.uproject and use Play > Standalone Game.
The default map is Engine/Maps/Entry; the scene is constructed by OKDemoGameMode.

## Missions

The menu provides three layouts of the same winter settlement:

| Mission | Crossing row | Objectives and challenge |
| --- | --- | --- |
| Zelezne hrdlo | 4 | Southern TNT store, northeast extraction |
| Severny brod | 3 | Southwest TNT, northern crossing, northeast extraction |
| Posledny transport | 5 | TNT behind the cabin, southern crossing, southeast extraction |

Collect TNT, cross the bridge, activate the detonator, and reach extraction.
Each mission changes the start, objectives, crossing and guard starting phase.
They share the environment artwork; they are not three distinct biomes.
The deterministic solver finds routes of 22, 24 and 27 turns respectively.

Red overlays show current sight. Detection is checked both before and after
enemy reaction. Cover and the river stop sight. A valid action advances one turn;
invalid movement or interaction never consumes a turn.

## Controls and Menus

- WASD / arrows, direction icon buttons or click an adjacent cell: move.
- E / hand icon: interact on an objective; Space / clock icon: wait.
- Z / undo icon: undo; R / menu Restart: restart the current mission.
- Escape / menu icon: pause menu, resume, missions, Options and quit.
- G: toggle grid lines. Threat and objective fills stay visible.
- Q / C: rotate 45 degrees, or hold for continuous rotation when stepping is off.
- Right mouse drag: rotate horizontally and tilt vertically.
- Page Up / Page Down: tilt; mouse wheel: zoom.
- Middle mouse drag: pan; I / J / K / L: pan up / left / down / right relative to the view.
- Home / crosshair button / partisan portrait: center on the player without changing zoom or angle.
- End: restore the board overview, including zoom and angle.
- Bottom camera buttons: rotate, tilt and zoom; Options can reset the camera.

Options include grid visibility, camera enable, stepped/continuous rotation and
four rendering quality presets. Preferences persist in the local Saved directory.
Menus block tactical actions without advancing guards. Camera controls are
disabled while a menu is open. After a victory, DALSIA MISIA selects the next
mission; the last mission returns to mission selection.

Panning is bounded to the current tactical board. Pan, focus and orbit never
advance turns or add undo snapshots. Focus is a one-time operation, not automatic
tracking. Options' explicit camera reset remains available with movement disabled.
Touch users can use the focus, rotate, tilt and zoom buttons; native touch drag
panning and a real-device Android check are not implemented in this step.

## Scale and Artwork

One cell is 180 cm. The main cabin blocks exactly 4 x 4 cells (720 x 720 cm).
All five cabin envelopes fit 720 cm using uniform scaling; the original display base is sunk
into the terrain. Roads exclude the main cabin footprint.
The parked car is fitted within a 3 x 2 cell envelope (540 x 360 cm) with its
original proportions preserved, not stretched to an implausible vehicle width.
It is decorative scenery outside the tactical grid, not a drivable unit.

Two conifer meshes, a shrub and a boulder were extracted from the supplied
Meshy_AI_Winter_Forest_Asset_C_0915163054_texture.blend with their shared PBR atlas.
These are not European beech trees. Nanite is enabled for supplied detailed meshes.
Ground and cobbles use conventional meshes to preserve shallow detail.
Water uses time-driven ripples, moving highlights and animated normals below the
carved banks. This is a visual surface simulation, not a fluid simulation.

## Verification

`./Start-Demo.ps1 -SmokeTest` renders and replays all three missions, checks
bridge destruction/undo/restart, turn synchronization, menu action blocking,
grid/threat independence, camera bounds and eight 45-degree picking angles,
plus building/vehicle dimensions. It also verifies all three units use the supplied
character mesh, original material and 180 cm height, without primitive body parts.
Pan/focus checks cover map limits, eight view angles and unchanged turn history.
HUD checks cover five desktop/narrow layout sizes, panel separation and actual
direction/undo/menu/grid button dispatch. They also check clicks on uncovered
top/bottom pixels are not swallowed by invisible UI strips.
Repeat at Full HD with the width/height flags.
It writes DemoStart.png, DemoWin.png, DemoLevel2.png, DemoLevel3.png and
DemoOptions.png to Saved, then exits. The launcher rejects failed checks or
material compilation errors. It does not replace a manual mouse/keyboard playthrough.

UE Automation tests under OperaciaKopanice.Demo cover all mission solutions and
action contracts. Tests/Standalone compiles the actual rule implementation with
a small value-type adapter, without linking UE.

See `Docs/ASSET_STORAGE.md` for the lightweight repository layout and the
location of the preserved Blender/FBX preparation sources. Original external
asset files remain unchanged in the archive.

## Remaining Production Work

This is a playable visual iteration, not visual parity with the reference image.
The player uses the supplied partisan; both guards use the separate supplied
officer figurine, with green/red faction discs. Characters are 180 cm high.
No primitive guard bodies are used when the supplied assets are present.
Historical clothing/markings review,
character rigging/animation, production audio,
deformable snow, drivable vehicles, historical accuracy review, full manual
input testing and packaged distribution remain. The demo uses deterministic grid
rules, not the separate PCG/AI Perception authoring systems for its enemy logic.

The 2026-09-26 reference art pass adds two Winterwood cabin instances, a command
caravan outside the board, wider cobbled roads, wooden paths and supply crates.
See Docs/REFERENCE_VISUAL_PLAN.md for the image analysis and remaining art passes.
The 2026-09-27 HUD pass adds supplied role portraits, corner objectives and
icon movement/camera controls. See Docs/REFERENCE_HUD.md for sources and licences.
Build products, caches, logs and local credentials are deliberately excluded from Git.
