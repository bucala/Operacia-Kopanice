# Operacia Kopanice: Stealth Real-Time Tactics (RTT)

Editor-hosted single-player UE 5.8.2 Windows prototype, now using continuous
NavMesh movement and active pause instead of turns, with Commandos-like unit
selection and contextual orders. This is stealth RTT, not a point-and-click adventure.
The winter settlement,
original supplied characters, cabins, forest, vehicle and animated river are retained.

## Start

Install Git LFS before cloning; run `git lfs pull`.
From this directory in PowerShell:

```powershell
./Start-Demo.ps1 -Build
```

Later launches: `./Start-Demo.ps1`. Override `-EngineRoot` if necessary.
Normal startup opens the main menu. Select an operation, read the briefing and
deploy; each mission runs immediately after its NavMesh is ready. Space is an
optional tactical pause, never a next-turn command.
Visual Studio C++ build tools and Windows SDK are required.
`-Width 1920 -Height 1080` selects Full HD.
Open the .uproject and use Play > Standalone Game as an alternative.
Entry is assembled at runtime by `OKRTGameMode`; it is not a baked editor level.
This is not a packaged standalone EXE or APK. The Canvas web adaptation now also
uses a real-time loop, but is not a browser build of this Unreal project.

## Missions

Three native missions are available from the menu. Mission 2 retrieves documents
from a forest camp; Mission 3 acquires TNT and disables a command post. Both new
areas are 86.4 x 86.4 m, use three active patrols and require the living team to
extract together. See [campaign details](Docs/CAMPAIGN.md) for layouts and assets.
In these forest missions, trunks and rocks block movement and sight. Shrubs stay
traversable and conceal crouched/prone units beneath their height; KRYT appears
on the portrait while the cover bonus is active. Standing or leaving the shrub
removes it. Concealment slows detection but does not grant invisibility.
The following describes Mission 1, Zimny viadukt.

Lead both the Partisan and Officer through the winter settlement.
Collect the marked TNT, bring both across the only bridge, reach the eastern
detonator and interact to destroy the bridge. Extract both at the northeast exit.
The detonator remains locked until both living members have fully cleared the
bridge onto the east bank. An early interaction leaves the bridge intact and
shows a message; retry after the second member crosses. The objective reports
crossing progress and changes to detonator activation when the team is safe.
Any party death fails the mission.
The mission starts running once navigation is ready. Press Space or the pause icon
only when you want to freeze the world and queue orders; press again to resume.

The west guard waits north-facing for 12 seconds, approaches the bridge, watches
east for 2 seconds, then turns north and returns. The southern east-bank guard
also alternates patrol and short observation stops. Approach the west bank in a
crouch, wait for the west guard to return and face north, then cross together.
Sight and hearing still react to exposed or noisy movement, including during waits.

Two guards patrol continuously. The near zone detects standing units immediately;
the far zone fills suspicion gradually. Crouching, prone stance and nearby cover
reduce detection. Walls block sight; running is audible over a larger distance.
Suspicious guards investigate noises and visible bodies; combat spreads an alarm
to nearby guards and deals real-time damage.

## Controls

| Action | Input |
| --- | --- |
| Select Partisan / Officer / both | 1 / 2 / 3 or Ctrl+A; portraits or left-click a party member |
| Cycle active specialist | Tab; keeps a multi-unit selection intact |
| Box selection / add to selection | Left-mouse drag / Shift + drag; Shift + portrait also adds |
| Move in current stance | Right-click terrain; left-click terrain never moves |
| Run to destination | Right double-click terrain; changes pace only when order executes |
| Append a move | Shift + right-click terrain |
| Active pause / resume | Space or pause icon |
| Queue orders | Issue orders during active pause, then resume |
| Stop selected units / clear orders | S / X or lower-right minus icon |
| Undo last waiting order of active specialist | Backspace or order-tray undo icon, during tactical pause only |
| Cancel armed targeting without stopping movement | Right-click world or Escape |
| Walk / run / crouch / prone | W / R / C / V or stance icons |
| Silent takedown | Right-click living guard, or T then left-click guard; approach from behind |
| Distraction | F or circle icon, then click ground; 5 charges, 6 s cooldown |
| Approach TNT / detonator / hide carried body | Right-click near its world marker; active member approaches and interacts |
| Interact immediately at current position | E or hand icon, within range |
| Carry body / drop carried body | Right-click dead guard; B or backpack icon also targets pickup/drops a carried body |
| Toggle one guard's cone | Left-click that guard; no attack is issued |
| Toggle all cones | Eye icon / Options |
| Rotate | Q / right bracket or rotate icons |
| Pan | Arrow keys or middle-mouse drag; also available during tactical pause |
| Continuous orbit and tilt | Disable stepped camera in Options; Alt + right drag |
| Zoom / focus active unit | Mouse wheel / Home |
| Menu / restart confirmation | Escape / F5 |
| Focus current objective | Crosshair in objective panel |

Numbered party markers remain visible over scenery and select allies on mouse
release or touch tap. Shift adds an ally; right-clicking a marker never moves or
attacks. Armed abilities cannot target the marker, but right-click still cancels
targeting. Gold identifies the active selected member, green another selected
member and grey an unselected ally. Nearby markers separate, avoid HUD panels,
character bodies and mission interaction targets,
and disappear for offscreen/dead/hidden units. Portraits and hotkeys remain
available when no unobstructed screen position fits. Options > Rozhranie > Znacky
timu toggles both rendering and hit targets and is saved with other preferences.
These markers are an ally-selection aid, not an enemy-visibility or AI-cover rule.

World orders briefly show a green acknowledgement for acceptance, red for
rejection or amber for partial group acceptance, with a count such as 1/2.
The bottom message names the member whose command failed and briefly takes
priority over an armed targeting prompt without cancelling targeting. Rejection
leaves that member's existing plan intact. Indicators fade even during tactical pause, never
intercept clicks and are omitted where they would overlap HUD controls. They are
not noise circles and do not alert guards. Route preview uses the same navigation
checks as submission, from the queued endpoint during pause/Shift and including
group formation offset; it no longer draws bank-projected routes into water.

The order tray shows the active specialist's plan, five numbered actions per page,
up to the existing 32-order limit. Arrows page through the plan; hover or tap an
action to inspect its type. The started movement/approach is highlighted and
remains in the plan when undo removes a later waiting action. Undo is disabled
outside tactical pause or when only a started order remains. It never changes
another specialist's plan, spends ammunition or rewinds an executed action.
S/X still stops all selected units explicitly. Empty queues have no tray, and
short viewports suppress the tray if it would overlap existing HUD controls.

Right-click world orders use a visibility raycast and complete NavMesh paths.
An interaction order stores its approach destination, then executes within range;
E and the hand icon retain immediate interaction at the active member's feet.
Double-click promotes the last matching movement order instead of duplicating it
in a paused or Shift-appended queue. Ordinary moves preserve the current stance;
use W/C/V to leave running after a double-click. Invalid orders preserve existing paths.
Interactions remember the chosen marker, so a hide order cannot collect nearby TNT.
Detonator and hide orders may be planned before TNT/pickup; prerequisites are checked
on arrival. A failed prerequisite ends that interaction with feedback, not a retry loop.
An unreachable terrain/approach order is rejected before changing existing orders.
Water, blocked interiors and disconnected banks after sabotage cannot cancel a
valid route. Appended routes are checked from the last queued destination and
checked again on execution; a stale approach ends without clearing later orders.
Body pickup and takedown also require unobstructed reach at arrival.
Unarmed left-click selection occurs on release; moving at least 8 screen pixels starts
a selection rectangle instead. Drag in either direction. Empty rectangles keep
the existing selection and never issue movement. Units are selected by their
projected capsule centres; dead units, guards and centres underneath HUD panels
are excluded. Starting on HUD never starts a drag. Leaving the viewport, opening
the menu, right-clicking or pressing X cancels a pending drag.
Touch input is release-based: a tap shorter than 24 screen pixels activates the
UI, party selection or enemy inspection, and terrain taps issue contextual commands.
It does not inherit the desktop left-click-only selection rule. A finger slide is ignored as a gesture and does
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
targeting armed; right-click or Escape cancels only targeting, while S/X also clears
selected command queues. Carry can be queued after a takedown of the
same guard. Friendly units are never valid takedown targets.
Pausing stops physics, patrols, cooldowns and detection, while selection, targeting,
camera and queued commands remain usable. Queues are limited to 32 per unit.
Menu pauses the world separately and preserves an existing tactical pause.
Escape returns one menu page at a time. Restart and quit have confirmations.
Graphics, camera and interface settings plus completed-operation flags are saved
locally. This does not save unfinished mid-mission progress.
Options cover cones, path preview, stepped camera and four quality presets;
these real-time preferences are saved locally in GameUserSettings.ini and restored
after restarting the mission or game. Smoke fixtures neither load nor overwrite
player preferences. Quality presets are named Low, Medium, High and Ultra.

When path preview is enabled, selected party members show their queued
Move/approach-Interact/Takedown/Carry destinations as real NavMesh route segments. The active
member's route is gold and other selected members' routes are green; rings mark
the queued destinations. Stance, immediate interaction and distraction orders do not create
fake route segments.

## Verification

`./Start-Demo.ps1 -SmokeTest` runs a rendered native integration fixture.
`./Start-Demo.ps1 -CampaignSmokeTest -Mission 1` plays both new missions, traverses
the actual HUD menus and deploys into the next operation with live guards.
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
RTT control checks cover left-click selection versus right-click orders, contextual
TNT/guard/body targeting, paused double-click promotion, preserved stealth stance,
group formations, Ctrl+A/Tab/S/Escape, UI gating and separate touch-tap behavior.
Navigation checks cover preserved active paths after invalid replacements, paused
queue integrity, same-position orders, execution-time target reachability, bridge
disconnection and wall-blocked versus unobstructed body pickup.
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

Characters use derived 14-bone skinned models with velocity-driven leg/arm cycles
and smoothed stance transitions. Players and guards have a shared 180 cm standing
height. See `Docs/CHARACTER_MOTION.md` for measurements and the Blender pipeline.
Prone/body poses still include placeholder transforms; dedicated production
takedown/carry clips, foot IK, cloth and historical uniform review remain.
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
