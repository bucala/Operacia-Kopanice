# Asset Storage

The playable repository keeps the runtime Unreal assets required by the current
demo: the primary cabin, car, forest, officer, partisan, winter-ground and HUD
assets. Unreal build output and import caches are intentionally ignored because
`Build.bat` and `Start-Demo.ps1 -Build` recreate them.

The following optional material is kept outside Git so the repository remains a
manageable source checkout:

- `ArtSource/`: Blender, FBX, preview and import-source files.
- `Content/Kopanice/Supplied/CabinAlt/`: alternate high-resolution cabin.
- `Content/Kopanice/Supplied/CommandVan/`: optional command van prop.
- `Content/Kopanice/DetailedCabin/`: detailed-cabin fallback assets.
- Duplicate concept renders under `attached_assets/`: only the README logo,
  main gameplay reference and GDD are kept in the repository.

In the development workspace these files are preserved at the sibling folder
`external-assets/Operacia-Kopanice-Source-2026-10-02`. They are not required for
the default real-time mission: missing alternates fall back to the primary
cabin, and the command van is an optional environment prop.

The repository does not commit these generated folders:
`Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` and `node_modules/`.
They are local build products and can be regenerated after cloning.
