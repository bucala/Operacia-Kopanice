# Asset Storage

The playable repository keeps the runtime Unreal assets required by the current
demo: the primary cabin, car, forest, officer, partisan, winter-ground and HUD
assets. Budgeted CabinAlt and CommandVan derivatives are now required by the
additional native campaign missions and are included through Git LFS. Their ten
runtime packages add approximately 32 MB, using 1024px textures rather than the
original high-resolution source payload. Unreal build output and import caches are intentionally ignored because
`Build.bat` and `Start-Demo.ps1 -Build` recreate them.

The following optional material is kept outside Git so the repository remains a
manageable source checkout:

- `ArtSource/`: Blender, FBX, preview and import-source files.
- `Content/Kopanice/DetailedCabin/`: detailed-cabin fallback assets.
- Duplicate concept renders under `attached_assets/`: only the README logo,
  main gameplay reference and GDD are kept in the repository.

In the development workspace these files are preserved at the sibling folder
`external-assets/Operacia-Kopanice-Source-2026-10-02`. They are not required for
the original real-time mission. The new campaign uses the smaller committed
derivatives; the high-resolution originals remain outside Git.

The repository does not commit these generated folders:
`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` and `node_modules/`.
They are local build products and can be regenerated after cloning.

On 2026-10-07 the upload checkout's tracked/staged runtime, source and documentation
payload measured approximately 688 MiB. This excludes `.git` history/LFS cache,
ignored build products and high-resolution external originals; it is not the
size of a packaged Windows game. Git LFS downloads the current asset versions.
