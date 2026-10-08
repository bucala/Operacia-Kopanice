# Stealth Real-Time Tactics Migration

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

`OKRTMission` contains the three immutable operation definitions. GameMode keeps
per-run objective state and builds the selected runtime scene; `OKRTMissionScene`
adds the larger forest layouts without introducing grid movement. `OKRTMenu`
owns HUD menu pages and native action callbacks. Deployment uses OpenLevel URL
options so results, briefing and restart preserve the chosen operation. See
[campaign and menu details](CAMPAIGN.md) for placement, persistence and limits.

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
Mission 1 permits sabotage only when every living party member's capsule clears
the east end of the physical span. A rejected interaction leaves geometry,
navigation and noise unchanged. HUD objectives count safe members during the
crossing and switch to detonator activation once the entire party is across.
Commands disallow partial paths. `AOKRTUnit::GetOrderPath` is the shared read-only
navigation query for submission and hover previews: it checks complete agent paths,
horizontal/vertical endpoint tolerance and real navigation even for identical
endpoints. Preview starts from the last queued destination during pause/Shift
append and applies the active specialist's formation offset. Water projected onto
a bank, inaccessible interiors and elevated goals cannot produce a valid preview.
Typed guard/body/interaction approaches use their own acceptance tolerances.
Preview points are stored in a world-space spline and projected by HUD; preview
does not validate ability resources or promise that a moving target stays reachable.
Production Landscape/mesh collision should replace flat proxies when terrain
elevation and authored interiors are introduced.

`Command` records requested/accepted member counts and a snapshot of the clicked
world target. The HUD draws a noninteractive acknowledgement for up to 2.4 seconds:
green accepted, red rejected or amber partial acceptance, with N/N for groups.
Group commands retain independent acceptance; a failed member's queue is untouched
and its named reason is kept in the feedback message. The marker uses real time,
so it expires during true tactical pause, and is omitted offscreen, behind HUD
controls or in menus/results. It adds no noise/perception event, target tracking,
hit region or resource cost. Disabling route preview does not disable acknowledgement.
While targeting remains armed, a recent rejection message temporarily takes
priority over its target-selection prompt; the ability is not cancelled by drawing.

In Missions 2 and 3, forest render instances keep HISM batching and NoCollision.
Their visible owner also holds narrow trunk capsules and rock boxes with BlockAll
and navigation relevance. These proxies obstruct Pawn and Visibility without
making whole tree canopies impassable. Deterministic placement reserves patrol,
objective and approach clearings and rejects overlap with existing obstacles.
Shrubs have no physical/nav obstruction: world-space cylinders derived from their
placed bounds grant concealment only when a crouched/prone body fits below the
clump height. Crouch tests 95 cm and prone 35 cm above the feet; cover applies the
existing 0.3 visibility multiplier, never zero. Standing clears cover immediately.
The HUD's KRYT badge reflects the live unit state. Existing tactical crate/rock
cover is also vertically bounded; Mission 1's virtual cover remains mission-local.

## Perception

Patrol routes contain `FOKRTPatrolStop` entries (world position, facing yaw and
wait seconds). Controllers first reach the stop, rotate at 90 degrees/second,
then start its observation timer. Failed paths retry without consuming a stop;
investigation interrupts the timer and returns to the pending stop afterward.
`PatrolWaitRemaining()` exposes the current stop timer to Blueprint/debug tooling.
Mission 1 starts south of the west patrol. Its north-facing rest lasts 12 seconds,
its east-facing bridge watch 2 seconds, and an explicit north-facing turn stop
keeps the returning cone away from the spawn. The east patrol stays on the
southern bank with an 8-second rest and a 2-second westward watch.

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

Production deployment runs as soon as navigation is ready. There is no forced
initial tactical pause. The main/pause menu freezes the world separately and
restores the previous tactical state on resume. Old planning regression fixtures
explicitly opt into pause; `-OKRTRealtimeSmoke` tests the running production path
with live guard brains, continuous movement and optional pause/resume.

True game pause freezes CharacterMovement, AI, perception, timers and cooldowns.
PlayerController uses full paused ticking and pause-enabled key/touch bindings.
GameMode paused ticking is limited to runtime test bookkeeping and camera/input
management. No tiny time-dilation approximation is used.

Orders are Move, Interact, Takedown, Distract, Carry and Stance.
Move and target approach use complete UE navigation paths.
Unpaused replacement interrupts current movement; Shift appends.
Paused submissions append to a bounded queue; S/X clears it explicitly.
`GetQueuedOrder` returns a copy for read-only HUD inspection. `OKRTOrderQueue`
shows five typed, numbered orders per page and resets/clamps pages after changing
specialist or queue length. Existing bitmap icons carry action/stance identity;
hover/tap inspection changes feedback only. The whole tray blocks world picking,
including disabled arrow/undo controls, and is omitted if it would intersect the
portrait/action/message HUD on a short viewport.

`UndoLastOrder` requires both tactical pause and a truly paused world, a living
friendly unit, no menu/outcome, and a waiting queue tail. `HasStartedOrder` also
checks UE path-following status, so double-click pace promotion cannot expose an
active path as a pending order. Removing a waiting tail does not stop/reissue the
retained navigation request, alter resources or affect another member. The
pause-enabled Backspace binding and HUD undo act only on the selected active
specialist. This is plan editing, not undo of actions already executed; S/X is
still the explicit selected-group stop.

Contextual Interact orders carry a world-space destination, `EOKInteraction` identity
and `bApproachInteraction`; they navigate first and interact only after reaching
range. E/HUD-hand interaction remains immediate. Typed interactions cannot
accidentally activate a different nearby marker. Detonator
and hide destinations can be planned before TNT/pickup; prerequisites are checked
at execution, not inferred from the state at mouse-click time.
`bRunToDestination` applies pace once when a Move begins; ordinary orders retain
the current stance. `PromoteLastMoveToRun` upgrades a matching
last move on double-click, avoiding duplicate orders while paused/Shift-appending.
A queued interaction occurs after preceding movement. Failed paths end an order
with feedback rather than teleporting or crossing water.
Takedowns check range, approach direction, cooldown and obstruction at execution.
Body carrying reduces speed; hide interaction unregisters the hidden Sight stimulus.

Move and Stance apply to all selected units. Interact, Distract, Takedown and Carry
apply only to the active portrait, so group selection cannot spend two charges or
send two carriers to one body. Submit returns whether the order was accepted.
Invalid submissions leave the previous queue and movement intact. Navigational
orders are checked against complete agent-specific NavMesh routes before queue mutation.
Movement endpoints must stay within 30 cm horizontally of the requested goal;
approach interactions allow 75 cm and guard/body approaches 100 cm. All reject
projection to a floor more than 75 cm away vertically. River/interior/disconnected
goals therefore cannot replace an otherwise valid active path. Appended commands
query from the last queued approach/movement destination instead of current feet.
The route is checked again once when execution starts, not on every moving tick.
If a queued approach becomes unreachable, only that order is removed and later
orders remain available. UE path following owns active route invalidation/replanning.
Identical endpoints use bounded NavMesh point projection, preserving same-position
moves and takedown-then-pickup plans without admitting points in water. Double-click
run promotion also validates the new click before modifying an existing move.
Targets are validated again at execution; stale targets cancel their own approach path.
Friendly takedowns and carrying living/hidden/already-carried guards are rejected.
Both takedown and pickup check vertical reach and a Visibility trace at arrival;
the body cannot be attached through a wall just because it is inside AI MoveTo's
acceptance radius. These checks do not introduce automatic rear-approach routing.
A carry may be queued behind a takedown of the same guard, but still requires a
dead body when executed. Cooldowns retain the pending attack/throw; empty inventory,
invalid targets and blocked/out-of-range throws produce feedback and end the order.
Invalid armed targeting remains armed for correction; Escape/right-click cancels
only targeting, while S/X also stops the selected units. Right-clicking
HUD controls does not activate them, and menu/outcome screens block skill arming.

## Selection Input

`OKRTPartyMarkers` builds small numbered screen-space affordances for living,
nonhidden party members only. Draw and pick use the same current-view projection
and placement helper, with 32 px desktop / 44 px compact bounds and alternate
positions for overlapping allies. Full bounds avoid HUD panels/buttons, projected
character bodies and mission interaction points, preserving guard/body/objective
picking on narrow views. No marker
is emitted when the anchor is offscreen or no safe position fits. Markers are
suppressed in menu/outcome screens and by the persisted PartyMarkers preference.
They deliberately render above scenery without testing camera occlusion; this
does not alter collision, navigation or guard perception. No enemies use them.

Unarmed mouse release and touch taps select through a marker before terrain
raycasting; captured Shift appends selection. Right-click/double-click markers
never issue world commands. Armed left/touch targeting cannot consume an ability
on an ally marker; right-click still cancels targeting. Markers are separate from
blocking HUD panels, so selection rectangles keep their existing body-centre
semantics. Hover identifies the ally and clears inappropriate path previews.

Desktop input is Commandos-like stealth RTT, not adventure point-and-click.
Left-click selects a living ally or inspects a living enemy's cone; terrain left-click
does not move. Right-click orders movement, takedown of a living guard, pickup of a
dead guard or approach/interaction at available TNT, detonator and body-hiding markers.
Friendly right-click is ignored. Double right-click runs to a terrain destination
without repeating target interactions. Shift appends world orders and adds selection.
1/2 select living specialists, 3/Ctrl+A selects the living party and Tab cycles the
active specialist without dropping an existing group. Arrow keys and middle drag pan
the camera while running or paused; Alt+right drag is reserved for continuous orbit.
The first drag frame records the pointer before applying deltas, preventing camera jumps.
Hover cursor and preview distinguish navigable terrain, interactions and body targets.

The controller's BeginPointer/UpdatePointer/EndPointer handlers separate a mouse
click from a box selection with an 8-pixel threshold. Left mouse press and release
bindings execute while paused. HUD and armed-skill clicks are handled on press;
ordinary selection waits for release. `CommandAt` and `TapAt` route through the same
target resolver with explicit command/touch intents. Touch taps select allies,
inspect enemies and command terrain contextually, rather than pretending to be
desktop left clicks. Armed abilities retain their separate left/tap targeting path.

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

Supplied characters now have lightweight derived rigs and procedural locomotion.
Refine skin weights, add foot IK and authored prone/takedown/carry animations.
See CHARACTER_MOTION.md for the scale comparison and current animation limits.
Move the native guard FSM into optional Behavior Tree/Blackboard assets if
designer iteration requires it; do not connect the old turn-perception bridge.
Add authored large missions, persistence, proper objective components, audio/VFX,
weather-modified ranges, real terrain collision, touch camera gestures and
packaged/device validation before claiming production or cross-platform parity.
