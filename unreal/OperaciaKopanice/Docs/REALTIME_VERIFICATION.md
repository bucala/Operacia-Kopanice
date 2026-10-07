# Real-Time Prototype Verification

## Party Selection Through Scenery: 2026-10-07

The UE 5.8.2 Win64 Development Editor build succeeded with the two new marker
translation units. Build-stage, development and upload DLL hashes match; all
190 runtime/source/config files also match between development and upload trees.
Existing preferred-MSVC, optional SDK and engine/input deprecation warnings remain.

Final rendered regression/campaign runs used the development checkout; the final
live-AI run used `C:/GitHub/Operacia-Kopanice/unreal/OperaciaKopanice`. Logs remain
local under each project's `Saved/Logs`:

| Run | Result | Local log |
| --- | --- | --- |
| Original regression 1920 x 1080 | 157/157 PASS | DemoSmoke-4c903d4c218c4358a50340696d1c8264.log |
| Original regression 480 x 800 | 157/157 PASS | DemoSmoke-c55a3818c69c4b80b8ab1f2223207f64.log |
| Campaign 1920 x 1080 | Both forest missions PASS; 353 PASS events | DemoSmoke-640de0c53405445e84cc70070f4d077f.log |
| Campaign 480 x 800 | Both forest missions PASS; 461 PASS events | DemoSmoke-0f4d753cd94844d1a0771bbda63b9b1e.log |
| Upload original live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-16653d12496c49ef98119543677508d3.log |

These final runs exited successfully without failed assertions, fatal errors,
material compilation failures or default-material substitutions. Campaign events
include repeated layout checks, not that many distinct test cases.

Twenty new isolated marker checks cover two distinct hit regions, selection
through an actual opaque render/pick blocker, Shift selection, touch selection,
right-click/double-click exclusion, armed-target rejection/cancellation, preserved
queues and tactical pause, dead/hidden/enemy exclusion, immediate toggle effects,
menu/results suppression, overlapping allies, offscreen/HUD exclusion and paused
camera orbit/zoom. Temporary fixture geometry and party transforms are restored;
these checks do not complete objectives or grant health in the live playthroughs.

A further control regression reserves guard and objective pick regions. The first
portrait trial exposed an ally marker intercepting a guard click; placement now
avoids projected character bodies and interaction points, and the final portrait
run passes enemy inspection, contextual takedown and body pickup unchanged.
Campaign fixtures exercise the real interface toggle and ensure shrub concealment
retains a selectable friendly marker. Both new missions still extract at full
health with all guard brains active; command-post sabotage provokes investigation.
The original viaduct also retains its safe forty-second opening, timed bridge
crossing, sabotage and full-health extraction with both guards alive and active.

The committed `Evidence/2026-10-07/Party-Markers-1920x1080.png` and
`Party-Markers-480x800.png` were inspected for nonblank assets, readable party
identity and separation from HUD controls. This is a screen-space selection aid,
not a character silhouette shader or an enemy-reveal system. No new runtime asset
packages were added. OS mouse timing, a preference-file restart round-trip and
physical Android devices remain manual gaps. Native editor-hosted Windows only;
the legacy web game and deployment are unchanged.

## Functional Forest and Concealment: 2026-10-07

UE 5.8.2 Win64 Development Editor incremental build succeeded, including the new
forest fixture. Campaign runs used the development checkout; original-mission
regressions used `C:/GitHub/Operacia-Kopanice/unreal/OperaciaKopanice` with the
same compiled module. Hashes matched for all 188 runtime/source/config files.
Logs remain local under each project's `Saved/Logs`.

| Run | Result | Local log |
| --- | --- | --- |
| Development campaign 1920 x 1080 | Both forest missions PASS; 347 PASS events | DemoSmoke-4bdc12ef6ee64a29ae66b62bf67271fd.log |
| Development campaign 480 x 800 | Both forest missions PASS; 455 PASS events | DemoSmoke-8e3517766c0c4e9e9112b01d2a2469bb.log |
| Upload original rendered regression 1920 x 1080 | 136/136 PASS | DemoSmoke-b622316d108c44388c74b88120a544b7.log |
| Upload original live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-e9ed2ccb926541b1941c6f41ca211026.log |

All engine runs exited successfully, without failed assertions, fatal errors,
material compilation failures or default-material substitutions. Campaign event
counts include repeated layout checks, not that many distinct cases. Existing
preferred-MSVC, optional SDK and engine/input deprecation warnings remain.

Eleven isolated forest assertions per new mission verify substantial trunk/rock/
shrub placement, a complete NavMesh detour around a real trunk, blocking of the
character capsule, trunk occlusion through the authoritative vision component,
and visibility beside the same trunk. Shrub cover has finite horizontal/vertical
bounds: standing visibility remains 1, crouching in cover is 0.105, and prone in
cover is 0.045, never zero. Larger maps do not inherit the original virtual cover.
These paused unit checks temporarily position and restore actors; they are
separate from the mission playthrough and do not grant objectives or health.

Both real-time playthroughs navigate into an authored shrub using production
queued stance/movement orders, display KRYT, then walk out to complete objectives
and extract both members at full health. All three enemy brains remain active
and all guards survive. Sabotage still draws an investigating patrol. These
playthroughs use no teleportation, guard disabling or invulnerability.

Full HD and portrait cover captures were inspected for visible assets, readable
cover state and nonoverlapping HUD elements; selected captures are committed in
`Evidence/2026-10-07/Forest-Cover-1920x1080.png` and
`Evidence/2026-10-07/Forest-Cover-480x800.png`. No new asset packages were required.
Tree collision describes trunks, not canopies; shrubs reduce detection rather
than physically obstructing navigation. Final terrain/road dressing, physical
mouse timing, mid-mission saves and packaged Windows/Android builds remain out
of scope. The legacy web game and deployment are unchanged.

## Native Campaign and Menu: 2026-10-07

A fresh Win64 Development Editor build from the upload checkout succeeded with
UE 5.8.2, followed by an incremental build of the startup navigation-input guard.
Existing preferred-MSVC, optional SDK and engine/input deprecation warnings remain.
This is not UE 5.4, packaged Windows or Android certification.

Final runs below used the same compiled module and runtime assets in
`C:/GitHub/Operacia-Kopanice/unreal/OperaciaKopanice`. Evidence logs are local
under that project's `Saved/Logs`, not included in the release payload.

| Run | Result | Local log |
| --- | --- | --- |
| Campaign 1920 x 1080 | Both new missions PASS | DemoSmoke-263a8a5568334436b9a34e246a990ba8.log |
| Campaign 480 x 800 | Both new missions PASS | DemoSmoke-654f6276afa04e2283f21765603fb80c.log |
| Original rendered regression 1920 x 1080 | 136/136 PASS | DemoSmoke-e1d89378aa504326a0620aa8fbb1e3b8.log |
| Original live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-38dddc21826045e89c186630a4d70ea0.log |

All four runs exited successfully with no failed assertions, fatal errors,
material compilation failures or default-material substitutions. The campaign
logs contain 271 and 421 PASS events respectively; these include repeated
per-frame layout checks, not that many distinct test cases.

The campaign fixture exercises main/pause pages, three-mission selection, briefing,
settings tabs and toggles, restart/quit cancellation, objective camera focus and
results-to-next-mission OpenLevel travel through production HUD callbacks. Early
pause/menu requests cannot freeze initial navigation construction. All authored
patrol stops and objective/extraction routes are navigable. Visible imported
alternate cabins, command vehicles and more than 200 forest instances per map
are checked; Nanite/instancing material usage flags are saved in the assets.

Both new missions extract both members at 100 health, with all three guards alive
and their brains enabled. Command-post sabotage provokes an investigating patrol.
No teleportation, killed/disabled guards or invulnerability completes these runs.
The original mission still completes a safe start, timed bridge crossing,
sabotage and extraction with both guards active.

Full HD scene/menu and portrait menu, briefing, options and gameplay captures
were inspected for nonblank textured rendering and nonoverlapping HUD elements.
Selected [in-engine captures](CAMPAIGN.md#captured-build) are committed. Runtime,
source and config hashes matched between upload and development trees (186 files).

Physical mouse timing, a preference-file restart round-trip and Android devices
remain manual verification gaps. Fixtures intentionally avoid writing player
preferences. Decorative forest collision, mid-mission saves, audio controls and
remappable/gamepad menus are not supplied by this pass. The legacy web game and
its deployment are unchanged.

## Safe Navigation and Body Reach: 2026-10-06

UE 5.8.2 Win64 Development Editor build succeeded. Existing preferred-MSVC and
engine/input deprecation warnings remain; this is not UE 5.4 certification.

| Run | Checks | Local Saved/Logs evidence |
| --- | --- | --- |
| Rendered 1920 x 1080 | 136/136 PASS | DemoSmoke-1620deb643354d65b14ca7a558304f8c.log |
| Rendered 480 x 800 | 136/136 PASS | DemoSmoke-eac1861610464ce49068cf0d7d465d00.log |
| Live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-2b9d1a9232004ac7ad9520c897fa707d.log |

All three final runs exited zero with no failed checks, fatal errors or material
compilation failures. Desktop and portrait pause screenshots were inspected for
nonblank sharp rendering, visible routes and nonoverlapping HUD elements.
The log still reports missing optional CabinAlt/CommandVan meshes; no assets were
added in this pass. Logs and screenshots remain local, not release payloads.

Twenty-two added checks exercise production command submission and native path
following. They cover rejecting water, off-mesh banks, cabin interiors, elevated
goals, target approaches and contextual interactions without replacing valid
orders; double-click promotion cannot bypass validation. Appended paused routes
start from the last queued destination, identical endpoints remain valid on the
NavMesh, and an unreachable resumed approach preserves the following stance order.
Body pickup fails through a visibility-only wall and succeeds after its removal.
After bridge destruction, a disconnected-bank order preserves a reachable move.

The separate live-AI mission still completes contextual TNT pickup, timed crossing,
sabotage and extraction with both guards active, no killed guards and no damage
to either party member. This pass uses rendered Windows fixtures, not physical
mouse timing, an Android device or a packaged Windows executable. The legacy web
game and its deployment are unchanged.

## Commandos-Like RTT Controls: 2026-10-06

UE 5.8.2 Win64 Development Editor build succeeded. Existing MSVC preferred-version
and engine/input deprecation warnings remain; this is not UE 5.4 certification.

| Run | Checks | Local Saved/Logs evidence |
| --- | --- | --- |
| Rendered 1920 x 1080 | 114/114 PASS | DemoSmoke-455600351db44aafa4e66741768ace5b.log |
| Rendered 480 x 800 | 114/114 PASS | DemoSmoke-16fd3c900b024db48f2ab234738fa3b9.log |
| Live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-2399c44ecde044c6b8f460c851e8e2bf.log |

All runs exited zero with no failed checks, fatal errors or material compilation
failures. Desktop and portrait pause screenshots were inspected for nonblank
scene rendering, clear HUD separation, route rings and sharp paused camera output.

Thirty-three additional checks use the production pointer/command/tap handlers,
real screen projection and collision hit tests. They cover selection-only terrain
left clicks, captured Shift selection, right-click contextual orders, group
formation, double-click promotion without duplicate paused orders, run on resume,
preserved crouch, friendly/HUD/menu exclusion, cone inspection, body pickup,
typed interactions, planning future detonator/hide orders, active-only abilities
and separate touch intents. Ctrl+A, Tab, S and Escape are dispatched through the
native InputKey/PlayerInput stack while paused; the double-click binding is checked.
The complete fixture also navigates to and executes typed hide/TNT interactions.

The live-AI fixture now acquires TNT through the controller's screen-targeted
contextual approach command, not a separate Move plus immediate Interact. Both
guards remain active, the spawn stays safe for 40 seconds, and both party members
extract at simulation time 67.26 seconds with 100 health. No guards are killed or
disabled and no actors are teleported to complete that mission.

An OS mouse playthrough was attempted in a native demo window but stopped before
input execution when the tool reported concurrent user input. Physical desktop
double-click timing therefore remains a manual verification gap. The portrait
run tests Windows layout and touch-handler routing, not an Android device build.
No assets, legacy web rules or web deployment changed in this control pass.

## Crisp Zoom During Tactical Pause: 2026-10-01

The native real-time and legacy cameras now override motion blur to zero. This
keeps orbit, zoom and two-finger pinch interactions sharp while the world is
paused, where a camera transform could otherwise leave a stale radial blur in
the render history. The real-time smoke fixture checks both motion-blur
overrides on the active camera before continuing the mission.
The rendered 1920 x 1080 smoke test passed 67/67 checks with zero failures,
fatal errors or material compilation errors. Evidence: `OKRT_Pause.png` was
inspected after the paused camera orbit/zoom pass and remains sharp.

## Queued NavMesh Routes: 2026-10-01

The HUD now draws queued Move/Takedown/Carry destinations for selected party
members by querying the same NavMesh path system used by command execution. The
active member is gold, other selected members are green, and each valid
destination receives a world ring. Stance, interaction and distraction orders
do not create fake route segments.

The rendered 1920 x 1080 smoke test passed 66/66 checks with zero failures,
fatal errors or material compilation errors. The pause screenshot was inspected
for visible gold and green route segments and destination rings. Build used UE
5.8.2 Win64 Development Editor with the existing toolchain/deprecation
warnings.

## Release-Based Touch Input: 2026-10-01

The real-time controller now records Touch1 press and evaluates the tap on release.
A release within 24 screen pixels activates the existing UI/world click path; a
finger slide is ignored. Press/repeat/release bindings execute while the world is
paused, and menu or cancel clears the pending touch state. Two active fingers pan
from their centroid delta and use pinch distance for camera zoom. This is a safe
touch interaction pass, not Android device certification.

The rendered 1920 x 1080 smoke test passed 65/65 checks with zero failures,
fatal errors or material compilation errors. The new touch binding regression passed
alongside the existing mission, AI, command, selection and HUD checks. Build used
UE 5.8.2 Win64 Development Editor with the existing toolchain/deprecation warnings.

## Winter Atmosphere Pass: 2026-10-01

Added a restrained winter presentation pass to the native runtime scene: a blue
ambient sky fill, low-density height fog with a higher distant layer, directional
snow-coloured inscattering and a small cool colour/exposure correction. The river,
bridge, collision proxies, NavMesh and objective layout were not changed.

The rendered Full HD smoke run passed all 64 checks with zero failed checks,
fatal errors or material compilation errors. Evidence: `RealTime-Atmosphere-2026-10-01.png`.
The screenshot was inspected for river readability, building silhouettes, party
portraits, objective markers and tactical controls. Build used UE 5.8.2 Win64
Development Editor; the existing preferred-toolchain/deprecation warnings remain.

## Box Selection and Active Member: 2026-10-01

UE 5.8.2 Win64 Development Editor build succeeded with the existing MSVC/engine
deprecation warnings. Both rendered runs passed all 64 checks, exit code zero:

| Viewport | Local log | Screenshot |
| --- | --- | --- |
| 1920 x 1080 | DemoSmoke-7e35cb9d6fb4475facbaf6b1f532ee34.log | RealTime-Selection-2026-10-01.png |
| 480 x 800 | DemoSmoke-b81c95dcad7f48bcae0ec438034d03a6.log | RealTime-Selection-Portrait-2026-10-01.png |

Logs are under Saved/Logs. No failed checks or material compilation errors were
found. Both pause screenshots were inspected: the active crosshair fits the
portrait, selected companions have a distinct bar/ring, and PAUZA remains visible.
The capture now waits for a paused frame rather than immediately resuming before
the screenshot renders. Screenshots show the completed group selection, not a
mouse drag in progress.

Eighteen new checks cover projected selection bounds, both drag directions,
single/additive selection, preserving the active member, no incidental movement,
true pause, dead/enemy exclusion, cancel/menu/viewport/HUD gating, small-motion
clicks and pause-enabled press/release bindings. Tests use the production pointer
handlers and real screen projection/hit tests; no physical OS mouse/touch session
was automated. The complete mission and prior command/AI regressions also pass.

No assets or legacy gameplay rules changed; legacy/commandlet results below are
earlier baselines, not reruns. The portrait run is a Windows layout regression,
not Android device certification. Touch gestures, rigs and animation remain out
of scope for this pass.

## Command and Pursuit Regression: 2026-09-28

Win64 Development Editor rebuilt successfully using the same UE 5.8.2/toolchain
below. Rendered 1920 x 1080 integration passed all 46 checks, exit code zero.
Local log: `Saved/Logs/DemoSmoke-24e36b45d5d640d4b9b23d1b27328263.log`.
No failed checks or material compilation errors were found. The current pause
screenshot was inspected; the supplied environment and character meshes render.

Fourteen additional checks cover rejection of friendly takedowns without cancelling
movement, rejecting living-body pickup, queued takedown/pickup, cancellation of a
stale approach, cooldown retention and exactly-once throw execution, empty ammo,
active-member-only group abilities, queue capacity, menu skill gating, stable
combat path request IDs, last-seen goals after occlusion and search transition.
The existing full mission route, pause/resume, hearing, victory and loss also pass.
Command fixtures call the same native Submit/Command/Tick APIs used in gameplay;
they are not a complete automated mouse/touch interaction test.

No rendering/layout code, assets or legacy rules changed in this pass. Portrait,
legacy and the six commandlet tests below are the previous baseline, not reruns
of this revision. Engine/toolchain warnings and the limitations below still apply.

## Baseline: 2026-09-27

Host: Windows, UE 5.8.2, Win64 Development Editor, MSVC 14.51.36247.
Build succeeded; the engine warns that this compiler is newer than its preferred
14.50 toolchain and emits engine-header deprecation warnings. This is not a
warning-free build or a UE 5.4 compatibility certification.

## Rendered Integration

| Run | Result | Evidence |
| --- | --- | --- |
| Real-time 1920 x 1080 | 32 checks, PASS | RealTime-2026-09-27.png, RealTime-Pause-2026-09-27.png |
| Real-time 480 x 800 | 32 checks, PASS | RealTime-Portrait-2026-09-27.png |
| Legacy grid 1280 x 720 | PASS, all three layouts | Legacy smoke remains selectable in launcher |

The narrow run was repeated after removing overlapping world-space objective
labels from the compact HUD. Viewport button bounds/separation passed in both sizes.
Scene-only pixel checks exclude the corner HUD and report RGB standard deviations
of approximately 79.7/77.1/74.5 at desktop and 70.4/68.1/65.5 at portrait size.
Both actual render outputs were inspected, not merely generated by a blockout script.

Integration checks cover complete NavMesh paths, a nonwalkable river, supplied
character meshes, near cone geometry and wall occlusion, stance-scaled far
suspicion, party movement, patrol movement, an initial navigation-ready planning
pause, Space-key pause/resume, frozen position, queued orders, takedown, carrying,
hiding, distraction ammo/cooldown, native Hearing investigation and movement,
TNT pickup, both members crossing the bridge, sabotage, dynamic removal of the
bridge path, extraction, no legacy turn advancement, combat detection and death.

Guard brains are disabled during controlled ability/navigation phases and enabled
for separate patrol, hearing and combat checks. The fixture teleports actors only
to set up isolated cone/takedown/combat situations, not to complete the bridge route.
The full route to extraction uses real CharacterMovement and path following.

## Existing Automation Suite

All six project tests returned Success: Demo.ActionContracts,
Demo.AllMissionsSolvable, Demo.ObjectLayout, PCG.Determinism, Terrain.Blending,
Turns.PhaseGuards. Commandlet exit code was zero. These are existing regression
tests; the new gameplay is exercised by the rendered real-time fixture above.
The commandlet also emitted two generic 'Condition failed' messages before the
project test queue started, near engine UnifiedError/low-level startup tests.
The automation log is therefore not globally error-free; no project test reported
a failed result. Raw logs remain under Saved and are not committed.

Seven supplied base materials were saved with Nanite usage enabled via
prepare_realtime_materials.py; subsequent rendered runs no longer reported missing
Nanite usage or failed material compilation.

## Mission Playability Fixture

Run `./Start-Demo.ps1 -MissionSmokeTest` after building. This separate rendered
fixture leaves both guard brains, sight, hearing, patrol routes and damage active.
It checks patrol reachability, waits at the unmodified spawn for 40 simulation
seconds, measures bridge observation/crossing windows, and issues normal stance,
movement and interaction orders through TNT pickup, sabotage and extraction.
It fails on combat, death, disabled guards or an unreachable objective. It never
teleports actors or neutralizes guards to complete the route.

On 2026-10-05, the rendered 1280 x 720 run passed all 14 checks. Both guards
remained active. The unmodified spawn stayed at zero suspicion for 40 seconds;
the party then collected TNT, crossed together, destroyed the bridge and reached
extraction at simulation time 67.24 seconds with 100 health each. Evidence:
`OKRT_MissionWin.png`; local log `DemoSmoke-65a9f1d8b65a42009567546001a3ee23.log`.
The broader 1920 x 1080 fixture then passed 77/77 checks, including animations,
pause/queues, selection, takedown/carry/hide, hearing, sabotage, victory and combat
loss, with no material compilation failures (`DemoSmoke-0bf6ad780a80445a9aedd1c72ca55f60.log`).
Measured gait offsets remained 0.0000 cm lateral / 0.5992 cm vertical.
The 480 x 800 HUD/animation fixture also passed earlier on the same date, before
the patrol revision (`DemoSmoke-5bb1b3aa3b944c92a2ba21b0a408050b.log`).

## Sabotage Safety: 2026-10-06

The rendered 1920 x 1080 integration fixture passed 81/81 checks after the
detonator lock was added (`DemoSmoke-1e1c81abdbe84b77abb899e502302973.log`).
New cases attempt sabotage with a teammate still west and with its capsule
overlapping the east end of the span. Both preserve the bridge and noise state;
the west-bank attempt also preserves a complete cross-river NavMesh path.
The objective reports partial crossing and unlocks detonator activation only
after both members fully clear the span. Normal sabotage, extraction and the
separate combat/loss fixture still pass. Material compilation reported no failures.
The 1280 x 720 live-AI mission fixture also passed 14/14 checks with the new lock
(`DemoSmoke-fae9d6e112d94f26b944751bf19b9bee.log`). Both units extracted at
simulation time 67.25 seconds with full health and both guards active.

## Current Verification Limits

No packaged Windows build, Android device/APK, browser port, multiplayer,
shipping configuration or manual complete mouse/touch playthrough is certified.
Derived skinned character models now use procedural locomotion; production
animation clips, foot IK and cloth simulation remain unimplemented.
Real-time large missions and production-grade terrain/interior collision remain
future work. Screenshots are actual prototype output, not reference-quality parity.
