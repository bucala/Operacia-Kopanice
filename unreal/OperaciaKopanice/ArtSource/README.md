# Detailed asset pipeline

The user's `OneDrive/Obrazky/AI/Game` folder contains image references, not FBX,
GLB or Blender meshes. Those images remain untouched. Geometry here is newly
authored; unseen sides and construction details are interpretations, not recovered
geometry from the images.

## Cabin, first detailed asset

Reference: `house1.png`. Implemented: individual rounded timber courses, masonry,
door/window openings, glazing and frames, roof boards and rafters, missing boards,
irregular thick snow with exposed roof patches, and hollow chimney.

- `build_cabin.py`: reproducible Blender 5.1 generator (meters, ground-center origin).
- `Generated/CabinDetailed.blend`: editable source geometry and UVs.
- `Generated/SM_OK_CabinDetailed.fbx`: export with smoothing information.
- `../Content/Python/import_detailed_cabin.py`: import into `/Game/Kopanice/DetailedCabin`,
  enable Nanite, assign UE procedural materials and save the assets.

Generate with Blender background mode:

```powershell
& 'C:/Program Files/Blender Foundation/Blender/blender.exe' -b --python ./ArtSource/build_cabin.py -- ./ArtSource/Generated
```

Then execute `Content/Python/import_detailed_cabin.py` using UE's Execute Python
Script action, or UnrealEditor-Cmd with `-run=pythonscript -script=<absolute script path>`.
Import regenerates only this dedicated generated asset folder's matching assets;
do not hand-edit those generated materials without duplicating them elsewhere.

The demo loads the imported mesh if present and fits its XY bounds to the existing
blocked cabin footprint. The old native template remains a fallback. Add
`-OKCabinReview` to the game command line for a closer inspection camera.
The local launcher also accepts `./Start-Demo.ps1 -CabinReview`. Combine with
`-SmokeTest` for an automated run that writes `Saved/CabinReviewStart.png` and
`Saved/CabinReviewWin.png` without replacing the normal gameplay screenshots.

Revision 2026-09-15: window trim moved to the exterior face on every wall;
side openings matched to frame width; irregular roof-snow damage edges rounded;
review camera pulled back to include the whole asset. Blender export contains
83,392 vertices and 80,498 polygons before FBX triangulation. The reimport completed
with 11 material slots and no importer errors or warnings.

## Remaining work

This is a first reference-based cabin, not a photorealistic reproduction of every
asset. Wood weathering is procedural, not a baked photographic texture set.
Character anatomy, faces, clothing folds, rigging, trees and vehicles still need
separate detailed assets. Do not label the existing primitive figurines as finished
models comparable to the supplied player/guard references.
