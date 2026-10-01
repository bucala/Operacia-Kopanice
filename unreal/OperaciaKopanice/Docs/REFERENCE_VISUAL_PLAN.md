# Main Gameplay Reference: Visual Rebuild

The user supplied Main_gameplay.png as the target on 2026-09-26. This document
describes implementation priorities; the reference is an art target, not a
claim that the current demo has reached its quality.

## Visual Analysis

The reference uses a close three-quarter camera, blue winter shadow, soft
sunlight and a continuous snow-covered village. Large irregular paving stones
form readable lanes. Snow fades onto their edges. Cabins and forest clusters
frame the central movement area. Wooden paths, boxes and smaller props supply
human scale. A fine pale grid stays subordinate to terrain and red threat areas.
Portraits and a compact objective occupy corners, leaving most of the village
uncovered. Character silhouettes and clothing distinguish roles.

The previous demo had flat snow, small sparse stones, five repeated cabins,
shared character geometry and a full-width operational HUD. The first rebuild
addresses terrain readability, supplied asset variety and lighting before the HUD.

## First Art Pass

- Wider paths with 1810 staggered, bevelled stones, about 34-38 by 24-28 cm.
  Shallow road depressions and uneven edges join them to the snow surface.
- Procedural snow color at broad and fine scales, subtle normal detail,
  softer sunlight and lower unshadowed fill. River flow and all crossings remain.
- Five low wooden path sections, authored as 35 individual planks.
- Twelve supply crates frame building entrances; TNT uses the same new wooden
  crate mesh instead of a yellow block. These props use procedural aged wood.
- Two background cabins use the supplied Winterwood model, including its trees
  and fence. All five cabin envelopes remain 4 x 4 cells, with uniform scaling.
- Additional forest clusters use the existing supplied fir meshes.
- The supplied command caravan is fitted within the approved truck envelope
  outside the tactical board. It has postwar-looking markings and is an art
  integration example, not certified mission-era vehicle content.
- Guards use a distinct supplied officer figurine; the player keeps the partisan.
  Both are grounded static meshes at 180 cm. Their source files are unchanged.
- A slightly lower, closer camera improves wall and character silhouettes.

The environment is currently shared by all three missions. Props outside the
board do not create tactical cover. Wooden paths are shallow visual details,
not new collision or navigation rules.

## Subsequent Passes

1. The initial corner HUD with supplied role art is implemented in the
   2026-09-27 pass; see REFERENCE_HUD.md. Refine selection feedback alongside
   subsequent art and interaction passes.
2. Remove remaining display bases from scenery, expand the supply prop library,
   and refine roof snow and forest variations after visual review.
3. Add skeletons, animation and historically reviewed enemy/vehicle assets.
4. Author a 48 x 48 mission with validated routes, coherent terrain and performance
   budgets. The current board remains 11 x 9; 96 x 96 is a later authoring target.

Evaluate each pass using rendered UE screenshots and mission regression playback.
Screenshots are evidence of the actual game, not substitute mockups. A Windows
editor demo and the separate 2D browser game remain distinct products until a
browser rendering/streaming path is implemented.

## Verification: 2026-09-26

The Win64 editor module built successfully on UE 5.8.2. Asset validation passed
for the new cabin, caravan, officer, roads and crates, including original PBR
texture channel assignments, Nanite settings and the officer's 180 cm height.

Rendered smoke tests passed at 1280 x 720 and 1920 x 1080. All three mission
solutions (22, 24 and 27 turns), unit mesh/material checks, building/vehicle
envelopes and camera picking at eight angles passed. These are automated editor
tests, not packaged Windows or Android device validation.

The actual Full HD starting view is saved as `ReferencePass-2026-09-26.png`
alongside this document. Review still identifies flat snow, visibly faceted
roof snow, paving that needs less regular shapes and the oversized HUD as the
largest gaps. The caravan is partly cropped in the default view; camera panning
reveals it. Asset integration is a first pass, not reference-quality sign-off.
