# Winter Environment Pass

The demo uses a continuous authored static-mesh snow surface, a carved stream,
3,095 individual cobbles and four background cabins. This is not a Landscape
or runtime PCG graph. Tactical rules, cover cells and the common Z=0 picking plane
use the same world coordinates. The main cabin now reserves 4 x 4 cells;
exterior buildings and vegetation are scenery only.

## Rebuild Terrain

Run Blender in background with `--python ArtSource/build_winter_ground.py`.
Then run UnrealEditor-Cmd against the project with `-run=pythonscript` and the
absolute path to `Content/Python/import_winter_ground.py` in `-script=`.
Ground/cobbles deliberately use exact conventional static meshes; the supplied
high-resolution cabins and forest assets use Nanite. The terrain authoring script
pre-flips Y and winding to compensate for FBX handedness conversion into UE.

## Supplied Forest

`export_forest_assets.py` takes the original forest blend path and an output
directory after `--`. Use `ArtSource/Supplied/Forest` as the output directory.
The original input is opened but never saved. The extractor separates loose
components, retains the UV atlas, centers each selected asset and grounds its
pivot. It produces FBX, texture PNGs, derived Blender libraries and a manifest.
The Blender libraries can be appended; `review_blend_asset.py` also renders them.

Reviewed components from `Meshy_AI_Winter_Forest_Asset_C_0915163054_texture.blend`:

- Mesh_0.002: snowy conifer, 808,468 polygons, normalized to 480 cm tall.
- Mesh_0.007: second snowy conifer, 423,277 polygons, 380 cm tall.
- Mesh_0.013: snowy shrub, 588,027 polygons, 100 cm tall.
- Mesh_0.006: snow-covered boulder, 59,912 polygons, 100 cm tall.

Run `Content/Python/import_forest_assets.py` in the UE Python commandlet. All four
share one PBR atlas material. UE 5.8 uses `shape_preservation` with
`NaniteShapePreservation.PRESERVE_AREA`, not the older `preserve_area` boolean.
The selected conifers are not claimed to be European beech trees.

## Verification

Run `Content/Python/validate_winter_environment.py` in a fresh UE process to check
persisted materials, dimensions, terrain alignment, texture color spaces and
Nanite settings. Then run `Start-Demo.ps1 -SmokeTest` and repeat with
`-Width 1920 -Height 1080`. The smoke test checks the full mission, undo/restart,
threat overlay state and projection/deprojection of every walkable cell. The
launcher now checks a unique run log and rejects material compilation failures.
These automated checks do not replace a manual mouse/keyboard playthrough.

The grid material has separate GridVisibility and Fill parameters so hiding lines
never hides threats. M_Water uses world coordinates and Time for flowing ripples
and tangent-space normals. All custom-node inputs must be connected again after
changing the input array; otherwise the material falls back to the default shader.

Remaining art work includes better snow blending over road edges, richer cabin
variations, deciduous forest species, character animation, richer HUD artwork and final
lighting. This pass is progress toward the supplied concept, not parity with it.
