# V&V Report: CAxisSegmentFeaturesFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `9fe07e17-aef1-4bf1-834c-d3a73dafc27d` |
| SIMPLNX Human Name | Segment Features (C-Axis Misalignment) |
| DREAM.3D v6.5.171 equivalent | `CAxisSegmentFeatures` (Reconstruction plugin, SIMPL UUID `bff6be19-1219-5876-8838-1574ad29d965`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-23; second engineer: Matthew Marine, 2026-07-25 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `CAxisSegmentFeatures` in DREAM.3D v6.5.171. Connected-component labeling replaces flood fill and preserves the folded c-axis angle rule. |
| Oracle (confirmed) | Class 1 analytical labels and Class 4 invariants are encoded in ten dedicated cases in `test/CAxisSegmentFeaturesTest.cpp`. All pass in CI. |
| Code paths enumerated | 31 of 44 exercised. Defensive guards, cancellation, external-store selection and injected failures have no direct fixtures. |
| Tests today | 29 test cases: 28 ordinary cases and one hidden input generator. They cover analytical labels, invariants, archive regression, validation and SIMPL conversion. |
| Exemplar archive | `segment_features_test_data.tar.gz` supplies inputs and four label arrays for regression tests. Independent expected values are defined in C++ fixtures. |
| Legacy comparison | Not run for the current algorithm. The DREAM.3D v6.5.171 comparison dated 2026-09-17 used DREAM3D-NX v7.4.2 and predates the rewrite. |
| Bug flags | None |
| V&V phase | COMPLETE |

## Summary

`CAxisSegmentFeaturesFilter` groups adjacent cells of the same positive phase when their folded c-axis angle is at or below the tolerance. Class 1 fixtures check exact labels, and Class 4 invariants check feature counts, background values and randomization. The seven defects listed below are fixed.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* Shared connected-component labeling replaces the legacy flood fill; `areNeighborsSimilar()` retains the normalized c-axis dot product, clamped angle and antiparallel fold.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Pure-Phi quaternions give folded distances `min(abs(ΔPhi), 180° − abs(ΔPhi))`; adjacency and equal positive phases determine exact labels, with N + 1 Active tuples and background zero.

*Encoded:* `src/Plugins/OrientationAnalysis/test/CAxisSegmentFeaturesTest.cpp::OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pure-Phi Chain, Face)` and seven analytical companions, plus two `Class 4 Invariants` cases. All pass in CI.

*Second-engineer review:* Matthew Marine, 2026-07-25.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.1. DREAM3D-NX v7.4.1 contains the fix for `CAxisSegmentFeaturesFilter-D6`. DREAM3D-NX v7.4.2 contains the fix for `CAxisSegmentFeaturesFilter-D1`. DREAM3D-NX v7.4.2 contains the fix for `CAxisSegmentFeaturesFilter-D4`. DREAM3D-NX v7.4.2 contains the fix for `CAxisSegmentFeaturesFilter-D5`. DREAM3D-NX v7.4.2 contains the fix for `CAxisSegmentFeaturesFilter-D7`. DREAM3D-NX v7.4.2 contains the fix for `CAxisSegmentFeaturesFilter-D8`. DREAM3D-NX v7.4.2 contains the fix for `CAxisSegmentFeaturesFilter-D9`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `CAxisSegmentFeaturesFilter-D1` | An unchecked first seed could create a phantom feature or grow from a masked cell. | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. | Labels only eligible cells. |
| `CAxisSegmentFeaturesFilter-D4` | Validation rejected excluded phases; later code also read Crystal Structures through unchecked phase indices. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Validates only participating phases and returns `-8364` for an invalid positive index. |
| `CAxisSegmentFeaturesFilter-D5` | A stale ImageGeom cast crashed accepted RectGrid input. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Accesses both geometries through `IGridGeometry`. |
| `CAxisSegmentFeaturesFilter-D6` | The feature count included the next unused ID, creating an extra empty tuple. | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was not affected. | Counts real features and reserves exactly one background tuple. |
| `CAxisSegmentFeaturesFilter-D7` | Matching but undersized cell arrays could pass preflight and be read outside their bounds. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Checks input and CellData tuple counts against the geometry (`-652`, `-653`). |
| `CAxisSegmentFeaturesFilter-D8` | Quats outside geometry CellData made execution look for FeatureIds at the wrong path. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Preflight and execution both derive FeatureIds from geometry CellData. |
| `CAxisSegmentFeaturesFilter-D9` | Cancellation could report no features instead of returning cleanly. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Checks cancellation before the no-feature error. |

## Code path coverage

31 of 44 paths exercised. Each uncovered row states the missing trigger.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/CAxisSegmentFeatures.cpp` (421 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/CAxisSegmentFeaturesFilter.cpp` (256 lines), `src/simplnx/Utilities/SegmentFeatures.cpp` (1159 lines), and `src/simplnx/Utilities/ClusteringUtilities.cpp` (126 lines).

Test names below omit their `OrientationAnalysis::CAxisSegmentFeaturesFilter: ` or `OrientationAnalysis::CAxisSegmentFeatures: ` prefix. The scan assigns temporary labels, merges equivalent labels and writes dense final IDs.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Validation | Mask cannot be instantiated → error `-8362` | *Not directly tested. Normal parameter validation rejects the invalid mask first.* |
| 2 | Validation | Phase zero → skip crystal-structure validation | `Phase 0 (Unindexed) Cells Tolerated` |
| 3 | Validation | Masked non-hexagonal cell → skip validation | `Masked Non-Hexagonal Cells Tolerated` |
| 4 | Validation | Participating phase exceeds ensemble bounds → error `-8364` | `Execute Error - Phase Out of Ensemble Bounds (-8364)` |
| 5 | Validation | Participating non-hexagonal phase → error `-8363` | `Execute Error - Non-Hexagonal Crystal Structure (-8363)` |
| 6 | Validation | Accept Hexagonal_High and Hexagonal_Low | `Class 1 Analytical (Phase Separation)` |
| 7 | Geometry | Accept Image and RectGrid through `IGridGeometry` | `Class 1 Analytical (Pure-Phi Chain, Face)`; `Class 1 Analytical (RectGrid Geometry)` |
| 8 | Labeling | Reject masked cell zero before assigning a label | `Class 1 Analytical (Mask Excludes Voxel 0)` |
| 9 | Labeling | Face connectivity uses X, Y and Z strides and clips image boundaries | `Class 1 Analytical (3D Linearization, 3x2x2)` |
| 10 | Labeling | All-connected mode joins edge and vertex neighbors | `Class 1 Analytical (Neighbor Scheme Face vs All)`; `FaceEdgeVertex Connectivity` |
| 11 | Cancellation | Stop during scans or before finalization without reporting `-87000` | *Not directly tested. No test injects cancellation.* |
| 12 | Labeling | Masked or phase-zero cell → retain label zero | `Class 1 Analytical (Mask Excludes Voxel 0)`; `Phase 0 (Unindexed) Cells Tolerated` |
| 13 | Labeling | No similar labeled neighbor → create a temporary label | `Class 1 Analytical (Pure-Phi Chain, Face)` |
| 14 | Finalization | No valid cell → error `-87000` | `Execute Error - No Features Found (-87000)`; `No Valid Voxels Returns Error` |
| 15 | Labeling | Neighbor has label zero → skip it | `Class 1 Analytical (Mask Excludes Voxel 0)` |
| 16 | Grouping | Different phases → do not join | `Class 1 Analytical (Phase Separation)` |
| 17 | Grouping | Angle is within tolerance → join | `Class 1 Analytical (Pure-Phi Chain, Face)` |
| 18 | Grouping | Antiparallel folded angle is within tolerance → join | `Class 1 Analytical (Pi-Fold Antiparallel C-Axes)` |
| 19 | Finalization | Resize feature matrix to N + 1; set Active[0] to zero and real features to one | `Class 1 Analytical (Pure-Phi Chain, Face)`; `Class 1 Analytical (RectGrid Geometry)` |
| 20 | Finalization | Randomization preserves partitions and background zero | `Class 4 Invariants (RandomizeFeatureIds)`; `Class 4 Invariants (RandomizeFeatureIds Preserves Masked Zeros)` |
| 21 | Preflight | Zero tolerance → error `-655` | `Preflight Error - Zero Tolerance (-655)` |
| 22 | Preflight | Selected cell arrays have different tuple counts → error `-651` | `Preflight Error - Cell array tuple count mismatch (-651)` |
| 23 | Preflight | Cell-array tuple count differs from geometry → error `-652` | `Preflight Error - Cell Arrays Smaller Than Geometry (-652)` |
| 24 | Preflight | Cell AttributeMatrix tuple count differs from geometry → error `-653` | `Preflight Error - Cell AttributeMatrix Smaller Than Geometry (-653)` |
| 25 | Output path | Quats outside CellData → create and access FeatureIds in geometry CellData | `Class 1 Analytical (Quats Outside Cell AttributeMatrix)` |
| 26 | Finalization | Randomization disabled → retain dense labels in first-occurrence order | `Class 1 Analytical (Pure-Phi Chain, Face)` |
| 27 | Input | Read Bool and UInt8 masks; no mask → treat every cell as unmasked | `Class 1 Analytical (Mask Excludes Voxel 0)` — both variants; `Class 1 Analytical (Pure-Phi Chain, Face)` |
| 28 | Validation | Repeated valid phase → reuse cached validation | `Class 1 Analytical (Pure-Phi Chain, Face)` |
| 29 | Input | Crystal-structure, phase, quaternion or mask read fails → propagate error | *Not directly tested. No fixture injects an input-store failure.* |
| 30 | Input | Fill two slice slots and replace the least recently used slot | `High Tolerance Merges All` — 15 slices and exact labels |
| 31 | Input | Requested slice already cached → update its use order | *Not directly tested. The nonperiodic forward scan loads each slice once.* |
| 32 | Grouping | Requested voxel lies outside both buffered slices → reject it | *Not directly tested. The driver supplies current or previous slice neighbors.* |
| 33 | Grouping | Disable slice buffers and use direct array callbacks | *Not directly tested. This filter enables slice buffers before its scan.* |
| 34 | Input | Unsupported mask storage → error `-8365` | *Not directly tested. The mask parameter permits only Bool and UInt8.* |
| 35 | Labeling | Adopt a neighbor label or unite different temporary labels | `FaceEdgeVertex Connectivity`; `Class 1 Analytical (3D Linearization, 3x2x2)` |
| 36 | Grouping | Both folded angle tests fail → keep separate features | `Class 1 Analytical (Pure-Phi Chain, Face)` |
| 37 | Dispatch | Use the in-memory equivalence table | `Class 1 Analytical (Pure-Phi Chain, Face)` — scenario harness |
| 38 | Finalization | Map positive labels to dense IDs; leave background zero | `Class 1 Analytical (Mask Excludes Voxel 0)` |
| 39 | Limits | Label capacity, allocation or temporary-store provider fails → return error | *Not directly tested. Fixtures do not reach capacity limits or inject provider failures.* |
| 40 | Output | FeatureIds read or write fails during labeling → propagate error | *Not directly tested. No fixture injects a FeatureIds-store failure.* |
| 41 | Finalization | Feature-matrix resize or randomized-label transfer fails → propagate error | *Not directly tested. No fixture injects these failures.* |
| 42 | Shared driver | Periodic face or complete-boundary merge | *Not directly tested. This filter does not enable the shared driver's periodic option.* |
| 43 | Validation | Negative phase → skip validation and labeling | *Not directly tested. The phase-exemption fixture uses zero.* |
| 44 | Dispatch | Use external equivalence storage | *Not directly tested. No fixture in this file requires an external store.* |

## Test inventory

All cases are in `src/Plugins/OrientationAnalysis/test/CAxisSegmentFeaturesTest.cpp`. There are 28 ordinary cases. All pass in CI. The hidden generator creates inputs and does not assert segmentation results.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::CAxisSegmentFeatures: No Valid Voxels Returns Error` | kept | All-masked synthetic data must return `-87000`. |
| `OrientationAnalysis::CAxisSegmentFeatures: Randomize Feature IDs` | kept | Synthetic layered orientations give three real features; checks dense randomized IDs and four Active tuples. |
| `OrientationAnalysis::CAxisSegmentFeatures: High Tolerance Merges All` | kept | A 90° tolerance merges all 15×15×15 cells; asserts every FeatureId is 1 and Active has two tuples. |
| `OrientationAnalysis::CAxisSegmentFeatures: FaceEdgeVertex Connectivity` | kept | Checks five face-connected features and three all-connected features, including edge and vertex pair identities. |
| `OrientationAnalysis::CAxisSegmentFeatures: Generate Test Data` | kept | Hidden (`[.GenerateTestData]`). Writes synthetic 15×15×15 and 200×200×200 inputs; does not execute segmentation. |
| `OrientationAnalysis::CAxisSegmentFeatures:Face` | kept | Archive regression: compares every label with `CAxis_FeatureIds_Face`, expects 57 Active tuples and checks tuple shapes. |
| `OrientationAnalysis::CAxisSegmentFeatures:All` | kept | Archive regression: compares every label with `CAxis_FeatureIds_All`, expects 37 Active tuples and checks tuple shapes. |
| `OrientationAnalysis::CAxisSegmentFeatures:MaskFace` | kept | Archive regression: compares every label with `CAxis_FeatureIds_Mask_Face`, expects 31 Active tuples and checks tuple shapes. |
| `OrientationAnalysis::CAxisSegmentFeatures:MaskAll` | kept | Archive regression: compares every label with `CAxis_FeatureIds_Mask_All`, expects 25 Active tuples and checks tuple shapes. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pure-Phi Chain, Face)` | new-for-V&V | Eight-cell chain; checks eight labels, four real features and Active values. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Pi-Fold Antiparallel C-Axes)` | new-for-V&V | Three cells; checks the antiparallel angle fold and two features. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Neighbor Scheme Face vs All)` | new-for-V&V | `Face Neighbors` and `All Connected Neighbors` variants check `[1,2,3,4]` and `[1,2,3,1]`. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Mask Excludes Voxel 0)` | new-for-V&V | `bool mask` and `uint8 mask` variants check `[0,1,1,0,2]` and three Active tuples (D1). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Phase Separation)` | new-for-V&V | Equal orientations in two phases give `[1,1,2,2]`; accepts Hexagonal_Low. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (3D Linearization, 3x2x2)` | new-for-V&V | Checks all 12 labels and six features on a 3×2×2 orientation field with different values along each axis. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (RectGrid Geometry)` | new-for-V&V | RectGrid with unequal X intervals gives `[1,1,2]` and three Active tuples (D5). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 1 Analytical (Quats Outside Cell AttributeMatrix)` | new-for-V&V | Quats and phases in a sibling AttributeMatrix still produce `[1,1,2]` in geometry CellData (D8). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 4 Invariants (RandomizeFeatureIds)` | new-for-V&V | Two runs check partition preservation, IDs 1 through 4, a nonidentity permutation and determinism. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Class 4 Invariants (RandomizeFeatureIds Preserves Masked Zeros)` | new-for-V&V | Checks that masked cells retain zero and that two real features remain distinct after randomization. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Phase 0 (Unindexed) Cells Tolerated` | new-for-V&V | Phase-zero background stays zero while indexed cells form one feature (D4). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Masked Non-Hexagonal Cells Tolerated` | new-for-V&V | A masked cubic cell stays zero; the two hexagonal cells remain separate (D4). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - Non-Hexagonal Crystal Structure (-8363)` | new-for-V&V | Participating cubic cells return `-8363`. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - Phase Out of Ensemble Bounds (-8364)` | new-for-V&V | Phase 7 with two ensemble entries returns `-8364` (D4). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Execute Error - No Features Found (-87000)` | new-for-V&V | All three cells masked; requires error `-87000` (D1 and D6). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Zero Tolerance (-655)` | new-for-V&V | Zero tolerance returns preflight error `-655`. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell Arrays Smaller Than Geometry (-652)` | new-for-V&V | Nine input tuples for ten cells return `-652` (D7). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell AttributeMatrix Smaller Than Geometry (-653)` | new-for-V&V | Ten input tuples with a nine-tuple CellData matrix return `-653` (D7). |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: Preflight Error - Cell array tuple count mismatch (-651)` | kept | Quats and phases with different tuple counts return `-651`. |
| `OrientationAnalysis::CAxisSegmentFeaturesFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` variants check filter identity and converted arguments. |

## Exemplar archive

- **Archive:** `segment_features_test_data.tar.gz`
- **SHA512:** `317d69384330d40c673f8e1a42df003dede5ac85331b2549e9f45467f7af6b74284f8dad1120427690719ebcd5066830e17031533381cc2cc0cc8622a422b914`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/CAxisSegmentFeaturesFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/CAxisSegmentFeaturesFilter.md`. This table lists Part 1; fixed SIMPLNX defects are in Part 2.

| Deviation | Observed difference |
|-----------|---------------------|
| `CAxisSegmentFeaturesFilter-D2` | DREAM.3D v6.5.171 always randomizes FeatureIds with a clock seed; SIMPLNX defaults to ordered labels and offers deterministic randomization. |
| `CAxisSegmentFeaturesFilter-D3` | SIMPLNX rejects participating non-hexagonal phases and invalid ensemble indices; DREAM.3D v6.5.171 does not apply these checks. |
