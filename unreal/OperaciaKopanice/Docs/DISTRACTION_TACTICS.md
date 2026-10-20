# Distraction Tactics

Main native Unreal game only. This is a playable RTT increment, not a port of
Commandos content and not an update to the separate browser adaptation.

## Play

- Select a specialist, press F, then LMB supported terrain to throw a stone.
  The preview is the sampled flight arc; the faint circle shows the maximum
  unobstructed impact-noise radius, not guaranteed audibility through walls.
- A stone has an 8 m horizontal throw limit and consumes one of five items on
  launch, with the existing six-second skill cooldown. Flight lasts 0.45-1.15 s.
  Guards hear the impact only after flight, at the actual impact location.
  Low cover may be cleared; tall walls and unsupported destinations reject the
  command. A newly introduced obstacle intercepts a flight without doing damage.
- Partisan: Y or the sound icon whistles at his current position. This attracts
  within 7 m, uses no stones and has a four-second shared skill cooldown. It can
  expose his location. The officer retains his separate medic specialization.
- Both actions require free hands. Shift queues a skill after a move. Z can
  combine a whistle with the other member's action; approach/cooldown barriers
  release synchronized actions, not synchronized stone impacts.
- Use the lure to turn or draw a mobile patrol away, then move the other member
  past its cone or approach from behind. Fixed sentries turn but keep their post.
  Sound does not downgrade a guard already in combat.
- Visible investigating guards show an amber line to their investigation point.
  This never reveals an unseen guard. Both skills, flights and cooldowns stop
  during tactical pause/menu. There is no automatic turn completion or pause.

## Runtime and Save Contract

`OKRTDistraction.cpp` owns the shared predicted arc, a small reused supplied rock
visual, swept 25 ms flight substeps and impact event. No rigid-body replication,
external asset, purchased pack or additional landscape geometry is required.
Stone collision is gameplay geometry, not a damage attack. Flights can continue
after the thrower dies; mission outcome freezes the simulation.

UE AI Hearing still delivers events. Each callback matches the source/tag/location
to its original noise pulse and tests that event's finite range. A blocking wall
reduces that range to 30%; other characters do not act as soundproof walls. This
is a deliberately simple obstruction model, not room/portal audio simulation.
Existing sight, casualty discovery and patrol rules remain authoritative.

Save schema 3 stores in-flight start/end, elapsed/duration and party index. Load
reconstructs the visual and continues the remaining flight without spending a
second item or replaying a launch/impact. Schemas 1 and 2 remain readable; newer
order values and flights are rejected in older schemas. Invalid/non-finite
flight state is rejected before changing the live world. F5/F9 slots are unchanged.

## Reference and Adaptation

The supplied Commandos 2 manual, printed pp. 22-23 and 36-37, describes finite-range
thrown distractions; pp. 30-31 distinguish mobile investigators from fixed posts.
Its whistle on p. 35 calls the dog, not guards. Our partisan whistle, stone
projectile, obstruction factors, ranges and cooldowns are original prototype
design choices. No original manual, art, audio or game data is redistributed.

The stone mesh uses the existing supplied forest-rock asset. The sound icon is
Lucide Volume2 under the repository's existing Lucide license. Generate raster
sources with `Tools/Generate-TacticalIcons.cjs`, then import only the new icon with
`Content/Python/import_distraction_hud.py`.

## Verification

`Start-Demo.ps1 -DistractionSmokeTest` renders the native mission and exercises
the actual Y/F9 input, arc/obstruction/ground validation, non-mutating preview,
delayed sound, finite and occluded hearing, pause, save/reload mid-flight,
mobile/fixed AI, team barriers, combat priority and new-obstacle interception.
Use `-Width 480 -Height 800` for the portrait layout regression. See
[recorded build and runs](REALTIME_VERIFICATION.md) for actual results.

Remaining limits: skill/throw animations are provisional, sound is represented
by the existing noise HUD and AI event rather than a newly authored whistle or
impact recording, and no item retrieval, remote decoy or room acoustics is added.
