# Operacia Kopanice: Real-Time Stealth Prototype

Editor-hosted single-player UE 5.8.2 Windows prototype, now using continuous
NavMesh movement and active pause instead of turns. The winter settlement,
original supplied characters, cabins, forest, vehicle and animated river are retained.

## Start

Install Git LFS before cloning; run `git lfs pull`.
From this directory in PowerShell:

```powershell
./Start-Demo.ps1 -Build
```

Later launches: `./Start-Demo.ps1`. Override `-EngineRoot` if necessary.
Visual Studio C++ build tools and Windows SDK are required.
`-Width 1920 -Height 1080` selects Full HD.
Open the .uproject and use Play > Standalone Game as an alternative.
Entry is assembled at runtime by `OKRTGameMode`; it is not a baked editor level.
This is not a packaged standalone EXE or APK. The existing Canvas web game remains
a separate legacy GO prototype, not a browser build of this Unreal refactor.

## Mission

Lead both the Partisan and Officer through the winter settlement.
Collect the marked TNT, bring both across the only bridge, reach the eastern
detonator and interact to destroy the bridge. Extract both at the northeast exit.
Destroying the bridge early can strand a party member; restart is then necessary.
Any party death fails the mission.
The mission starts in tactical pause once navigation is ready, allowing an initial
plan before guards begin moving. Resume with Space or the pause icon.

Two guards patrol continuously. The near zone detects standing units immediately;
the far zone fills suspicion gradually. Crouching, prone stance and nearby cover
reduce detection. Walls block sight; running is audible over a larger distance.
Suspicious guards investigate noises and visible bodies; combat spreads an alarm
to nearby guards and deals real-time damage.

## Controls

| Action | Input |
| --- | --- |
| Select Partisan / Officer / both | 1 / 2 / 3; portraits or click a party member |
| Box selection / add to selection | Left-mouse drag / Shift + drag; Shift + portrait also adds |
| Move | Right-click terrain; left-click or single touch also works |
| Append a move | Shift + terrain click |
| Active pause / resume | Space or pause icon |
| Queue orders | Issue orders during active pause, then resume |
| Cancel selected units' orders / armed targeting | X or lower-right minus icon |
| Walk / run / crouch / prone | W / R / C / V or stance icons |
| Silent takedown | T or swords icon, then click guard; approach from behind |
| Distraction | F or circle icon, then click ground; 5 charges, 6 s cooldown |
| Interact: TNT / detonator / hide carried body | E or hand icon, within range |
| Carry body / drop carried body | B or backpack icon; click a dead guard |
| Toggle one guard's cone | Right-click that guard |
| Toggle all cones | Eye icon / Options |
| Rotate | Q / right bracket or rotate icons |
| Pan | Middle-mouse drag |
| Continuous orbit and tilt | Disable stepped camera in Options; Alt + right drag |
| Zoom / focus active unit | Mouse wheel / Home |
| Menu / restart | Escape / F5 |

Interaction happens at the unit's current position, not at the cursor.
Unarmed left-click world actions occur on release; moving at least 8 screen pixels starts
a selection rectangle instead. Drag in either direction. Empty rectangles keep
the existing selection and never issue movement. Units are selected by their
projected capsule centres; dead units, guards and centres underneath HUD panels
are excluded. Starting on HUD never starts a drag. Leaving the viewport, opening
the menu, right-clicking or pressing X cancels a pending drag.
Touch input is release-based: a tap shorter than 24 screen pixels activates the
same UI/world click as the mouse; a finger slide is ignored as a gesture and does
not move a unit. Two fingers pan the camera and change zoom by pinch. This keeps
the current Android interaction safe while leaving single-finger camera orbit
available for a later pass.
Camera zoom and pinch remain crisp during tactical pause; camera motion blur is
disabled so a previous zoom cannot leave a radial blur on screen.
The active member has a gold portrait bar, crosshair marker and gold world ring;
other selected members have green markers. Box selection keeps the current active
member when it remains selected. A single click/portrait sets a new active member.
Takedown and carry commands navigate to their target first.
Movement and stance apply to the selected group; abilities and interaction use
only the active portrait. Attack/throw commands wait for their cooldown instead
of being discarded. Invalid targets leave existing movement intact and keep
targeting armed; X cancels targeting. Carry can be queued after a takedown of the
same guard. Friendly units are never valid takedown targets.
Pausing stops physics, patrols, cooldowns and detection, while selection, targeting,
camera and queued commands remain usable. Queues are limited to 32 per unit.
Menu pauses the world separately and preserves an existing tactical pause.
Options cover cones, path preview, stepped camera and four quality presets;
these new real-time preferences are session-local.

When path preview is enabled, selected party members show their queued
Move/Takedown/Carry destinations as real NavMesh route segments. The active
member's route is gold and other selected members' routes are green; rings mark
the queued destinations. Stance, interact and distraction orders do not create
fake route segments.

## Verification

`./Start-Demo.ps1 -SmokeTest` runs a rendered native integration fixture.
It tests real NavMesh bank connectivity and nonwalkable water, imported meshes,
cone geometry and cabin occlusion, stance-scaled suspicion, continuous party
movement and patrols, real Space-key pause/resume, frozen physics and order queues,
rear takedown, carrying/hiding, hearing investigation, ammunition/cooldowns,
both units crossing the bridge, TNT, sabotage, dynamic bridge path invalidation,
extraction, zero turn-counter changes and a separate combat/loss scenario.
HUD bounds are checked against the actual viewport.
Command regressions also cover friendly/invalid targets, unchanged movement on
rejection, takedown-then-carry planning, stale-target path cancellation, cooldown
waiting, empty inventory, active-member skills, queue capacity and menu gating.
AI checks verify stable pursuit requests and last-seen search after losing sight.
Selection checks exercise the native pointer handlers with real projected party
coordinates, including reversed/additive drags, empty/dead/enemy rejection, HUD
exclusion, cancellation, small-motion clicks and pause-enabled input bindings.

The fixture temporarily disables guard brains for controlled ability/navigation
checks, then enables patrol/hearing/combat in separate phases. It does not prove
that every player strategy succeeds or replace a manual input playthrough.
Repeat with `-Width 480 -Height 800` for narrow layout.
Screenshots are in `Saved/Screenshots/WindowsEditor/OKRT_*.png`; logs are in Saved.
The launcher rejects missing PASS, check failures and material compile errors.

Legacy grid regression:
`./Start-Demo.ps1 -LegacyGridDemo -SmokeTest`.
Legacy play:
`./Start-Demo.ps1 -LegacyGridDemo`.
Old rules and controls are documented in `Docs/LEGACY_GRID_DEMO.md`.

## Scope

One real-time mission is playable. The former three grid layouts are preserved
only in the legacy mode; large 48 x 48 and 96 x 96 real-time missions are not authored.
No grid, cell moves, turn counter, undo-turn action or turn coordinator drives
the new mode. Approved physical building/vehicle envelopes are unchanged.

Characters are supplied static meshes, not rigged animated characters.
Crouch/prone/body poses are explicitly placeholder transforms; production locomotion,
takedown/carry animation, animation-driven combat and historical uniform review remain.
AI uses a native finite-state controller with Sight/Hearing perception, not a
Behavior Tree asset. Collision proxies currently cover the playable banks,
bridge, main cabin and cover crate; imported decorative meshes remain collision-free.
Interiors, advanced cover search, ballistics, weapon effects, production audio,
save/load, broader acoustic simulation and real-device Android testing remain.

The water remains an animated surface shader, not a fluid solver.
Nanite usage is persisted on all seven supplied base materials.
Asset regeneration, including Blender source work, is not required to play.
See `Docs/REALTIME_STEALTH.md` for architecture and extension points.
See `Docs/REALTIME_VERIFICATION.md` for tested configurations and remaining gaps.
