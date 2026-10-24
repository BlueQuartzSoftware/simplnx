# V&V Report: ComputeFeatureReferenceMisorientationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `24b54daf-3bf5-4331-93f6-03a49f719bf1` |
| SIMPLNX Human Name | Compute Feature Reference Misorientations |
| DREAM.3D v6.5.171 equivalent | `FindFeatureReferenceMisorientations` (OrientationAnalysis plugin, UUID `428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-01; second engineer: Nathan Young, 2026-06-03 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindFeatureReferenceMisorientations` in DREAM.3D v6.5.171. SIMPLNX retains both reference modes and adds double-precision orientation math, input checks, block reads and center coordinates. |
| Oracle (confirmed) | Class 1 analytical angles and centers, with Class 4 bounds, skip rules and feature means. Six analytical fixtures and an invariant sweep encode the oracle. |
| Code paths enumerated | 11 of 23 paths exercised. Gaps include cancellation, I/O failures, invalid feature counts and block-boundary cases. |
| Tests today | 10 test cases: six analytical cases, one invariant sweep, one bounds case, one conversion case and one hidden boundary case. The nine default cases run in the supplied CI configuration. |
| Exemplar archive | None. Tests construct inputs and expected values in C++; `compute_feature_reference_misorientation.tar.gz` is retired. |
| Legacy comparison | Not run on the current algorithm. The DREAM.3D v6.5.171 comparison predates the current block-read algorithm. |
| Bug flags | None |
| V&V phase | Source review, analytical fixtures and deviation records are complete. A legacy comparison with the current algorithm remains open. |

## Summary

`ComputeFeatureReferenceMisorientationsFilter` calculates each cell's misorientation from its feature's average orientation or its farthest-boundary cell orientation. It also calculates feature means and, in the second mode, reference-cell coordinates. Class 1 fixtures and Class 4 invariants verify both modes; the fixed defects are listed below.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* The DREAM.3D v6.5.171 source and current algorithm share reference selection and feature averaging. SIMPLNX corrects Mode 1, adds reference-cell coordinates, reads data in blocks and checks indices.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Pure rotations about the cubic c-axis give known angles; literal cell positions give centers, and participating-cell sums divided by counts give feature means.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeFeatureReferenceMisorientationsTest.cpp` — six `Class 1 - ...` fixtures and `Class 4 - Invariants Sweep`; nine default test cases run in CI. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-06-03.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.1 contains the fix for `ComputeFeatureReferenceMisorientationsFilter-D2` and `ComputeFeatureReferenceMisorientationsFilter-D3`. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureReferenceMisorientationsFilter-D4`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|---------------------------|
| `ComputeFeatureReferenceMisorientationsFilter-D2` | Mode 1 accessed the average-quaternion input even when that mode did not supply it. | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was affected. | The filter obtains the feature count from the selected feature matrix when average quaternions are absent. |
| `ComputeFeatureReferenceMisorientationsFilter-D3` | Mode 1 used a cell index to read the feature-average quaternion array, causing wrong values or an invalid access. | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was not affected. | The filter reads the selected cell's quaternion from the cell array. |
| `ComputeFeatureReferenceMisorientationsFilter-D4` | SIMPL conversion selected the cell matrix as the feature matrix. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The filter derives the feature matrix from the legacy AvgQuats path. |
| `ComputeFeatureReferenceMisorientationsFilter-D5` | A participating phase could read beyond Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter returns `-34901` before the lookup. |
| `ComputeFeatureReferenceMisorientationsFilter-D6` | An invalid crystal-structure value could index beyond the orientation operators. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter returns `-34902` before the operator lookup. |

## Code path coverage

11 of 23 paths exercised. Each uncovered path and its reason appear below.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureReferenceMisorientations.cpp` (249 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureReferenceMisorientationsFilter.cpp` (261 lines).

Test names below omit the `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: ` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Reference | Mode 0 uses the feature-average quaternion. | `Class 1 - Mode 0 SingleGrainIdentity`; `Class 1 - Mode 0 KnownAngle5deg` |
| 2 | Reference | Mode 1 selects the greatest distance; the later equal-distance cell wins. | `Class 1 - Mode 1 KnownCenter`; `Class 1 - Mode 1 MultiGrain CenterIsolation` |
| 3 | Centers | Write the selected cell coordinates in 2D and 3D. | `Class 1 - Mode 1 KnownCenter`; `Class 1 - Mode 1 3D Volume` |
| 4 | Cells | Positive feature and phase compute angles, sums and counts. | `Class 1 - Mode 0 KnownAngle5deg`; `Class 1 - Mode 1 KnownCenter` |
| 5 | Cells | Feature zero or phase zero leaves a zero cell angle. | `Class 1 - Mode 0 MultiGrain EdgeCases` |
| 6 | Cancellation | Stop during center selection or cell processing. | *Not directly tested. No fixture injects cancellation.* |
| 7 | Finalize | A feature has no participating cells; its mean is zero. | `Class 1 - Mode 0 MultiGrain EdgeCases` |
| 8 | Finalize | A nonempty feature gets sum/count; background stays zero. | `Class 1 - Mode 0 MultiGrain EdgeCases`; `Class 1 - Mode 1 MultiGrain CenterIsolation` |
| 9 | Validation | Participating phase exceeds the ensemble bounds; return `-34901`. | `Phase and Laue Index Bounds` — both modes |
| 10 | Reference | A distance tie crosses a 65,536-cell block boundary. | *Not directly tested. Only the hidden `genuine HDF5 65536-block tail oracle` targets this boundary.* |
| 11 | Cells | Process a full 65,536-cell block and a one-cell tail. | *Not directly tested. Only the hidden `genuine HDF5 65536-block tail oracle` supplies this size.* |
| 12 | Validation | Participating Laue index exceeds the operator bounds; return `-34902`. | `Phase and Laue Index Bounds` — both modes |
| 13 | Cells | Negative phase skips angle calculation. | *Not directly tested. No fixture assigns a negative phase.* |
| 14 | Validation | Reject invalid FeatureIds before feature-array access. | *Not directly tested. No fixture supplies a negative or out-of-range FeatureId.* |
| 15 | Validation | Neither feature source supplies tuples; return `-34900`. | *Not directly tested. Every fixture supplies feature tuples.* |
| 16 | Reference | Mode 1 obtains the feature count without an AvgQuats array. | *Not directly tested. The shared scaffold creates AvgQuats in both modes.* |
| 17 | Validation | Ignore an invalid phase on feature zero. | `Phase and Laue Index Bounds` — successful execution in both modes |
| 18 | Input | Ensemble or average-quaternion cache read fails. | *Not directly tested. No fixture injects a cache read failure.* |
| 19 | Reference | Feature-ID, distance or center-quaternion read fails. | *Not directly tested. No fixture injects a center-selection read failure.* |
| 20 | Cells | Feature-ID, phase or quaternion read, or angle write, fails. | *Not directly tested. No fixture injects a cell I/O failure.* |
| 21 | Reference | No center candidate reaches zero distance; the default index remains zero. | *Not directly tested. Every Mode 1 feature has a qualifying center.* |
| 22 | Conversion | Convert legacy Mode 0 parameters and UUID. | `SIMPL Backwards Compatibility` — `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` |
| 23 | Conversion | Convert Mode 1 with distinct cell and feature matrices. | *Not directly tested. Both conversion fixtures use Mode 0 and the same parent for all array paths.* |

## Test inventory

The nine default test cases run in CI. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Phase and Laue Index Bounds` | kept | Both modes reject invalid participating phase/Laue values and ignore an invalid phase on feature zero. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: SIMPL Backwards Compatibility` | kept | Checks UUID, comments and arguments in `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)`. Both fixtures use Mode 0 and identical parent paths; they do not isolate D4. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: genuine HDF5 65536-block tail oracle` | new-for-V&V | Hidden. Requires HDF5 stores. Literal angles are [15°,30°] in Mode 0 and [15°,0°] in Mode 1; means are 22.5° and 7.5°. The later tied center is (65536.5,0.5,0.5). |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 SingleGrainIdentity` | new-for-V&V | 2×2×2 identity orientations; all cell angles and feature means are zero. Checks Class 4 invariants. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 KnownAngle5deg` | new-for-V&V | 2×2×2; all cells are 5° from the identity reference. Checks angles, mean and Class 4 invariants. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 MultiGrain EdgeCases` | new-for-V&V | 4×3×1; checks background, unphased cells, an empty-count feature, literal angles and means, and Class 4 invariants. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 KnownCenter` | new-for-V&V | 3×3×1; checks the selected center (1.5,1.5,0.5), cell angles, feature mean and Class 4 invariants. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 MultiGrain CenterIsolation` | new-for-V&V | 2×3×1; checks two independent centers, the later-cell tie rule, literal angles, means and Class 4 invariants. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 3D Volume` | new-for-V&V | 3×3×2; checks the center (1.5,1.5,1.5), angles, mean and Class 4 invariants. |
| `OrientationAnalysis::ComputeFeatureReferenceMisorientationsFilter: Class 4 - Invariants Sweep` | new-for-V&V | Two configurations check nonnegative angles, the cubic angle bound, skip rules, background zero and feature means. |

## Exemplar archive

- **Archive:** None. Inputs and oracles are inline; `compute_feature_reference_misorientation.tar.gz` is retired.
- **SHA512:** Not applicable. The current plugin `test/CMakeLists.txt` has no registration for this archive.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureReferenceMisorientationsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureReferenceMisorientationsFilter.md` for source evidence, comparison artifacts and recommendations.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureReferenceMisorientationsFilter-D1` | Orientation precision and accumulation order change cell angles and feature means. |
| `ComputeFeatureReferenceMisorientationsFilter-D2` | DREAM.3D v6.5.171 crashes in direct Mode 1; current SIMPLNX obtains the feature count without the Mode 0 input. |
| `ComputeFeatureReferenceMisorientationsFilter-D7` | SIMPLNX adds the selected cell's coordinates as a Mode 1 feature array. |
