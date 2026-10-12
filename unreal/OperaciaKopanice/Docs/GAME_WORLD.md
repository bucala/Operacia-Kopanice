# Kopanice: Native Mission World Contract

## Scope and Reference Decisions

This is the main Unreal real-time game, not the legacy grid demo or the Canvas
adaptation. The design below is both an implementation contract and a boundary
for further work. Existing scenery/assets remain; no Commandos assets or scripts
are copied, and proprietary MIS syntax is not imported.

Studied references supplied by the owner:

- Commandos 2 PC Manual: printed pp. 14-17 (selection, groups, movement),
  pp. 22-23 (context actions), pp. 30-31 (guards, perception, alarm).
- Commandos: Behind Enemy Lines manual: printed pp. 10-11 (selection/movement),
  pp. 14-17 (sight, enemies, hearing), pp. 28-29 (keys).
- [Mission overview](https://commandosmodding.fandom.com/wiki/Mission) and its
  [mission-data section](https://commandosmodding.fandom.com/wiki/Commandos/Mission):
  separate the map, objectives, world objects and scripted events.
  Direct access was blocked; indexed page extracts were consulted.

The originals differ: BEL selects with LMB; Commandos 2 selects with RMB.
Kopanice deliberately adopts the Commandos 2 mouse model. Space/P remain tactical
pause, unlike Commandos 2's Space stance shortcut; C/V handle low stances.
This is our own documented RTT system, not an exact reproduction of every original rule.

## Input Contract

| Intent | Desktop input | World mutation |
| --- | --- | --- |
| Select specialist | RMB ally/numbered marker, portrait, 1/2 | Selection only; no stop or path restart |
| Toggle group membership | Ctrl + RMB/portrait/1/2 | Add/remove; pick an already selected active member after removal |
| Select area | RMB drag; Ctrl adds | Living party capsule centres only; an empty box preserves selection |
| Move | LMB walkable terrain | Complete NavMesh destination in current stance |
| Run | Double LMB terrain | Promote matching move, never duplicate a paused order |
| Queue | Shift + LMB, or LMB during tactical pause | Append bounded per-unit orders |
| Inspect guard | RMB living enemy | Toggle visual cone only; detection continues independently |
| Silent attack | Arm T/swords, then LMB guard | Active specialist only; rear approach/reach/cooldown checked at execution |
| Interact | LMB objective, or arm E/hand then LMB objective | Typed approach; prerequisites checked on arrival |
| Cancel tool | RMB or Escape | Disarm only; never clear queued movement |
| Stop | S/X | Explicitly cancel selected units' plans |
| Pause | Space/P | Freeze simulation, preserve camera and command input |

LMB a living enemy without an armed attack cannot initiate combat. LMB allies
or their world markers cannot turn into terrain movement. RMB never activates
HUD buttons; LMB portraits/menu controls consume their click. Alt+RMB is reserved
for continuous camera orbit and cannot begin world selection.
Touch remains a separate tap-selection/context-order path; it is not a fake RMB.
UE emits a pressed event together with a double-click event. The controller
dispatches that second click once and only promotes a matching terrain move;
skills and objective interactions cannot replay through double-click promotion.

## World Layers

1. Presentation: supplied cabins, foliage, characters, cars, snow/river meshes,
   materials, portraits and procedural animation. A visible asset is not itself
   a promise of navigation, cover, interior access or an inventory interaction.
2. Physical world: world-space centimetres; CharacterMovement and agent-specific
   NavMesh. Banks, span, cabin boxes, tree trunks and rock proxies define routes
   and Visibility obstruction. Water and destroyed span are not traversable.
3. Tactical world: living units, stances, noise, LoS, cover volumes, bodies,
   awareness and immutable authored guard duties/routes. No turn/cell occupancy.
4. Mission world: immutable `FOKRTMission` definitions and live GameMode state;
   typed objective interactions drive event effects and all-member extraction.

`FOKRTMissionState` is computed from actors and objective flags on demand. It is
not a second mutable save/state store. `OKMissions::Stage`, `CanExecute` and
`CanExtract` share the progress rules between HUD, focus, interaction and victory.
Presentation changes cannot silently unlock a mission objective.

## Implemented Operations

| Operation | Required chain | Population / duty |
| --- | --- | --- |
| Zimny viadukt | Acquire TNT -> both capsules clear east bank -> destroy bridge -> both extract | Two mobile patrols; west rest 12 s north, bridge watch 2 s east, north turn before return |
| Lesny kurier | Acquire documents -> both extract | Two patrols and one fixed sentry |
| Tiche velitelstvo | Acquire TNT -> sabotage command vehicle -> both extract | Two patrols and one fixed sentry |

All members must survive; an empty team or partial extraction cannot win.
Wrong-operation actions cannot collect an item or trigger sabotage. Repeated
completed target actions cannot repeat their world effect. Mission objectives can
be queued before prerequisites, but an early arrival reports failure and ends
that order. There is no hidden turn that unlocks the next stage.

The mission definition now owns each guard's spawn, facing, duty and timed route.
Scene assembly consumes that data; forest placement reserves those watch points.
Mission 1 geometry destruction still removes bridge collision/navigation and emits
an explosion event. The command-post target emits noise but does not yet replace
its mesh with a wreck. Documents are an objective state, not a transferable item.

## Awareness and Patrol Contract

- Patrol: reach a stop using NavMesh, finish a visible yaw turn, then count watch
  time. A failed path is not arrival. Routes loop independently of player clicks.
- Investigate: mobile patrol approaches a heard/seen disturbance and searches,
  then returns to its pending route. A fixed sentry turns toward the disturbance
  without leaving its post. A newly seen body can trigger investigation once.
- Combat: confirmed sight/nearby alarm; last-known-position search after lost
  sight. Mobile patrols pursue; fixed sentries remain at their post, and neither
  can shoot outside the configured firing range. Alerts propagate locally.
- Far sight: standing builds suspicion; crouching is slower. Crawling has zero
  visual detection rate here, but still emits small movement noise.
- Near sight: all stances can be detected; exposed standing confirms immediately.
  Cabins/rocks/trunks occlude both zones. Low cover reduces nonzero detection.
- Display: cone visibility is inspection only and never disables the guard brain.
  Hearing, bodies and combat do not depend on whether the cone is drawn.

## Execution and Event Ownership

PlayerController resolves an intent. GameMode assigns group movement/stance or
one active specialist's skill. Unit validates before changing its queue, navigates
through UE path following and rechecks dynamic targets on arrival. GameMode alone
owns objective side effects, noise dispatch, sabotage and the mission result.
The guard FSM observes the live physical/tactical world, not mission HUD text.
Menus freeze the world separately and restore its previous tactical pause state.

The original legacy turn subsystem is not part of this chain. No new scripting
engine, parallel rule simulator or dependency was introduced for this contract.

## Not Yet Implemented

Disguise, tied/unconscious enemies and recovery,
usable vehicles, interiors/floors, ladders, swimming, barracks reinforcements,
flashlights, real acoustic occlusion, weather-driven perception, arbitrary scripted
objectives need separate tested implementation.
Officer-only medkits and live mission save/load are implemented in the modernization
slice below. Takedown/distraction remain shared prototype abilities; portraits do
not imply a complete specialist skill tree. Terrain still has simplified collision
and animations are procedural.
Native editor-hosted testing is not Windows shipping/APK/web parity certification.

## Authoring and Verification

Add/change a definition in `OKRTMission.cpp`; reserve complete reachable routes,
objective approaches, entry and extraction in scene assembly. A HoldPost guard
uses one authored watch stop. Keep render meshes separate from tactical collision.
Adding assets alone does not create a new mechanic.

`OKRTWorldSmoke.cpp` checks mission chains, casualty/partial extraction rejection,
authored population/duty, far crawling immunity, near exposure, occlusion and
sentry/mobile disturbance response. Controls, original integration, live-AI
bridge playthrough and both campaign playthroughs remain independent fixtures.
See [verification](REALTIME_VERIFICATION.md) for actual run evidence and limitations.

## Origins-Inspired Modernization

The [official Origins FAQ](https://www.kalypsomedia.com/post/commandos-origins-faq)
informs coordinated team actions and specialist roles. This remains an original
Kopanice prototype, not a reproduction of Origins or a claim of equivalent scope.

- Command Mode owns separate one-action-per-member drafts. Cancelling editing
  preserves normal queues. Commit validates all participating orders before any
  queue is replaced. A batch then approaches and waits at a readiness barrier;
  cooldown, reach, rear angle, line of sight and typed mission prerequisites still
  apply. After release, actions execute in the real-time frame loop.
- A lost target/path or explicit stop cancels an unreleased batch for the whole
  participating team. One batch can be active at a time. Completed actions are
  not rolled back after release. The system is not a future-outcome simulator.
- SaveGame schema 1 stores stable actor indices, not transient UObject pointers.
  Validation runs before world mutation; loading rebuilds scene/topology and
  navigation before restoring saved pause. Ordinary paths restart toward saved
  destinations, rather than serializing UE path-following internals.
- The desktop map is a schematic, not a navigation proof or omniscient radar.
  Enemy dots require current actual party LoS. Clicks focus the camera only.
- Cabin perimeter shrubs/rocks/crates reuse existing meshes as non-blocking HISM
  dressing. They do not introduce implicit cover. Existing tactical proxies keep
  owning cover/navigation. Snow normals use derivative-filtered detail to reduce
  distant shimmer; masks for existing river/road transitions remain unchanged.

Multiplayer/co-op, a six-specialist roster, disguise, multi-step synchronized
timelines, fully authored combat/carry animations and AAA-quality terrain/assets
remain future work. The current menu/save systems are editor-hosted Windows
features, not a packaged product or web/Android parity promise.
