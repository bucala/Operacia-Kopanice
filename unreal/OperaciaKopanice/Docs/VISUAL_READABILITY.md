# Winter Readability and Tactical UI

Main native Unreal version, 2026-10-20. The browser adaptation is unchanged.

## Lighting and PBR

The derived character rigs retain WorldGridMaterial in their asset slots. The
runtime now explicitly binds the canonical Partisan/Officer material to every
rendered slot (players and guards), with a rendered regression asserting this.
No large skeletal mesh reimport/upload is needed. The read-only
`inspect_readability.py` reports both source parameters and underlying rig slots;
component-level overrides are checked in the lighting fixture.

The source officer atlas has median RGB near 53/255 and the cabin near 57/255.
Packed metallic channels are already close to zero. Dark albedo, ambient-light
balance and dense near-camera fog combined to obscure form; this was not an
observed all-metal import error.

- One shadow-casting movable sun retains a two-degree source angle and virtual
  shadows/CSM fallback. No shadowless directional fill or emissive lift is added.
- A specified built-in DaylightAmbientCubemap replaces realtime sky capture.
  Paused startup and low quality receive a reproducible probe; its intensity is
  scene calibration, not a physical lux measurement. Snow-reflected lower light
  and bounded occlusion retain depth. AO is 0.2 at 35 cm radius.
- Near-camera/secondary fog is reduced. Fixed exposure and zero motion blur remain.
- Four small material packages expose linear albedo gamma/gain, normal strength
  and a roughness lower bound. Packed metallic B is intact, AO is 1, emission is 0.
  Original textures, UVs and rigs are not reimported; runtime material binding is explicit.
- A subtle world-space snow fringe joins cabin plinths/boots to the ground.
  GroundSnowHeight defaults to 18 cm for the current flat mission datum. This is
  not an RVT or a general landscape-height solution for future hillsides.
- Mission 1 visual cobbles use 0.45 vertical scale to sit in snow. Navigation and
  collision remain on existing proxies; object footprints are unchanged.

`Content/Python/refine_readability_materials.py` rebuilds only Cabin, CabinAlt,
Officer and Partisan materials from existing textures. Re-run it after reimporting
those materials. It does not modify the source bitmaps. Parameters are AlbedoGamma,
AlbedoGain, NormalStrength, RoughnessMin, DielectricSpecular, GroundSnowHeight and
GroundSnowAmount.

## Interface

Desktop party frames are horizontal, 216 x 110 px, exposing stance, health,
inventory, queue and readiness. The narrow layout retains its compact footprint.
The action dock has consistent hover/active/disabled states and protected gaps:
clicking between buttons cannot issue movement. Unavailable carrying skills stay
visible but disabled. Objective contrast, health green/red and selection gold
communicate real-time state, never turns.

Menus retain tested commands and IDs, use the live scene under a dimmer backdrop,
clear selection edges, visible disabled loading and switch-like binary options.
Message wrapping uses line spacing that fits compact feedback panels.

## Verification and Limits

`Start-Demo.ps1 -LightingSmokeTest` captures all four qualities and zoom/orbit.
Compare identical Q2 close views, not high quality against low resolution. Check
shadowed cloth/wood and snow clipping: settings-only PASS is insufficient.
Local comparisons live in `Saved/VisualReview-20261020`, not the upload.
See [recorded runs](REALTIME_VERIFICATION.md) for results and residual DX12 risk.
This remains a prototype, not production animation, fully blended terrain or AAA parity.
