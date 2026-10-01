# Real-Time Stealth Migration

## Boundaries

`AOKWinterEnvironmentActor` owns shared decorative world construction.
Neither it nor the real-time controllers call the legacy rule engine.
Only `OKObjectLayout` is reused to preserve approved centimetre-scale envelopes.

`AOKRTGameMode` owns mission state, party selection, physical bank/obstacle
surfaces, runtime NavMesh bounds, camera, noise pulses and preview spline.
`AOKRTUnit : ACharacter` owns movement, stance, health, body state and typed orders.
`AOKRTGuardController : AAIController` handles UE path following for every unit;
guard controllers additionally run Patrol -> Investigate -> Combat state logic.
`UOKRTVisionComponent` owns line-of-sight geometry and two procedural visual sectors.
`AOKRTPlayerController` owns terrain picking and pause-safe bindings.
`AOKRTHUD` draws selection, objective, skills, suspicion, paths and noise circles.
There is deliberately no inheritance from `OKDemoGameMode`.

The runtime scene adds a restrained `AExponentialHeightFog` layer for winter depth
and a cool movable sky fill. The fog is visual-only: it does not alter line of
sight, hearing, movement speed or NavMesh. Gameplay perception remains authoritative
through the vision component's explicit visibility traces, so the presentation
pass cannot make AI behavior disagree with the tactical rules.

## Navigation and Collision

The imported render ground has no cooked collision. Invisible BlockAll boxes
supply flat playable banks with a river gap and a separate raised bridge surface.
A cabin proxy blocks both Pawn and Visibility; a crate is functional cover.
Actor dimensions and navigation coordinates are world-space centimetres, not cells.
RVO avoids other moving/stationary characters without introducing tile occupancy.
NoCollision is set as an explicit collision profile on decorative static meshes
because StaticMeshComponent can restore default collision when re-registered.

`AOKRTNavBoundsVolume` adds a noncolliding box to the empty runtime brush so
NavigationSystem obtains valid component bounds. RuntimeGeneration is Dynamic.
Bridge destruction removes the physical bridge surface and updates navigation;
the existing viaduct also supplies a NavAreaNull gap obstacle.
Commands disallow partial paths. Hover preview uses the same synchronous
NavigationSystem path points, stored in a world-space spline and projected by HUD.
Production Landscape/mesh collision should replace flat proxies when terrain
elevation and authored interiors are introduced.

## Perception

Sight/Hearing configurations are registered on the guard controller and character
Sight stimuli register on units. Hearing callbacks start investigation at the
stimulus location, with a reduced occluded hearing reach. Noise event maximum
range differs by stance: run 1100 cm, walk 260, crouch 110, prone 55.
A distraction impact emits at its destination, not at the thrower.
Throws have 800 cm reach and a visibility obstruction check.

Sight authority is the two-zone component's continuous dot-product/range/Visibility
trace, evaluated against all living party units. The most detectable visible unit
is chosen so a crouching unit cannot mask a standing companion.
Near: 330 cm, 35-degree half-angle. Far: 1000 cm, 45-degree half-angle.
Near standing/no-cover detection is instant. Other near detections gain 2.4/s;
far detections gain 0.4/s, multiplied by walk/run 1, crouch 0.35, prone 0.15
and cover 0.3. Suspicion decays while unseen.
Visual cones are rebuilt at most every 0.12 s while visible and clipped by rays.
Hidden/carried bodies are excluded from discovery.

Guards investigate an audible event or newly seen dead guard, search for four
seconds, then resume waypoint patrol. Combat broadcasts once to guards within
1400 cm, tracks last seen location, shoots for 35 damage at 1.1 s intervals after
an initial reaction delay and searches after six seconds without sight.
Visible pursuit reuses UE's moving actor goal rather than replacing the request
each frame. Loss of sight immediately replaces that goal with the last seen world
position and clears actor focus. Failed/idle pursuit retries are bounded to 0.5 s;
both pursuit and last-seen investigation require complete navigation paths.
This is prototype AI, not realistic combat doctrine or a production hearing model.

## Pause and Orders

True game pause freezes CharacterMovement, AI, perception, timers and cooldowns.
PlayerController uses full paused ticking and pause-enabled key/touch bindings.
GameMode paused ticking is limited to runtime test bookkeeping and camera/input
management. No tiny time-dilation approximation is used.

Orders are Move, Interact, Takedown, Distract, Carry and Stance.
Move and target approach use complete UE navigation paths.
Unpaused replacement interrupts current movement; Shift appends.
Paused submissions append to a bounded queue; X clears it explicitly.
A queued interaction occurs after preceding movement. Failed paths end an order
with feedback rather than teleporting or crossing water.
Takedowns check range, approach direction, cooldown and obstruction at execution.
Body carrying reduces speed; hide interaction unregisters the hidden Sight stimulus.

Move and Stance apply to all selected units. Interact, Distract, Takedown and Carry
apply only to the active portrait, so group selection cannot spend two charges or
send two carriers to one body. Submit returns whether the order was accepted.
Invalid submissions leave the previous queue and movement intact. Targets are
validated again at execution; stale targets cancel their own approach path.
Friendly takedowns and carrying living/hidden/already-carried guards are rejected.
A carry may be queued behind a takedown of the same guard, but still requires a
dead body when executed. Cooldowns retain the pending attack/throw; empty inventory,
invalid targets and blocked/out-of-range throws produce feedback and end the order.
Invalid armed targeting remains armed for correction; X cancels it. Right-clicking
HUD controls does not activate them, and menu/outcome screens block skill arming.

## Selection Input

The controller's BeginPointer/UpdatePointer/EndPointer handlers separate a mouse
click from a box selection with an 8-pixel threshold. Left mouse press and release
bindings execute while paused. HUD and armed-skill clicks are handled on press;
ordinary world clicks wait for release. Touch retains its existing single-tap
path and is not converted into a drag gesture in this pass.

Box selection uses projected living party capsule centres and normalized screen
bounds, excludes HUD-covered centres, preserves selection on an empty rectangle,
and supports additive Shift selection. It does not enqueue orders. The old active
member survives when still selected; otherwise the first hit becomes active.
HUD marks the active member with a gold bar, existing crosshair asset and world
ring; other selected members are green. Portrait Shift-click both adds and
activates a member. Pointer state is cancelled on invalid screen coordinates,
menu/outcome gating, right-click, ability arming or explicit cancel. Touch
press/release bindings are pause-safe. Only a release within 24 screen pixels of
the press becomes a click; slides are ignored so a mobile finger gesture cannot
issue an accidental movement. With two active fingers, the controller pans from
the centroid delta and applies pinch distance to camera zoom. Single-finger orbit
remains a future extension.

When path preview is enabled, the HUD asks the Navigation System for a path from
each selected unit to every queued movement, takedown or carry destination. The
active unit uses a gold route and other selected units use green routes. Rings
mark valid queued destinations, while stance, interaction and distraction orders
remain route-free because they do not move the unit.

## Unreal Authoring

Public types and stance/order APIs are Blueprint-accessible.
Objective locations are EditDefaultsOnly world-space properties on the game mode.
For a larger mission, derive a GameMode/Unit Blueprint and move the prototype
objective coordinates and spawn layout into a mission data asset first.
PatrolRoute on guard controllers and cone ranges/angles are exposed for authoring.
Set party/guard identity before spawning the controller.
New playable scenery needs deliberate navigation and Visibility collision;
adding a render-only asset does not automatically make it cover.
The default mode is OKRTGameMode; the old game mode is available through a
`?game=/Script/OperaciaKopanice.OKDemoGameMode` map URL.

## Remaining Migration

Rig supplied characters and add locomotion/stance/takedown/carry animations.
Replace static render-pose approximations without changing command contracts.
Move the native guard FSM into optional Behavior Tree/Blackboard assets if
designer iteration requires it; do not connect the old turn-perception bridge.
Add authored large missions, persistence, proper objective components, audio/VFX,
weather-modified ranges, real terrain collision, touch camera gestures and
packaged/device validation before claiming production or cross-platform parity.
