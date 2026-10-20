# Real-Time Prototype Verification

## Winter Readability and Tactical UI: 2026-10-20

The main native game fixes a confirmed rig assignment defect: both underlying
skeletal asset slots reference WorldGridMaterial, not the supplied uniforms.
Runtime component overrides now bind the canonical materials for players and
guards. `InspectReadability-20261020.log` records source-slot paths/parameters;
the rendered fixture checks every animated unit's actual assigned material.
No skeletal mesh/texture reimport or large asset upload is needed.

UE 5.8.2 Win64 Development Editor builds successfully. Current development and
short-path DLL SHA-256 is
`58D8D6B21075A3FD22E60993FF30DFE95F89BA65CC78B43419ED5FDDE1DD30DA`.
The lighting run uses production-identical build
`28FEA6A35BFFB5BA94A3C00A39EF3B9FD415A5A3B642FA71BA0B9195CCCFBB0F`;
only the Modern fixture's bounded cooldown wait/diagnostic changed afterward.

| Rendered run | Result | Local log |
| --- | --- | --- |
| Four-quality lighting, uniform bindings, zoom/orbit | 18/18 PASS | DemoSmoke-de8e427626804815bd6053a4e2853722.log |
| Team plans, medic, save/reload and menu, 1280 x 720 | 41/41 PASS | DemoSmoke-531acad8321f4a2bb4b6d238552b879d.log |
| Same UI/plan regression, 480 x 800 | 40/40 PASS | DemoSmoke-3ae0986c122e461ca9fcdb1c95aa0990.log |
| Navigation/input/world integration | 279/279 PASS | DemoSmoke-962371f66d724e03b911413323dfcc9b.log |
| Distraction lifecycle and F9 reload, 480 x 800 | 43/43 PASS | DemoSmoke-ddb7cad1734b4613a6c021004c464033.log |

These total 421 checks with no failed checks/material fallbacks. The portrait
fixture omits the desktop-only minimap assertion. Captures were visually reviewed.
The previous live mission/campaign runs below precede this visual pass; they are
not represented as fresh complete campaign playthroughs on this renderer build.

The same 1280 x 720 Q2 close view is saved under local
`Saved/VisualReview-20261020/Before` and `After`. These are actual rendered captures,
not retouched images. Display-luma means use RGB weights .2126/.7152/.0722 on the
listed fixed rectangles, not physical light measurements or isolated surface masks:

| Screen rectangle (left,top,right,bottom) | Before | After |
| --- | --- | --- |
| Partisan coat (365,443,379,463) | 42.7 | 61.8 |
| Officer coat (448,511,461,529) | 41.0 | 51.3 |
| Shadowed cabin wall (650,220,720,270) | 30.7 | 64.2 |
| Snow (420,150,470,190) | 210.8 | 208.2 |

The scene review region (240,140)-(900,500) has 0% pixels at >=250 in every RGB
channel in both images. This supports improved dark-surface readability without
merely raising the snow exposure; it is not a full photometric quality benchmark.
Low probe intensity was rejected as too dark and the high-intensity experiment
as washed out, despite their settings-only checks passing. Final intensity is 3.5
with lower fog, bounded AO and moderated PBR normals. Sun shadows remain enabled.

One startup (`DemoSmoke-7434037bdfd04f92a1120f4504778801.log`) repeats the existing
DX12 page fault at frame 3 before any completed GPU frame. It is not claimed fixed.
The subsequent Modern attempt (`DemoSmoke-7844c964761741be88e8b641be466972.log`)
exposed a fixture assuming 1.7 wall-clock seconds equals a simulation cooldown.
The fixture now waits for batch completion with a 12-second wall-clock bound and
retains its exact resource/queue assertions; no production cooldown was shortened.
Final diagnostics show batch=0, ammo=4/4 and empty queues, and subsequent runs pass.

The material commandlet finishes with zero errors/two existing deprecated-API
warnings (`ReadabilityMaterials-20261020-final.log`). Four material packages total
70,850 bytes. Original textures/rigs, generated sources, logs, captures and binaries
stay outside upload. Engine/compiler deprecation warnings remain. This is an
editor-hosted prototype, not a packaged Windows/Android release or web update.
See [visual rules and limits](VISUAL_READABILITY.md).

Upload staging matches 231 native Source/Config/Content files and all 99 runtime
packages. Four changed material index/LFS hashes match the tested development
files. Only the native development branch is published; main remains untouched.

## Distraction Tactics: 2026-10-18

Main native game only; [rules, controls and reference distinction](DISTRACTION_TACTICS.md).
UE 5.8.2 Win64 Development Editor build and the HUD import commandlet succeed.
The development DLL matches the short-path build SHA-256:
`CD9E89DF17A9CA38099982CF4C8A87AECFB3F6DFE104684C7FD7D3A189B09CD6`.
Existing engine/input deprecation, preferred compiler and optional SDK warnings
remain. No packaged Windows/Android build or browser release is produced.

| Rendered run | Result | Local log |
| --- | --- | --- |
| Distraction lifecycle, 1280 x 720 | 43/43 PASS | DemoSmoke-be4ef46971e34ffa833e10898dc668d0.log |
| Distraction lifecycle, 480 x 800 | 43/43 PASS | DemoSmoke-0deb6216484b474f991d3976e22293b3.log |
| Synchronized plans, medic and F9 reload | 41/41 PASS | DemoSmoke-b2066dfccecc42b6b761a9460fccbd70.log |
| Nonlethal/carry/recovery regression | 59/59 PASS | DemoSmoke-8e85d63fb2f84d2e8086b62a58114e18.log |
| Original navigation/input/world integration | 279/279 PASS | DemoSmoke-04e88987328246b5a024dab1918d9ad7.log |
| Mission 1 with live AI | 15/15 PASS | DemoSmoke-35037e463b2841878764da5e7e2635a7.log |
| Production real-time startup | 11/11 PASS | DemoSmoke-ff916366077c496ca5e9335b4f279050.log |

These seven runs total 491 passing checks, zero failed checks and no material
fallbacks. They cover actual Y/F9 input, low/high and dynamically introduced
obstacles, unsupported/range rejection, queued-origin arc prediction, free hands,
delayed impact, event-specific hearing/occlusion, true pause, nonzero elapsed flight
save/reconstruction, single spending, mobile investigation, fixed posts, combat
priority and a mixed whistle/throw synchronization barrier. Current saves use
schema 3; schemas 1/2 remain readable with their older order/state limits.
Mission 1 completes with both live guards and full party health. Captures for
desktop/portrait were inspected; these are controlled rendered fixtures, not
packaged-device certification or an unassisted human playthrough.

The separate courier -> command-post campaign also passes on this build:
364 checks, both live operations completed with full party health, in
`DemoSmoke-0d2bd6d1526348daadacce8a99a5187b.log`. Repeated per-frame layout checks
make this campaign count timing-dependent.

The initial distraction fixture found a fixed sentry briefly following a retained
patrol path before hearing its lure (`DemoSmoke-6e116d6424144fb58d0fc42c46836021.log`).
HoldPost now stops retained paths and never navigates to route stops. The final
fixture records zero sentinel displacement. The launcher additionally rejects
explicit failed check lines even if a later PASS marker exists.

One Mission 1 startup failed with the existing DX12 GPU page fault before any
completed GPU frame (`DemoSmoke-2e2b8bb3ea5a4cf38cae616440d7744f.log`, frame 3,
3,127.71 MiB local use / 7,189 MiB budget). The separate retry above passes.
This is not claimed fixed or diagnosed as a specific driver/engine defect.

The isolated native-branch upload matches 229 Source/Config/Content files,
including 99 runtime packages checked against LFS hashes. One new icon is
13,536 bytes; no original models, manuals, generated PNG sources, logs or binaries
are added. All 33 generated-header include positions, launcher parsing and
`git diff --check` pass. The independently updated main checkout is not changed.

## Manual-Based Nonlethal Stealth: 2026-10-18

Only the main native game is changed. See [rules and reference pages](NONLETHAL_STEALTH.md).
UE 5.8.2 Win64 Development Editor compilation succeeds. The final HUD build
is copied into the development project; its DLL SHA-256 matches the short-path build:
`DB099F597B6CD6D9F15E91FFD3452895BCBA882A7837C61654199CA0662C49D0`.
Preferred-MSVC/optional SDK and engine/input deprecation warnings remain.
There is no new packaged Windows/Android/web release.

Final rendered runs, with no failed checks or material fallback errors:

| Run | Result | Local log |
| --- | --- | --- |
| Nonlethal lifecycle, 1280 x 720 | 59/59 PASS | DemoSmoke-ee1b174b32a34351a8dad30f86b1d2ff.log |
| Nonlethal lifecycle, 480 x 800 | 59/59 PASS | DemoSmoke-0e38e661516d4b08a6e0dc093dd555da.log |
| Command Mode, healing, save/load, 1280 x 720 | 41/41 PASS | DemoSmoke-b42fabf49b804231bfc7a3345a19c0da.log |

These exercise actual K/G bindings, rear/front attack rules, a real-time queued
knockout -> bind -> carry chain, contextual LMB binding, carry speed/no-run rules,
drop-before-run promotion without duplicate orders, hidden casualties, timed
unbound recovery, pause-frame gating, mobile assistance with real NavMesh arrival,
fixed-sentry duty, wall obstruction, snapshot rejection and F9 scene reconstruction.
Schema 1 validation remains supported; newly written saves use schema 2.
HUD control bounds and supplied rig initialization are checked. Desktop/portrait
captures were visually inspected; this is not packaged/device certification.

The complete preceding gameplay build
`39584845D921E3C0DA9E28B3E4EC69A6D86E1E88CFE580ADB5B03B54C17622DA`
also passed the following. The only subsequent runtime change is clipping casualty
indicators outside the viewport or under existing portrait/objective panels:

| Run | Result | Local log |
| --- | --- | --- |
| Original navigation/input/world integration, 1280 x 720 | 279/279 PASS | DemoSmoke-c330b63b8002467ab137c91b9e27c18a.log |
| Production real-time startup, 1280 x 720 | 11/11 PASS | DemoSmoke-32c29b14e54840b694be501da8dd93b7.log |
| Mission 1, live AI, 1280 x 720 | 15/15 PASS | DemoSmoke-50eaf60e0f684d5d86f056a76bcc1ba3.log |

The live courier -> command-post campaign passed 249 checks in
`DemoSmoke-5f634147bbe948ceb7d4f5e1b343ed5e.log` on build
`6456254A51400C56AE125FB6271349BBA372D44B2CEB3FB187E9AE183126B81C`.
Both missions retain living enabled guard brains and finish with full party health.
Its per-frame layout assertions make the count timing-dependent. That campaign run
precedes the final initial-pose/queued-run refinements; the whole campaign was not
repeated on the final HUD build. The subsequent lifecycle and integration fixtures
cover those refinements separately.

The isolated development-branch upload worktree matches 225 Source/Config/Content
files after text line-ending normalization, including 98 runtime package SHA-256
checks against Git LFS pointers. Two new icon packages total 29,780 bytes.
The icon import commandlet succeeds with zero commandlet errors/warnings.
33 generated-header include positions and `git diff --check` pass.
Original meshes, manuals, generated PNG sources, logs, captures and build output
are not added to the upload. The independently updated `C:/GitHub/Operacia-Kopanice`
`main` checkout is left unchanged; publication targets the native development branch.

Five earlier DX12 attempts failed with GPU MMU/page faults before the first
completed GPU frame: `DemoSmoke-dcf71d6da82640cb8ea49e4f95fde5f1.log`,
`DemoSmoke-24cf344035da44509296b15d35cbd494.log`,
`DemoSmoke-d784ee910bcb4b2d843069aec2d78a7c.log`,
`DemoSmoke-7f5818608dde416a893d5fd3518def30.log` and
`DemoSmoke-96340e1664fe4e59aa71aeb720481249.log`.
Successful runs do not prove this renderer/driver-path instability resolved, and
the dumps do not identify a root cause. The initial-pose fix is not claimed as a GPU
crash fix. An early pause-clock failure was corrected with explicit unit/guard
pause gates; paused synthetic-tick fixtures now explicitly resume before execution.
Authored knockout/binding/carry animations and production assistance AI remain
future work; current poses and instant-at-arrival assistance are prototype behavior.

## Native Rustbound Bridge and Lighting: 2026-10-13

Only the main native game is changed. Web runtime/assets and Workers deployment
remain untouched. UE 5.8.2 Win64 Development Editor compilation succeeds;
development/upload DLL SHA-256 matches:
`EFC780D07B92C52B1864A75F6E4596999FF64849AA7F73208A2846E86DD5537F`.
All 108 Source/Config files and 113 delivered Content files match between the
two checkouts. Seven new bridge packages total 14,232,951 bytes; original
Blender/FBX/PNG sources, build output and captures remain local/ignored.
The supplied original's SHA-256 is unchanged after the export pipeline.
Preferred-MSVC/optional SDK and engine/input deprecation warnings remain.
The upload checkout was later switched externally to `main`. It is left
untouched. A separate development-branch worktree verifies all 221 source,
config and Content files against development: text matches after line-ending
normalization, and every runtime package matches its Git LFS SHA-256 pointer.
These results concern `codex/native-ue54-environment`, not the old `main` tree.

Final editor asset validation (`BridgeFinalValidation.log`) passes with zero
commandlet errors/warnings: all three meshes are Nanite, dimensions/rail height
and material slots match, textures are 1024px with the expected color spaces,
normal convention and no emissive material input.

| Rendered run | Result | Local log |
| --- | --- | --- |
| Development lighting/bridge 1920 x 1080 | 10/10 PASS | DemoSmoke-96fc7ed4624348639889f2f7749b09de.log |
| Development lighting/bridge 480 x 800 | 10/10 PASS | DemoSmoke-12517d2e777c4e80ad6651148f905e91.log |
| Upload Command Mode/save/load 1920 x 1080 | 41/41 PASS | DemoSmoke-5bf05ef469cb463694e426bf2121cb20.log |
| Upload original/control/world 1920 x 1080 | 279/279 PASS | DemoSmoke-dac3e3d4e6ad40afbefbb24959f074f7.log |
| Upload original mission, live AI 1280 x 720 | 15/15 PASS | DemoSmoke-96bc58af229e4362b8722309e2d15e7f.log |
| Upload production startup 1920 x 1080 | 11/11 PASS | DemoSmoke-99b7d50100fb4026856d6444fe7eba7e.log |
| Development courier -> command post, live campaign 1920 x 1080 | 254/254 PASS | DemoSmoke-ab7f025750e04523933455465498a6bb.log |

The lighting fixture captures overview, zoomed and rotated views at all four
quality levels while paused; exposure/AO remain stable. It checks one movable
shadow-casting sun, real-time sky capture, nonblack lower-hemisphere bounce,
bridge deck/collision alignment and complete mission navigation. Low scalability
intentionally disables dynamic shadows; the other presets retain them. Captures
were visually inspected for readable wood/uniform detail and grounding shadows.
The save/load fixture additionally checks that sabotage restores the supplied
fractured mesh with permanent supports and disconnects river navigation.
The live Mission 1 run observes forty safe-spawn seconds and repeated patrol
windows, then moves both units across the new bridge, destroys it and extracts
with full health. Its playthrough keeps guard brains enabled and does not use
teleports, invulnerability or killed guards.
The final stable-checkout campaign run completes document retrieval, team
extraction, travel to the next mission, command-post sabotage and extraction
with active guard brains and full team health. Per-frame layout assertions
make its check count timing-dependent. The overview/close/portrait bridge and
two campaign world captures were visually inspected; central image-crop RGB
standard deviations of 64.48/66.78/72.44 provide a nonblank sanity check,
not a visual-quality score. No material fallback errors appear in these runs.

A same-camera paused FHD comparison uses mixed scene crops, not segmented
object measurements or a visual-quality score. Mean RGB / pixel-mean below 20:

| Mixed crop | Before | After |
| --- | --- | --- |
| Cabin (805,352)-(912,411) | 75.7 / 44.26% | 103.8 / 11.02% |
| Partisan (607,566)-(627,604) | 45.9 / 38.82% | 63.1 / 0.00% |
| Officer (664,601)-(681,637) | 55.4 / 41.67% | 69.7 / 0.16% |
| Snow (425,702)-(580,842) | 202.6 / 0.00% | 210.1 / 0.00% |

No pixels in the snow crop have mean RGB above 250. Character/cabin textures
are unchanged; editor exports confirm sRGB base color, linear packed masks and
mostly nonmetallic materials. The fix is authored light/exposure balancing,
not emissive meshes, removal of occlusion or photometric daylight calibration.

Two DX12 startup attempts hit a GPU MMU/page-fault crash on the NVIDIA adapter
(`DemoSmoke-9bc79e8aa284498189d4c2e36b3bec0c.log` and
`DemoSmoke-750aae2ee8c244cbbf4e54f9b853a2cb.log`). The latter crashed at frame 3
before any completed GPU frame, with 3,541 MiB used against a 7,189 MiB local
budget. Breadcrumbs include Nanite/VSM work; the dump does not identify a shader
or establish a driver-versus-engine root cause. Successful rendered runs do not
establish that this startup instability is resolved. An early material-pin
error was caught by actual rendering; the
import script now asserts every connection and final renders have no material
compilation/default-material fallback errors.

The bridge uses a pre-authored damaged-state swap, not Chaos or falling-debris
simulation. No UE 5.4 build, packaged Windows EXE or Android build is certified
by these editor runs. See [pipeline and limits](BRIDGE_PIPELINE.md).

## Main Native Modern Tactics: 2026-10-12

Only the main Unreal game changes. The web runtime, web assets and Workers
deployment remain untouched. UE 5.8.2 Win64 Development Editor compilation
succeeds; final DLL SHA-256:
`15C8C58273226DE35B33DB4362C6389F94A80EB748ACD0144151FF59A43414FC`.
Development/upload binaries match, as do all 107 Source/Config files and all
103 delivered Content files. The seven new HUD packages total 92,995 bytes.
No heavy mesh or source-model payload was added. Build products stay ignored.
Preferred-MSVC/optional SDK and engine/input deprecation warnings remain.

| Rendered run | Result | Local log |
| --- | --- | --- |
| Upload Command Mode/save/load 1920 x 1080 | 40/40 PASS | DemoSmoke-a37390ba239847ea9dd2773d750bf1ae.log |
| Development Command Mode/save/load 480 x 800 | 39/39 PASS | DemoSmoke-5a78c76de97b4373a3cdb002f9d524ca.log |
| Upload original/control/world 1920 x 1080 | 279/279 PASS | DemoSmoke-0f55862f9b844726bd8623defa2e93a1.log |
| Development original/control/world 480 x 800 | 279/279 PASS | DemoSmoke-e84bbc0e4e0c498d922d40a2997505cb.log |
| Upload production startup 1920 x 1080 | 11/11 PASS | DemoSmoke-695c7047578f47ff9682e8068e483a23.log |
| Upload original mission, live AI 1280 x 720 | 15/15 PASS | DemoSmoke-df82c6b20b3a4f8a9f3bb7e1f0f1fbbb.log |
| Upload courier -> command post, live campaign 1920 x 1080 | 302/302 PASS | DemoSmoke-c224a1e483e348a5bbbd0e7e5c262463.log |

`Start-Demo.ps1 -ModernSmokeTest` exercises actual Z/H/F5/F9 input, fresh isolated
save contents, friendly-marker healing, independent drafts, atomic duplicate-target
rejection, range validation, cooldown/readiness synchronization, cancellation and
no automatic pause on completion. Unknown schemas, invalid actor indices and
non-finite coordinates are rejected without mutating the world. Pending execution
barriers survive snapshots. A real level travel then restores a wreck, carried
body, inventory, guard suspicion/timers, camera, drafts and pause; it verifies the
river is disconnected and executes the restored plan exactly once.
This fixture disables guard brains and uses controlled setup/teleports for
serialization cases. It is not a live mission playthrough. Its save slot is separate
from the player's and is cleaned on success. The live mission/campaign fixtures
listed above retain active guard brains and complete all three operations with
both units at full health, without invulnerability, teleports or killed guards
during their playthrough phases. Campaign layout counts vary with frame timing.

Modern HUD captures were visually inspected at FHD and portrait resolution.
World-crop RGB standard deviations exceed 55 in all four plan/load captures,
providing a nonblank sanity check, not a visual-quality score. The first frame
can still show transient Nanite fallback detail while pages stream; captures
wait for multiple rendered frames. Imported editor render-data compilation is
finished before entering the paused scene. Additional nonblocking perimeter
dressing instances number 8/68/85 in Missions 1/2/3; existing tactical collision
and concealment are unchanged. Menu save/load rows and draft tools fit both
viewports; the desktop-only minimap click focuses the camera without unit orders.
The minimap shows enemies only through current party LoS, not omniscient radar.

No Unreal 5.4 build, packaged Windows EXE, Android build, co-op, six-specialist
roster, multi-step synchronized timeline or full production animation set is
certified. This is a tested native prototype, not Commandos Origins visual parity.

## Main Native World and Input: 2026-10-11

This slice changes only the main Unreal game and its documentation. The Canvas
runtime, web assets and Workers deployment are unchanged. The supplied manuals
and indexed mission extracts inform the [native world contract](GAME_WORLD.md);
no original manuals, mission scripts or game artwork are redistributed.

UE 5.8.2 Win64 Development Editor compilation succeeds. Final DLL SHA-256:
`402D054F241C3C3E1053444C0725244D4D8D4713B70E8BC2A0E4AB1193B1EEDD`.
Development and upload binaries match. All 101 Source/Config files and all 94
delivered Content files match between the two checkouts. Content packages are
unchanged; native DLLs, logs, research captures and build caches remain ignored.
Preferred-MSVC/optional SDK and engine/input deprecation warnings remain; this
does not certify an Unreal 5.4 build, shipping EXE, Android or web input parity.

| Rendered run | Result | Local log |
| --- | --- | --- |
| Development original/control/world 480 x 800 | 279/279 PASS | DemoSmoke-a2406d7af1b34b879fa9756e3f7ab0b2.log |
| Upload original/control/world 1920 x 1080 | 279/279 PASS | DemoSmoke-e72d1b1d19f04700aa3aca2a9e5225b0.log |
| Upload production startup 1920 x 1080 | 11/11 PASS | DemoSmoke-d3d004135a6244459262f0b26330156b.log |
| Upload original mission, live AI 1280 x 720 | 15/15 PASS | DemoSmoke-89572023955a4c5097c306f1b831b812.log |
| Upload courier -> command post, live campaign 1920 x 1080 | 286/286 PASS | DemoSmoke-b200331bf0bc4951845f4a1ae394b857.log |

The controls fixture dispatches actual UE mouse/hotkey events as well as projected
world-pick helpers. It covers LMB movement, double-LMB run promotion, RMB selection,
Ctrl membership/number hotkeys, cancellation over HUD, explicit silent attack,
typed E/hand targeting and rejection without losing plans. The real double-click
test caught UE emitting both pressed and double-click events; normalization now
issues that second click once. The planning fixture waits for the same complete
scene-navigation readiness gate as production, not just the first bridge path.

World tests cover all three objective chains, wrong-operation interactions,
casualty/empty/partial extraction rejection, bridge safety, authored guard data,
fixed versus mobile disturbance response, near/far stance detection and collision
occlusion. The original fixture isolates some guard behavior for deterministic
checks; production startup independently observes autonomous movement/patrols
without a Space press, then optional pause, queue execution and menu returns.
Native FHD and portrait order-plan screenshots were visually inspected for a
nonblank world, supplied assets and usable HUD. They remain under ignored Saved.
Central scene crops have RGB standard deviations above 59 in both viewports.
The separate original live-AI playthrough observes forty seconds of safe spawn,
repeated bridge watches and ten-second crossing windows, then collects TNT,
crosses, destroys the span and extracts both members at full health without
teleports, invulnerability or disabled guard brains.
The campaign fixture traverses native menus, tests forest/HUD behavior, deploys
the courier, extracts the team, travels through the results/next-operation flow,
then collects TNT, sabotages the command post and extracts both members. Its
playthrough phases keep guard brains enabled and use queued navigation/orders;
sabotage must attract a mobile patrol. Both mission results retain full team
health. Repeated per-frame layout assertions make the total count timing-dependent.

## Web Tactical Readability: 2026-10-11

This slice changes only the web runtime/interface and shared documentation. Native
source, binaries and content packages remain at the previously verified build;
no new native compilation or campaign playthrough is claimed for this UI slice.

Typecheck, production build and 102/102 Vitest tests pass: 43 real-time tests and
59 archived legacy tests. Eleven new regressions cover stable separated marker
geometry, HUD exclusion, dead/hidden/offscreen/enemy filtering, camera/stance
picking, foreground paint order, armed mouse/touch selection, marker box selection,
typed mission orders, bounded pagination and wide-road/stone stroke isolation.

Local Playwright passes 1920 x 1080, 480 x 800 touch, 360 x 740, 600 x 480 and
800 x 480 without runtime errors or overlapping/clipped HUD panels. It exercises
seven-command plans, five-slot paging, inspection without world changes, tail undo
without position rewinds, independent member queues and current-order pause state.
Development-only read-only diagnostics verify marker selection after zoom/rotation
and pixel colour above scenery. Production runs never expose that diagnostic API.
Actual wheel events are sent over terrain and canvas hashes must change; merely
remaining nonblank while the cursor is over a HUD button is not a zoom check.

Marker settings survive reload. Touch checks use real browser pointer events at
human-paced intervals and destinations clear of character selection hitboxes.
Screenshots/JSON stay under ignored Saved/WebRTT-2026-10-11. The web remains a
planar sprite adaptation, not native visual parity or a physical Android test.

Final live production verification also passes all five viewports without errors.
Worker version `210d0e51-9342-44ca-92da-73436b428aa4` serves
`index-CjqeLyZ6.js` (82.1 kB gzip) and `index-D9tgwA2B.css`.
Only source/docs and a 126,316-byte [live mobile capture](../../../artifacts/operacia-kopanice/docs/evidence/2026-10-11/web-rtt-mobile.png)
are committed; asset textures, UE packages, dependencies and native DLLs are unchanged.
Live reports/captures remain under Saved/WebRTT-Live-2026-10-11.

## Running By Default, Native and Web: 2026-10-08

The previous web entry point really was the legacy turn-based GO engine; native
deployment also forced an initial tactical pause. The default web entry now loads
an independent continuous-time simulation, and native deployment no longer forces
that pause. Space remains optional; neither version waits for a next-turn action.

UE 5.8.2 Win64 Development Editor compilation succeeded. DLL SHA-256:
`BDBD6CA3B835A67DCA03F3D8408260C580AE62419FC02F59233086771C1E591F`.
Development and upload DLLs match. All 100 Source/Config files and all 94 delivered
Content files match between the two checkouts. No content packages were added.
The DLL and build caches stay local, not in Git.

| Native run | Result | Local log |
| --- | --- | --- |
| Production-path startup 1920 x 1080 | 11/11 PASS | DemoSmoke-cf9df95fb8524306b14916f8538da6b8.log |
| Production-path startup 480 x 800 | 11/11 PASS | DemoSmoke-4ad246bdba30432ea50f36f63136cba8.log |
| Upload production-path startup 1920 x 1080 | 11/11 PASS | DemoSmoke-b233d8a541cc434d95ad3721c5e63995.log |
| Original planning/control regression 1920 x 1080 | 221/221 PASS | DemoSmoke-87c0358d02c842898806a1fdf2c7bae1.log |
| Original mission with live AI 1920 x 1080 | 15/15 PASS | DemoSmoke-0c4858f000c64bd792c697c105c7181d.log |

Run `Start-Demo.ps1 -RealtimeSmokeTest` for the new rendered fixture. It starts
without a Space press, accepts one destination and measures continuous movement,
observes patrols without further player input, then checks optional pause, queue
execution and both paused/running menu returns. Guard brains stay enabled and
no teleport, immunity or altered perception is used. The older isolated planning
fixtures opt into pause deliberately; they are not production startup behavior.

Web typecheck and production build pass. Vitest passes 91/91: 32 new real-time
simulation/input tests and 59 preserved legacy tests. Coverage includes fractional
movement, autonomous patrols, simultaneous party movement, frame-rate independence,
pause/cooldowns, validated queue replacement, gesture cancellation, LoS/stances,
ammunition reservations, route invalidation after bridge destruction and objectives
for all three maps. A complete first mission runs with active guards using actual
orders, takedown, carrying/hiding, sabotage and two-member extraction, without
teleports or invulnerability. The other maps' objective sequencing tests isolate
AI; they are not claimed as live-guard campaign playthroughs.

`tools/verify-web-rtt.cjs` verifies mouse/touch movement, optional pause, queued
commands, group movement, menu preservation, settings, zoom and visible canvas
pixels. Local development diagnostics additionally observe timed patrols; live
production checks read friendly HUD positions and timing, not hidden AI state.
Screenshots and layout checks cover 1920 x 1080, 480 x 800, 360 x 740, 600 x 480
and 800 x 480, including panel overlap and text/container bounds. The final build
is published separately with Wrangler to the existing Cloudflare Workers URL;
a source Git push alone does not deploy the web application.

Final live browser verification passed all five viewport sizes with no runtime
errors; production contains no development diagnostic object. Workers version
`731ebc53-638d-4e75-a520-182d0a49b023` serves `index-B9BYFxDr.js` and
`index-BDri2ssT.css`. The gzip JavaScript bundle is about 80.4 kB; existing artwork
is unchanged. The small [live mobile capture](../../../artifacts/operacia-kopanice/docs/evidence/2026-10-08/web-rtt-mobile.png)
is committed; desktop/options captures and JSON reports remain under ignored
`Saved/WebRTT` and `Saved/WebRTT-Live`.

The web remains a separate planar Canvas adaptation with existing PNG sprites,
PathFinding.js navigation and simpler effects, not an Unreal HTML5 build or art
parity. Physical Android hardware and packaged EXE/APK remain untested. Historical
reports below describe their own earlier builds; the web is no longer unchanged.

## Command Feedback and Route Previews: 2026-10-08

The UE 5.8.2 Win64 Development Editor build succeeded. Final DLL SHA-256 is
`B513DF8D37847F6F64087ECCAA188AE4B60ACB5F39F281887254817615368ED8`, matching
build stage, development and upload trees. All 100 source/config/project files
and all 94 required content files match between development and upload checkouts.
Existing preferred-MSVC, optional SDK and engine/input deprecation warnings remain.

| Run | Result | Local log |
| --- | --- | --- |
| Final original regression 1920 x 1080 | 221/221 PASS | DemoSmoke-942402e7640f499ea30396b44e6d7fdc.log |
| Final original regression 480 x 800 | 221/221 PASS | DemoSmoke-b6612ce5d2834c49bb26185a9822ea03.log |
| Campaign 1920 x 1080 | Both forest missions PASS; 346 PASS events | DemoSmoke-decdbd1f00de40db8c0d3519f02f03b3.log |
| Campaign 480 x 800 | Both forest missions PASS; 524 PASS events | DemoSmoke-1debb82ea85249dea198215309541285.log |
| Upload original live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-8bbbee47160b4f8cba6d561862a5ff13.log |

Logs remain local under Saved/Logs. Original/campaign regressions ran from the
development checkout; the final live-AI run used C:/GitHub/Operacia-Kopanice.
All runs exited successfully without failed checks, fatal errors, material compile
failures or default-material substitutions. Campaign event counts include repeated
layout checks, not that many distinct cases. Campaign runs preceded the final
armed-prompt priority and explicit armed-hover type corrections; the final original
regressions and upload live-AI run used the final DLL.

Twenty-four additional assertions cover read-only queued path origins, group
formation offsets, water/interior/elevated rejection, missing targets, acceptance
counts, independently full group queues, menu/result gates, resource preservation,
real-time expiry during pause, non-finite targets and responsive world indicators.
Rendered fixtures check that indicators avoid HUD controls and intercept no input,
retain armed targeting, and restore the original pending mission plan. Captures
confirm that a recent rejection remains readable instead of an armed target prompt.
These isolated fixtures do not execute mission objectives or grant player immunity.

Both larger missions and the original viaduct complete with both members at full
health and every guard alive with an active brain. No turn/grid execution is added.
Full HD and portrait accepted/rejected captures were visually inspected for rendered
assets, legible feedback and nonoverlap. Only the roughly 0.5 MiB
[portrait rejection capture](Evidence/2026-10-08/Command-Rejected-480x800.png)
is committed; other captures stay under ignored Saved/Verification.
No runtime asset packages were added. OS mouse timing, physical Android devices,
packaged EXE/APK and web parity remain unverified; this is the native editor-hosted
Windows demo and the legacy web game is unchanged.

## Tactical Order Planning: 2026-10-08

The UE 5.8.2 Win64 Development Editor build succeeded with the new order tray and
fixtures. Final DLL hashes match across build stage, development and upload trees;
all 98 source/config/project files match between development and upload checkouts.
Existing preferred-MSVC, optional SDK and engine/input deprecation warnings remain.

| Run | Result | Local log |
| --- | --- | --- |
| Final original regression 1920 x 1080 | 197/197 PASS | DemoSmoke-862be4d9148b4100aad508cd9cac2f1a.log |
| Final original regression 480 x 800 | 197/197 PASS | DemoSmoke-ad50679688474b25b2d3ca8a5c1a4376.log |
| Campaign 1920 x 1080 | Both forest missions PASS; 336 PASS events | DemoSmoke-4ed214ad8cf442a38ea8e59f62c8398f.log |
| Campaign 480 x 800 | Both forest missions PASS; 453 PASS events | DemoSmoke-3acee97cb37d499eb1e13828114684da.log |
| Upload original live-AI mission 1280 x 720 | 15/15 PASS | DemoSmoke-21aa6f1c69474131996cb2c344b0c6e0.log |

Logs remain local in each project's Saved/Logs. Original/campaign regressions
used the development checkout; the live-AI run used C:/GitHub/Operacia-Kopanice.
All runs exited successfully without failed assertions, fatal errors, material
compile failures or default-material substitutions. Campaign events include
repeated layout checks, not that many distinct cases. Campaign runs preceded the
compact portrait's redundant-crosshair removal; final original regressions and
the upload live-AI run cover that cosmetic delta.

Forty additional smoke assertions cover read-only typed order snapshots, invalid
indices, pause/menu/outcome/dead/enemy gates, waiting-tail removal, resource
preservation and protection of a started UE movement request, including pace
promotion. Rendered HUD checks exercise inspection, panel input blocking, touch
undo, the 32-order limit, all seven pages, pause-enabled Backspace, page clamping
after removal, specialist switching and restoration of the original mission plan.
The isolated path fixture explicitly invokes a zero-delta unit tick while paused,
then restores the queue; it does not complete objectives or move actors to targets.

Both forest missions and the original viaduct retain successful two-member
extraction at full health with guards alive and brains active. Planning stays
real-time with true tactical pause, not turn execution or action rewind.

Full HD/portrait plan and portrait capacity captures were inspected for rendered
assets, readable counters, page indices and nonoverlapping controls. Only the
roughly 0.5 MiB Evidence/2026-10-08/Order-Plan-480x800.png is committed; the other
captures remain local under Saved. No runtime asset packages were added. OS mouse
timing, physical Android devices and packaged Windows/Android builds remain gaps.
This is the native editor-hosted Windows demo; the legacy web game is unchanged.

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
