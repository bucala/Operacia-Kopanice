# Nonlethal Stealth Contract

## Reference and Own Rules

This slice changes only the main native Unreal real-time game. The owner's
Commandos 2 PC Manual describes hit/tie/gag and liberation by another soldier
on printed pp. 42-43 (PDF page 23); carrying only dead or bound enemies is on
printed pp. 22-23 (PDF page 13). Recovery of unbound enemies is discussed with
difficulty on printed pp. 4-5. Manuals, their illustrations and Commandos assets
are not redistributed.

Kopanice uses its own controls, timings and shared prototype specialist skills,
not the complete original roster. K arms knockout; G arms binding; T retains
the explicit lethal rear attack. Both specialists can use these abilities.
45 seconds of simulation time is the authored recovery interval, not a number
claimed from the manual. No automatic rear-routing is introduced.

## Live Unit State

Health remains authoritative for life/death. An unconscious guard keeps health,
but is not operational: no locomotion, animation update, sight, hearing reaction,
combat or patrol. The grounded pose shares the existing incapacitated-body
presentation; there is not yet an authored punch, tying or carrying animation.
The imported upright pose and ground offset initialize before any simulation
tick; paused startup does not depend on one later movement tick to place a rig.

| Action/state | Execution contract |
| --- | --- |
| Knockout | Living operational enemy, free hands, complete approach, reach, vertical/Visibility clearance, rear angle and attacker cooldown |
| Unbound unconscious | Recovers after 45 simulation seconds; true pause and menu stop the clock |
| Bind | Living unconscious unbound enemy on the ground, free hands, complete approach and physical reach |
| Bound | Never self-recovers; recovery clock is zero |
| Carry | Dead or unconscious bound enemy only; slows carrier to at most 100 cm/s, disables running and its noise |
| Drop/hide | Existing B/drop and typed HideBody interaction; hidden casualty loses collision and perception stimulus |
| Wake/free | Restores standing capsule, walking, upright visual and investigation at its own location; refuses hidden/carried/dead enemies and obstructed standing space |
| Death | Clears unconscious/bound/recovery state; existing corpse behavior remains |

LMB a conscious guard cannot attack automatically. LMB an unbound unconscious
guard resolves binding; LMB a dead/bound guard resolves carrying. Explicit tools,
HUD icons, hover paths, queue display and synchronized planning use typed orders.
Inspection/cone toggles never revive a guard or disable authoritative perception.
The recovery timer/bound icon appears only when a party member actually sees
the casualty; hidden/carried enemies are excluded from party sight and minimap.

A knockout -> bind -> carry sequence can be queued for one target during
tactical pause or Shift append. Queued prerequisites are rechecked on execution:
if a casualty recovers, dies, is hidden or changes carrier, stale actions fail
without attaching through a wall or silently attacking again. A synchronized
plan still contains one independent action per member and rejects conflicting
access to the same target before changing either ordinary queue.

## Guard Assistance

A mobile patrol remembers a newly seen unconscious/bound ally, investigates its
position and attempts a complete agent-specific NavMesh approach. Liberation
requires actual arrival within 145 cm, vertical clearance, an unobstructed
Visibility trace and space for the restored standing capsule. It is currently
instant at valid arrival, not a timed assistance animation.

A fixed sentry observes/investigates without leaving its post. Confirmed combat
has priority over body assistance; a new sound/investigation can interrupt an
assistance goal. Hidden/carried casualties cannot be discovered or freed.
Discovery memory clears when an enemy becomes operational again, allowing
another later knockout to be discovered. This is not barracks reinforcements
or a full scheduling/medical AI system.

## Save Compatibility

Save schema 2 appends order enum values and adds unconscious/bound flags,
recovery time and a guard assistance target stored as a stable enemy index.
Schema 1 remains readable with original order values and default new fields.
The existing quicksave slot name is intentionally retained for migration.
Older executables do not support schema 2 saves.

Validation precedes world mutation and rejects invalid indices, non-finite
clocks, bound-conscious/dead-unconscious states, invalid carriers/unbound living cargo
and hidden conscious enemies. Load rebuilds navigation, restores incapacitated
poses and body ownership, and resumes or freezes recovery according to saved
pause state. Snapshot tests and actual F9 level reconstruction are separate.

## Verification

```powershell
./Start-Demo.ps1 -NonlethalSmokeTest -Width 1920 -Height 1080
./Start-Demo.ps1 -NonlethalSmokeTest -Width 480 -Height 800
```

The fixture uses controlled positions and explicit short recovery clocks to
isolate mechanics. It also executes a real-time queued chain and a mobile
NavMesh assistance approach, injects actual K/G/F9 bindings, checks HUD bounds,
round-trips the isolated automation save slot and removes it after success.
It is not an unassisted mission playthrough or packaged/device certification.
Final run evidence and unresolved renderer limits are recorded in
[verification](REALTIME_VERIFICATION.md).
