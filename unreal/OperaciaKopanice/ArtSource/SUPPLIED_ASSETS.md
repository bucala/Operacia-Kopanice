# Supplied Blender assets

The original OneDrive files are read-only inputs. Prepared FBX meshes, extracted
textures and Blender review renders live in `ArtSource/Supplied`. The manifest
records original filenames, dimensions, polygon counts and material links.

## First Integrated Batch

- Cabin: Snowy_Wooden_Cabin_Di_0915151619, replaces the previous generated house.
  Footprint fitted to the existing 330 cm tactical obstruction; retained plinth.
- Car: classic_black_car_0915151432, 520 cm long, parked outside the movement grid.
  Visual scenery only, not a drivable or historically certified vehicle.
- Partisan: Slovak_Partisan_Figur_0915153436, derived mesh without display plinth
  and disconnected props. The reviewed source is cut at 25 cm after initial
  normalization, then its largest connected component is placed on the ground,
  scaled to 180 cm and oriented toward +X. Original UVs and textures are retained.
  Static figurine only: no skeleton, skin weights or locomotion animations yet.

All three use Nanite and their supplied base-color, roughness (green), metallic
(blue) and tangent normal maps. Normal texture green is flipped for UE.
The packed map's red channel is not assumed to contain ambient occlusion.
No external model or texture generation service is used.

## Reproduce

Run Blender in background with `--python ArtSource/prepare_supplied_assets.py --`
followed by the original asset directory and an absolute output directory
ending in `ArtSource/Supplied`. An optional third argument selects Cabin, Car or
Partisan. Then run the project's UnrealEditor-Cmd with `-run=pythonscript`
and `-script=<absolute path>/Content/Python/import_supplied_assets.py`.
Close the editor/game before reimporting assets. Rebuild C++ with Start-Demo.ps1
`-Build`; launch normally or use `-SmokeTest` for the mission regression test.

The 13 original files were structurally audited with `audit_blend_assets.py`.
Other character variants, the second cabin and command vehicle are not yet
integrated. The forest kit has subsequently been reviewed and split into two
conifers, a shrub and a boulder. See `WINTER_ENVIRONMENT.md` for that pass.
The imported cabin reports near-zero tangents in some source triangles; inspect
normal shading before considering these production-ready geometry.

Large binary assets are marked for Git LFS in the repository attributes.
No commit, upload or push is performed by these scripts. Install/use Git LFS
before staging the assets for a future push; several FBX files exceed 100 MB.
