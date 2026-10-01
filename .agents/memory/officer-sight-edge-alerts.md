---
name: Officer sight-edge alerts
description: Invariant for resolving overlapping officer sight beams and infantry alerts.
---

An officer’s outermost visible sight cell is a warning cell only for that specific officer. If another officer also sees that cell as an inner beam cell, the player is still spotted.

**Why:** Treating all officer edge cells as one shared exemption makes overlapping beams nondeterministically safer than the mission rules intend.

**How to apply:** When checking lethal sight, evaluate each guard’s own visible-cell list and exempt only that guard’s final cell; keep a regression test for overlapping officer beams.