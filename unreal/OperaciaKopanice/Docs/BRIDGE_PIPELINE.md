# Rustbound Bridge and Winter Lighting

The 2026-10-20 [readability pass](VISUAL_READABILITY.md) supersedes the lighting
balance below with a specified daylight probe and calibrated PBR materials.
Bridge geometry, scale, collision and destruction rules remain unchanged.

## Delivered Asset

Mission 1 now uses the project owner's supplied
`Meshy_AI_Rustbound_Overpass_1012173027_texture.blend`, not the cube-built bridge.
The original file is never saved or overwritten. Its SHA-256 is
`E9109CFA453C8EA76C1F03E8F825B221DFC0726E262B10A210B9175E1A2BE099`.
The original, prepared FBX files and PNG import sources remain local/ignored;
only seven playable Unreal packages are distributed through Git LFS.
These packages total 14,232,951 bytes (approximately 13.6 MiB).

| Part | Triangles before Nanite clustering | Purpose |
| --- | --- | --- |
| SM_OK_BridgeSpan | 110,900 | Concrete deck and railing, hidden after sabotage |
| SM_OK_BridgeSupports | 10,640 | Permanent lower piers |
| SM_OK_BridgeRubble | 102,600 | Cut/tilted original geometry, shown after sabotage |

Horizontal proportions and UVs are retained. The resulting footprint is
459.87 x 260 cm; only the above-deck railing is adjusted to 90 cm.
The local pivot is the walking surface, placed at world Z=-8 cm, matching
both bank surfaces. The original 280 x 220 cm river-gap collision proxy remains
responsible for the crossing. The deck's outer overlaps rest on the existing
banks, not on a larger newly navigable river region. Sabotage removes the proxy
and enables the existing navigation blocker; the full-party safety gate is unchanged.
Imported render meshes do not independently generate collision/navigation.

The shared bridge actor retains its blockout method for legacy/editor fallback.
The main native game selects `BuildSuppliedModel()`. Permanent supports remain
while the span is replaced by pre-authored fractured geometry. This is a state swap,
not Chaos destruction or a simulated debris fall.

## Rebuild

Use background Blender 5.1.2 and the supplied source file:

```powershell
& $Blender --background --factory-startup --python "$Project/Tools/prepare_bridge.py" -- $Source "$Project/ArtSource/Supplied/Bridge"
& $EditorCmd "$Project/OperaciaKopanice.uproject" -run=pythonscript "-script=$Project/Content/Python/import_bridge.py" -unattended -nop4 -nullrhi
& $EditorCmd "$Project/OperaciaKopanice.uproject" -run=pythonscript "-script=$Project/Content/Python/validate_bridge.py" -unattended -nop4 -nullrhi
```

`--inspect` reports source geometry/deck sections without exporting;
`--inspect --preview` also renders a Blender diagnostic image into the chosen
output folder. Export uses centimetres, FBX face smoothing and separate meshes.
The original has 119,580 triangles and three packed 2048px textures. Runtime
derivatives use 1024px textures, sRGB base color, linear packed roughness/metalness,
and DirectX-convention tangent normals. Nanite is enabled on all three meshes.
`M_Bridge` adds sparse upward-facing snow with native world-space material
masking; it retains the concrete/rust atlas and has no emissive lighting.
Every material graph connection is checked before saving.

Source-model redistribution rights remain those supplied by the project owner;
the code license does not grant additional rights to external models.

## Lighting

All three native missions now share one movable, shadow-casting sun (2-degree
source angle), live atmosphere capture and a cool nonblack lower hemisphere
approximating snow-reflected ambient light. The old shadowless directional fill
is removed. Skylight occlusion remains enabled; short-range AO is restrained
rather than disabling shadows or adding emissive character materials.
VSM remains the configured desktop shadow method, with a CSM distance/cascade
fallback on the sun. The lowest quality preset intentionally disables dynamic
shadows according to Unreal scalability; Medium/High/Epic retain them.

The tactical camera uses fixed manual exposure, neutral contrast/gamma and no
motion blur. Looking at bright snow, a dark facade or a zoomed/paused scene no
longer drives histogram exposure adaptation. Clothing/wood textures are unchanged:
diagnostic exports confirm sRGB base color, linear masks and predominantly
nonmetallic surfaces.

This is an authored winter lighting balance, not a calibrated photometric
daylight simulation or a new gameplay light-visibility mechanic.
See Epic's [Sky Lights documentation](https://dev.epicgames.com/documentation/unreal-engine/sky-lights-in-unreal-engine)
for atmosphere capture and lower-hemisphere controls.
Run `Start-Demo.ps1 -LightingSmokeTest -Width 1920 -Height 1080` for paused
overview/close/rotated renders across four quality levels and a detailed bridge view.
See [verification](REALTIME_VERIFICATION.md) for actual results and remaining limits.
