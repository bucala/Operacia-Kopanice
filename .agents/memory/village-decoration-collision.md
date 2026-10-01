---
name: Village decoration collision
description: Durable rule for keeping decorative village props consistent with the GO puzzle grid.
---

Decorations are authored as visual objects anchored to logical cells, but collision behavior must be represented explicitly rather than inferred from sprite size. Houses and trees are solid by default; lightweight props such as crates and fences remain visual-only unless a level sets movement or sight blocking flags.

**Why:** Large sprites can overlap neighboring cells visually, and treating every decoration as solid would silently change existing puzzle routes. Explicit defaults plus opt-in flags preserve authored solutions while preventing walk-through props.

**How to apply:** When adding a decoration kind, decide its movement and sight defaults in the grid model, keep critical nodes free of solid decorations, and add a fixture test for any new collision behavior.