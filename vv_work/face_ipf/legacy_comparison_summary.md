# Legacy Comparison: ComputeFaceIPFColoring (issue #1635)

Date: 2026-06-30

## Goal

Three-way A/B of the Phase-2 face-IPF coloring: SIMPLNX (fixed) vs DREAM3D 6.5.171 (buggy) vs DREAM3D 6.5.172 (fixed).

## Environment

- **6.5.171 (buggy):** `/Users/mjackson/Applications/DREAM3D.app/Contents/bin/PipelineRunner` (Apr 2023)
- **6.5.172 (fixed):** `/Users/mjackson/Workspace3/6.5.172/DREAM3D-Build/D3D-Rel-Qt515-6_5_171/Bin/PipelineRunner`
- **SIMPLNX (fixed):** `…/DREAM3D-Build/NX-Com-Qt69-Vtk96-Rel/Bin/nxrunner`

## Inputs

1. **`fixture_legacy.dream3d`** — a legacy-native 4-face triangle mesh (built with the
   `legacy_dream3d` writer extended with `add_triangle_geom`), mirroring the Class 1 analytical
   unit-test fixture: feature1 = cubic (phase 1), feature2 = hex (phase 2), identity orientations,
   corner-aligned face normals. Both legacy `PipelineRunner` binaries read it without error.
   - F0 `(1,2)` cubic|hex (mixed-phase), F1 `(-1,2)` --|hex (boundary), F2 `(1,-1)` cubic|--, F3 `(1,3)` cubic|cubic
2. **`6_6_Small_IN100_GBCD.dream3d`** — the real 756,474-face mesh, used for the SIMPLNX-vs-baked
   real-data check (legacy binaries cannot read its NX-written geometry; see Note A).

## Result 1 — three-way on the legacy-native fixture (the headline)

Second (Phase-2 side) color per face. Cubic first-side colors and the cubic|cubic face (F3) agree across all three.

| Face | kind | 6.5.171 (buggy) | 6.5.172 (fixed) | SIMPLNX (fixed) |
|---|---|---|---|---|
| F0 | cubic\|hex (mixed) | **(255,0,0)** wrong-phase cubic | (0,0,255) hex | (0,255,0) hex |
| F1 | --\|hex (boundary) | **(0,0,0)** black | (0,0,255) hex | (0,255,0) hex |
| F2 | cubic\|-- | (0,0,0) black | (0,0,0) black | (0,0,0) black |
| F3 | cubic\|cubic | (0,0,255) | (0,0,255) | (0,0,255) |

Two independent deviations fall out cleanly:

- **D1 — the #1635 bug (root cause: bug in 6.5.171).** On F0 and F1, 6.5.171 colors the Phase-2
  side with Phase-1's symmetry (F0 → cubic red instead of a hex color) or leaves it black
  (F1, `feature1 ≤ 0`). 6.5.172 **and** SIMPLNX both apply the hex operator. 6.5.171's output
  matches the hand-derived *buggy* oracle exactly. **The fix is confirmed on real legacy binaries.**
- **D2 — EbsdLib hex basal hue (root cause: library).** Where both fixed implementations apply
  the hex operator (F0, F1), legacy EbsdLib (6.5.172) produces **blue (0,0,255)** and EbsdLib
  3.0.0 (SIMPLNX) produces **green (0,255,0)** for the same hex basal direction. Cubic coloring
  is identical across all three. This is independent of #1635 — it is the hex IPF basal-corner
  hue assignment changing between EbsdLib versions. Both colors have red-channel 0 (a basal
  direction is never the red c-axis corner), so the convention-independent invariant holds for
  both; only the green/blue hue assignment differs.

## Result 2 — SIMPLNX (fixed) vs baked buggy baseline on the real 756,474-face mesh

The baked `SurfaceMeshFaceIPFColors` in `6_6_Small_IN100_GBCD.dream3d` is authentic buggy
(6.5.171-class) output. SIMPLNX-fixed vs that baseline differs on **exactly 120,000 faces**, all
of them `feature1 ≤ 0` boundary faces (the D1 boundary manifestation), **0** elsewhere, **0** on
the 636,474 interior faces. (Single-phase data, so no mixed-phase or hex faces appear — D2 does
not show here.)

## Note A — NX→legacy geometry incompatibility (separate finding)

The NX-written `6_6_Small_IN100_GBCD.dream3d` cannot be read by either legacy `PipelineRunner`
(both 6.5.171 and 6.5.172 fail identically: `SharedTriList` loads as 0 tuples → `-10200`). Root
cause: DREAM3D-NX writes the triangle connectivity as `DataArray<size_t>`, which legacy
`H5DataArrayReader` does not recognize (legacy `SharedTriList = DataArray<int64_t>`); the legacy
reader does not reconstruct the NX `_SIMPL_GEOMETRY` group. This is why the fixture in Input 1 was
authored natively in legacy format. Worth filing as its own NX-export issue; unrelated to the IPF filter.

## Conclusion

- **D1 (#1635):** confirmed and fixed. Trust SIMPLNX / 6.5.172. 6.5.171 was wrong.
- **D2 (hex hue):** library deviation between EbsdLib (green) and legacy EbsdLib (blue); affects
  all hex IPF coloring (face and cell). **Resolved:** EbsdLib is the canonical IPF authority and
  EbsdLib 3.0.0 and 3.1.0 agree on green (the Class 1 oracle test passes unchanged on a from-source
  EbsdLib 3.1.0 build, `NX-Com-Qt69-Vtk95-Rel-EbsdLib`); legacy DREAM3D's blue is the deviation.
  Trust SIMPLNX; expect hex IPF hues to change vs. legacy.
- The Class 1 oracle's exact hex value (green) is the canonical EbsdLib value (3.0.0 == 3.1.0); the
  convention-independent assertion is red-channel == 0, which every version satisfies.

## Fixes Applied

- SIMPLNX `Algorithms/ComputeFaceIPFColoring.cpp:114,123` `phase1`→`phase2`; `KNOWN BUG` block
  removed. No legacy code modified (6.5.172 already carried the D1 fix; D2 is not fixed here).
- `legacy_dream3d` writer extended with `add_triangle_geom` (Gate-1 validated).
