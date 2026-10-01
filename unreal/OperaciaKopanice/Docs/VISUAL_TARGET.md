# Visual Target: Snowy Kopanice

Reference review: 2026-09-12. This is the art direction requested by the user,
not a statement that the current UE demo already meets it.

## References

Primary visual reference confirmed by the user on 2026-09-16:
`C:/Users/bucal/OneDrive/Obrázky/AI/Game/Gemini_Generated_Image_uhfr37uhfr37uhfr.png`.
This supplied image takes
precedence over the earlier website references for environment composition.
It is an art target, not a screenshot of the current implementation.

Specific cues: continuous snow terrain filling the frame, cobbled paths partly
covered with snow, irregular snow banks, clustered cabins and frosted vegetation,
small crates and timber details, cold daylight with soft blue shadows, realistic
human proportions, thin white ground grid and translucent red threat areas.
The dark portrait/action HUD has restrained brass trim. Do not reproduce the
concept's garbled captions or duplicated directional controls. Threat shapes
must continue to represent the actual tactical rules, not imply a new cone model.

Next environment milestone: replace the exposed board appearance with continuous
snow and integrated paths while preserving the existing mission coordinates,
click picking, sight blockers and undo/restart behavior. Imported detail meshes
alone do not meet this milestone.

- Playable predecessor: https://operacia-kopanice.replit.app/
- Target presentation: https://project-operacia-kopanice.magicpatterns.app/

Both pages were inspected in a browser, including the predecessor's Zacvik
game view and the target's hero and environment section. The reference site
distinguishes in-game sprites from concepts. Its hero is a visual target;
its appearance alone does not establish an implemented 3D game or reusable meshes.

## Required Direction

- Elevated three-quarter/isometric composition with a continuous snowy settlement.
  The current exposed rectangular board and black void are not the target.
- Weathered timber cabins with stone foundations, readable doors/windows,
  roof snow, fences, wood piles and paths integrated into the surroundings.
- Snow-covered trees, bare deciduous branches, irregular rocks and snow banks.
  Avoid sphere-on-cylinder trees as deliverable environment art.
- Recognizable human silhouettes: partisan coat, trousers, boots and equipment;
  differentiated guard silhouettes. Cylinders remain debug representations only.
- Cold daylight, soft shadows and restrained atmospheric depth. Maintain clear
  separation of interactive units from scenery without neon body colors.
- Subtle tactical nodes and paths over terrain. Threat overlays communicate the
  exact rules, not a decorative cone that implies a different detection shape.
- Compact dark HUD with restrained brass accents, portraits and recognizable
  action icons. Do not copy the reference website's navigation/sidebar into gameplay.

## First Art Slice

Keep the existing solvable TNT/viaduct mission and its undo/restart behavior.
Upgrade one playable area before expanding the map or adding vehicles:

1. Camera and continuous snowy ground with a legible crossing and river banks.
2. One finished cabin kit and a small vegetation/rock kit, with consistent scale.
3. One partisan and two readable guard representations replacing markers.
4. Ground-bound tactical overlays and a portrait/action HUD.
5. Lighting, contact shadows and visual checks at 1280x720 and 1920x1080.

Use Blender for custom geometry, silhouette refinement and UVs where needed;
use UE for material authoring, assembly, lighting, placement and gameplay.
Existing web sprites may serve as references, but must not be presented as 3D
models. Audit the local asset inventory and provenance before importing assets.

## Acceptance Checks

- Start and victory screenshots read as a snowy Slovak settlement, not a test board.
- All walkable nodes and objectives remain visible; scenery cannot hide hazards.
- Camera projection and cursor-to-ground picking agree throughout the play area.
- Overlay visibility exactly matches the deterministic rules; no silent rule changes.
- Mission solvability, action contracts, undo and restart tests remain green.
- Complete a manual keyboard and mouse playthrough in addition to the smoke test.
- Capture measured runtime performance on the target machine; do not infer it from
  Nanite being enabled or from static screenshots.

Historical vehicles and fully deformable snow are separate later slices, not
prerequisites for the first coherent environment. The site labels vehicles as
concepts; their presence there does not establish suitability for every mission.
