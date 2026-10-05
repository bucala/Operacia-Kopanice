# Character scale and real-time motion

All measurements are in Unreal centimetres, including hat and footwear.
The derived skeletal meshes retain the supplied UVs and existing materials.

| Unit | Source model | Standing height | Capsule diameter | Runtime polygons |
| --- | --- | --- | --- | --- |
| Partisan | Supplied Partisan | 180 cm | 60 cm | 28,025 |
| Officer | Supplied Officer | 180 cm | 60 cm | 28,007 |
| Guard 1 | Supplied Officer | 180 cm | 60 cm | 28,007 |
| Guard 2 | Supplied Officer | 180 cm | 60 cm | 28,007 |

The capsule is a navigation envelope, not the visible shoulder width.
Hat-inclusive height is intentionally identical for readability and fair scaling.
The original meshes contained 431,821 and 1,755,591 polygons respectively.
Original Blender/FBX sources remain in the external authoring archive.

`Tools/rig_characters.py` creates separate 14-bone skinned derivatives in Blender.
`Content/Python/import_animated_characters.py` imports them without copying textures.
`OKRTAnimation.cpp` drives the pose from continuous CharacterMovement velocity:
walk/run cadence, alternate leg and arm swing, knee flexion and smoothed stance
transitions. Tactical pause freezes the actor and its pose. No turns drive movement.

Pelvis offsets are applied in component space after hierarchical bone composition.
FBX local axes and parent unit scales must never multiply centimetre offsets.
Walking produces zero lateral pelvis offset and at most 0.6 cm vertical movement;
cadence is capped at 1.65 cycles per second and speed is filtered at starts/stops.
The runtime smoke fixture verifies both imported rigs, all four unit heights,
leg movement and the pelvis displacement bound over 120 simulated frames.

Verified on UE 5.8.2: the rendered desktop mission fixture passed 77 checks on
2026-10-02. Measured pelvis displacement was 0.0000 cm laterally and 0.5992 cm
vertically. All four unit heights measured 180.0 cm. This bounds the procedural
pose offset; it does not measure navigation avoidance or certify production gait.

These are procedural prototype rigs, not hand-authored production animation clips.
The officer coat has blended leg weights and can stretch at extreme poses.
Prone still uses an overall body tilt; takedown, death and carrying need dedicated
animations, foot IK and cloth authoring in a later animation pass.

## Environment and interface

`refine_surface_transitions.py` blends stone into snow using the same world-space
road segments as the terrain authoring script. Shoreline colour follows the carved
river centre, transitioning from dark moving water to lighter shallow water.
Wood receives sparse snow dusting from the same world-space noise as the ground.
Water is an animated shader, not a fluid simulation.

Menu rows accept clicks on both labels and icons. Restart is available in the
pause menu. Portrait counters have inventory/order icons; the HUD identifies
real-time play, tactical pause and the currently armed ability.
