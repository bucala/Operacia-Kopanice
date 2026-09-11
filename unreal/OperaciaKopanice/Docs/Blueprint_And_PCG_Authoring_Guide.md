# Blueprint and PCG Authoring Guide

## 1. Environment and Terrain Setup

### Landscape material

Create `M_OK_Landscape_Master`:

1. Add a `LandscapeLayerBlend` with weight-blended layers:
   - `Paved_Roads_Interiors`
   - `Autumn_Mud`
   - `Deep_Winter_Snow`
2. Create matching Landscape Layer Info assets:
   - `LI_Paved_Roads_Interiors`, physical material `PM_OK_Paved`, friction `1.0`
   - `LI_Autumn_Mud`, physical material `PM_OK_Mud`, friction `0.6`
   - `LI_Deep_Winter_Snow`, physical material `PM_OK_Snow`, friction `0.3`
3. In the master material, blend visual material functions:
   - Paved: compact asphalt, concrete, timber interior floor variants.
   - Mud: brown-black wet soil, puddle roughness breakup, wheel rut normal detail.
   - Snow: blue-white packed snow, wind crust noise, deep-snow height mask.
4. Add Runtime Virtual Texture sampling for footprints:
   - `RVT_OK_Footprint_Depth`
   - `RVT_OK_Footprint_Normal`
5. Use the RVT footprint mask to lower snow through World Position Offset or Nanite displacement. Keep actual collision simple and drive gameplay from `UOKFootprintTrackerSubsystem` plus nav area costs.

Blueprint hookup:

1. Create `DA_OK_TerrainSurfaceRuntime` from `UOKTerrainSurfaceDataAsset`.
2. Assign `PM_OK_Paved`, `PM_OK_Mud`, and `PM_OK_Snow` to the three entries.
3. Add `UOKTerrainMovementSurfaceComponent` to playable soldiers, vehicle pawns, and tracked AI pawns.
4. Bind `OnFootprintRequested`:
   - Mud: spawn a deferred decal with 18-25 second fade.
   - Snow: write into `RVT_OK_Footprint_Depth`, spawn compacted snow decal if desired, and call `RegisterFootprint` on `UOKFootprintTrackerSubsystem`.

### PCG: dense beech forests

Create `PCG_OK_Myjava_BeechForest`:

1. Input: Landscape surface from a PCG Volume.
2. Surface sampler: points every 600-900 cm.
3. Attribute filters:
   - Slope between 4 and 34 degrees.
   - Altitude between 280 and 900 m world-equivalent.
   - Exclude road/interior and settlement splines using Difference/Subtract.
   - Exclude 1200 cm around mission-critical combat arenas unless intentionally forested.
4. Density:
   - Multiply by large-scale noise for organic clumps.
   - Favor north/east-facing slopes for denser beech stands.
5. Static Mesh Spawner:
   - 6-10 European beech trunk/canopy variants built in UE Modeling Mode or imported later if allowed.
   - Uniform scale 0.75-1.45.
   - Align to normal, random yaw.
   - Enable Nanite on generated static meshes.
6. Secondary scatter:
   - leaf litter decals, saplings, fallen branches, deadwood.
   - Keep collision disabled on small dressing meshes.

### PCG: Vrsatec-style limestone rocks

Create `PCG_OK_Vrsatec_LimestoneRocks`:

1. Input: Landscape surface.
2. Filter slope 22-58 degrees and altitude 390-970.
3. Use ridgeline masks, erosion/noise, and spline keep-out around roads.
4. Spawn clustered limestone meshes with:
   - Scale 0.55-2.8.
   - Pitch/roll aligned to normal with random yaw.
   - Larger anchor rocks first, smaller debris second.
5. Use Nanite, high roughness material, world-aligned lichen/moss layer, and snow top-mask in winter levels.

## 2. Native UE5 Modular Architecture

### Drevenice and stone structures

Use `BP_OK_Drevenica_Template` as the editor assembly control:

1. Set `Style` to `DrevenicaLogCabin`, `CarpathianStonework`, or `MixedLogAndStone`.
2. Adjust `BayCountX`, `BayCountY`, `BaySize`, and `WallHeight`.
3. Click `RebuildTemplate`.
4. Convert the resulting blockout into higher-fidelity static meshes using Modeling Mode:
   - PolyModel > Bevel log edges.
   - Deform > Lattice or Bend for imperfect timber.
   - Sculpt/Displace for hand-hewn wood grain.
   - Mesh Boolean or Pattern for window/door cuts.
   - Bake to Static Mesh and enable Nanite for log/stone micro-geometry.
5. Keep gameplay collision on simplified proxy boxes, not the Nanite mesh.

Recommended modular kit pieces:

- `SM_OK_Drevenica_LogCourse_320`
- `SM_OK_Drevenica_NotchedCorner`
- `SM_OK_Drevenica_WindowSmall`
- `SM_OK_Drevenica_DoorPlank`
- `SM_OK_Drevenica_ShingleRoof_A`
- `SM_OK_StoneWall_Block_320`
- `SM_OK_StoneFoundation_Corner`
- `SM_OK_LimePlaster_DamagedPanel`

### Concrete modular tile platform

Use `BP_OK_ConcreteTilePlatform`:

1. Set `SideEngraving` to examples like `TATRA T77A / 1938` or `IFA W50`.
2. Set `RearEngraving` to mission, museum, or faction text.
3. Assign a concrete material with procedural aggregate, chipped edges, and dirt in bevel cavities.
4. Place vehicles or character showcase actors directly on top.
5. For true engraved geometry, select the generated actor, convert the cube to a static mesh, then use Modeling Mode text/boolean engraving. The Blueprint text remains useful for fast editor iteration.

## 3. Vehicles and Props

Create three Blueprint subclasses:

- `BP_OK_Kubelwagen_Typ82`
- `BP_OK_OpelBlitz_3T`
- `BP_OK_Tatra_T77A_1938`

For each:

1. Set `VehicleType`.
2. Assign a vehicle static mesh. The mesh can be created natively with Modeling Mode primitives for blockout, then refined with bevel, mirror, poly edit, and boolean tools.
3. Assign `M_OK_Vehicle_ProceduralWeathering`.
4. Call `ApplyVehiclePreset` in the editor.

Material `M_OK_Vehicle_ProceduralWeathering` should expose these scalar parameters, already driven by C++:

- `MudAmount`
- `FrostAmount`
- `RoofSnowAmount`
- `PaintWearAmount`
- `UseHeightSnowMask`

Material graph notes:

- Mud: world-position lower-body mask multiplied by noise and wheel arch cavity mask.
- Frost: glass-only material function using grazing-angle Fresnel plus blue-white noise.
- Roof snow: height/top normal mask, accumulated through WPO or Nanite displacement if mesh supports it.
- Paint wear: curvature-style mask approximated with baked vertex color or procedural edge noise.

## 4. AI Navigation and Physics

### Snow depth and nav cost

1. Add `NavModifierVolume` over deep snow pockets and assign `UOKSnowDepthNavArea`.
2. Set Recast NavMesh runtime generation to Dynamic for mission maps using changing snow/obstacles.
3. For fine-grained snow depth, keep visual deformation in RVT and route tactical penalty through:
   - terrain speed modifier on pawns;
   - `UOKSnowDepthNavArea` for pathfinding;
   - AI footprint queries through `UOKFootprintTrackerSubsystem`.

### Destroyed bridge obstacles: Mission 1 viaduct

1. Create `BP_OK_Mission1_ViaductObstacle`.
2. Add one or more `UOKDynamicNavObstacleComponent` boxes that cover collapsed bridge spans.
3. On bridge destruction:
   - enable collision on the obstacle component;
   - call `SetObstacleActive(true)`;
   - optionally spawn debris meshes and `NavModifierVolume` with high traversal cost around rubble.
4. If engineers repair or clear the bridge, call `SetObstacleActive(false)`.

### Turn Coordinator and AI Perception

Use the existing turn subsystem as the authoritative phase owner:

1. On `PlayerAction` events that should make sound, call `UOKTurnPerceptionBridgeSubsystem::ReportTurnSound`.
2. Add `UAIPerceptionComponent` with Sight and Hearing configs to enemy controllers.
3. Add `UOKWeatherPerceptionComponent` to the same controller or pawn.
4. When autumn fog or winter weather changes, call `ApplyWeatherState`.
5. On Enemy Reaction phase, query:
   - AI Perception for recent sight/hearing stimuli.
   - `UOKFootprintTrackerSubsystem::QueryDetectableFootprints` for snow tracks within patrol radius.
6. For 708th Volksgrenadier units:
   - fog reduces sight cones;
   - snowfall slightly muffles sound;
   - blizzard heavily reduces both;
   - deep snow tracks can trigger investigate behavior even without line of sight.

## 5. Native UE Asset Creation Checklist

- Build terrain first: Landscape sculpt, road splines, layer paint.
- Author landscape material and physical layer infos before tuning movement.
- Build PCG graphs with debug point visualizers enabled.
- Create a small test map with one hillside, one road, one cabin, one vehicle platform, and one enemy patrol.
- Verify:
   - movement speed changes across all three surfaces;
   - mud footprints fade;
   - snow footprints persist and are queryable by AI;
   - PCG forests avoid roads;
   - limestone clusters sit on slopes/ridges;
   - bridge destruction invalidates nav paths;
   - fog and snow update enemy sight/hearing ranges.

