# Real-Time Prototype Verification

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

## Not Verified

No packaged Windows build, Android device/APK, browser port, multiplayer,
shipping configuration or manual complete mouse/touch playthrough is certified.
No rigged animation exists on the supplied static character meshes.
Real-time large missions and production-grade terrain/interior collision remain
future work. Screenshots are actual prototype output, not reference-quality parity.
