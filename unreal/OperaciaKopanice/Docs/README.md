# Operacia Kopanice Native UE 5.4 Environment Systems

This package is a drop-in C++ and Blueprint-authoring scaffold for building the Myjava / White Carpathians environment natively in Unreal Engine 5.4 with Modeling Mode, PCG, Nanite, Lumen, Landscape materials, and Blueprint-authored assembly.

## What is included

- Terrain surface runtime data for paved/interior, autumn mud, and deep winter snow layers.
- Character movement surface sampling with friction and speed modifiers.
- Footprint tracking for mud/snow, including persistent snow tracks queryable by AI.
- PCG scatter rule data for European beech forests and Vrsatec-style limestone formations.
- Editor-spawnable modular building and concrete display platform actors.
- Historical vehicle actor presets with procedural weathering material parameters.
- Weather-aware AI Perception component for fog/snow sight and hearing attenuation.
- Turn/perception bridge for feeding turn-phase sounds into UE AI Hearing.
- Dynamic nav obstacle component and snow-cost nav area for destroyed bridges and deep snow.

## Install

1. Copy `Source/OperaciaKopanice` into the root of the Unreal project.
2. If the existing game module already uses the name `OperaciaKopanice`, merge these files into that module instead of replacing the module entry point.
3. Add dependencies from `OperaciaKopanice.Build.cs` to the existing module if merging.
4. Enable these UE plugins:
   - Modeling Tools Editor Mode
   - Procedural Content Generation Framework
   - Landmass, if using Blueprint brushes for terrain shaping
   - Water, if riverbeds or snow bridges need water/ice integration
5. Regenerate project files, build the editor target, then create Blueprint subclasses from the provided C++ classes.

## Suggested Blueprint Assets

- `DA_OK_TerrainSurfaceRuntime` from `UOKTerrainSurfaceDataAsset`
- `DA_OK_PCG_MyjavaScatterRules` from `UOKPCGScatterRuleSet`
- `BP_OK_Drevenica_Template` from `AOKModularBuildingTemplateActor`
- `BP_OK_ConcreteTilePlatform` from `AOKConcreteTilePlatformActor`
- `BP_OK_Kubelwagen_Typ82`, `BP_OK_OpelBlitz_3T`, `BP_OK_Tatra_T77A` from `AOKHistoricalVehicleActor`
- `BP_OK_Mission1_DestroyedBridgeObstacle` with `UOKDynamicNavObstacleComponent`
- Enemy controller or pawn component: `UOKWeatherPerceptionComponent`

## Terrain Surface Defaults

| Surface | Friction | Speed Modifier | Footprints | AI Detectable |
| --- | ---: | ---: | --- | --- |
| Paved roads / interiors | 1.0 | 1.0 | None | No |
| Autumn mud | 0.6 | 0.5 | Temporary decals | No |
| Deep winter snow | 0.3 | 0.285 | Persistent deep tracks | Yes |

The physical material should be assigned through each Landscape Layer Info asset, not through a single global material slot. The movement component line-traces down, reads the physical material, then applies the runtime values.

