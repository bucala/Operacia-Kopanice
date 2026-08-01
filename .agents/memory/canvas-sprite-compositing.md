---
name: Canvas sprite compositing
description: Durable guidance for rendering uploaded PNG assets in the isometric canvas.
---

Use normal source-over compositing for PNG sprites that already carry an alpha channel. Screen blending is not a reliable substitute for background removal: it makes light photographic backgrounds and slabs remain visibly bright against the snowy board.

**Why:** An uploaded vehicle image exposed that screen compositing can turn unwanted pale pixels into a rectangular plaque, even when the source appears visually acceptable in isolation.

**How to apply:** Validate asset alpha on a dark and light background before registering it, and prefer removing or replacing an asset over masking it with a blend mode that changes its colors.