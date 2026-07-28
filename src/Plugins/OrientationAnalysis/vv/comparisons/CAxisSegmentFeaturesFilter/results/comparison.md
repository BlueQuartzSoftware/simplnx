# Legacy Comparison: CAxisSegmentFeaturesFilter

Date: 2026-07-22 (TC1–TC4); TC5_3D added and full suite rerun 2026-07-24

## Test Cases

Five pure-Phi Bunge fixtures (TC1–TC4 identical to the Class 1 analytical fixtures in
`test/CAxisSegmentFeaturesTest.cpp`; TC5_3D exercises the y/z stride branches of the 3-D flood
fill, which the 1-D fixtures cannot), tolerance 10°, face neighbors:

| Case | Grid | Phi per cell (deg) | Extras | Expected partition |
|---|---|---|---|---|
| TC1_Chain | 8×1×1 | 0, 5, 8, 45, 50, 120, 124, 90 | — | {0,1,2} {3,4} {5,6} {7} |
| TC2_PiFold | 3×1×1 | 2, 176, 88 | π-fold branch | {0,1} {2} |
| TC3_Mask | 5×1×1 | 0, 20, 22, 0, 90 | bool mask [0,1,1,0,1] | {1,2} {4}; cells 0,3 → id 0 |
| TC4_Phase0 | 4×1×1 | 0, 0, 0, 0 | phases [0,1,1,1] | {1,2,3}; cell 0 → id 0 |
| TC5_3D | 3×2×2 | 0, 50, 55, 4, 90, 53, 8, 95, 130, 6, 176, 120 | bool mask [0,1,1,1,1,0,1,1,1,1,1,1] | {1,2} {3,6,9,10} {4} {7} {8,11}; cells 0,5 → id 0 |

TC5_3D uses x-fastest linearization (idx = x + 3y + 6z). Feature {3,6,9,10} spans a z-stride hop
(3→9: |4−6| = 2°), a y-stride hop (9→6: |6−8| = 2°), and a π-fold x hop (9→10: fold(6°,176°) = 10°);
{8,11} spans a y-stride hop at z = 1 (|130−120| = 10°). Masked voxel 0 additionally pins the
validated-first-seed (D1) path in 3-D against legacy.

## Input Data

One legacy v7 `.dream3d` file authored by `../pipelines/make_input.py` (legacy_dream3d writer),
read by BOTH runners, so the Quats/Phases/Mask/CrystalStructures inputs are byte-identical.

## Runners

- Legacy: `/Users/mjackson/Applications/DREAM3D.app/Contents/Bin/PipelineRunner` (official 6.5.171)
- NX: `/Users/mjackson/Workspace5/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel/Bin/nxrunner`, simplnx at the V&V commit (post D1/D4/D5 fixes)

## Results

Full-suite rerun 2026-07-24 (legacy FeatureIds differ run-to-run by construction, D2):

| Case | Legacy FeatureIds | NX FeatureIds | Partition match | Feature count match |
|---|---|---|---|---|
| TC1_Chain | [1,1,1,4,4,2,2,3] | [1,1,1,2,2,3,3,4] | YES | YES (4+1 Active tuples) |
| TC2_PiFold | [1,1,2] | [1,1,2] | YES | YES (2+1) |
| TC3_Mask | [0,2,2,0,1] | [0,1,1,0,2] | YES | YES (2+1) |
| TC4_Phase0 | [0,1,1,1] | [0,1,1,1] | YES | YES (1+1) |
| TC5_3D | [0,4,4,2,1,0,2,5,3,2,2,3] | [0,1,1,2,3,0,2,4,5,2,2,5] | YES | YES (5+1) |

**All five cases match at the segmentation-partition level, and feature counts are identical.**
Raw legacy ids are a random permutation of NX ids because 6.5.171 unconditionally randomizes
FeatureIds with a clock-derived seed (`CAxisSegmentFeaturesFilter-D2`); bit-identical FeatureIds are
not attainable against 6.5.171 by construction. NX ids additionally match the Class 1 oracle
expectation exactly (TC5_3D against an independently computed reference flood fill).

## Fixes Applied

None in this comparison round — SIMPLNX had already been reconciled against the Class 1/4
oracle earlier in the V&V cycle, which fixed three SIMPLNX defects (D1 unvalidated first seed,
D4 spurious rejection of unindexed/masked cells, D5 RectGrid crash). No legacy patch: legacy
output is not wrong on any shared code path.

## Notes

- Legacy 6.5.171 cannot disable FeatureIds randomization (not exposed as a parameter), so the
  comparison is partition-level by necessity. Partition equality is the meaningful invariant.
- NX-only behavior not comparable against 6.5.171: 26-neighbor "All Connected" scheme, RectGrid
  geometry input, uint8 masks, crystal-structure validation (D3).
- Reproduce: `python3 ../pipelines/make_input.py`, then run both pipelines in `../pipelines/`.
