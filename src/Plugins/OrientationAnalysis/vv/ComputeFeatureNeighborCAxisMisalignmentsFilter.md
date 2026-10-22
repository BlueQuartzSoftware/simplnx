# V&V Report: ComputeFeatureNeighborCAxisMisalignmentsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| SIMPLNX Human Name | Compute Feature Neighbor C-Axis Misalignments |
| DREAM.3D v6.5.171 equivalent | `FindFeatureNeighborCAxisMisalignments` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-04; second engineer: Nathan Young, 2026-06-09 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindFeatureNeighborCAxisMisalignments` in DREAM.3D v6.5.171. SIMPLNX adds double-precision matrix calculations, Hexagonal_Low support, and the documented corrections. |
| Oracle (confirmed) | Class 1 angles and averages, with Class 4 invariants, are encoded in four ordinary tests in `test/ComputeFeatureNeighborCAxisMisalignmentsTest.cpp`. All pass in CI. |
| Code paths enumerated | 11 of 24 exercised. Gaps include average-disabled execution, Hexagonal_Low, angle folding, cancellation, and I/O errors. |
| Tests today | 7 ordinary test cases cover analytical values, invariants, input validation, and SIMPL conversion. One hidden test defines a quaternion chunk-boundary fixture. |
| Exemplar archive | `compute_feature_neighbor_caxis_misalignments.tar.gz` remains registered, but its test consumer was removed. Current tests construct their inputs and expected values in C++. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None |
| V&V phase | Verification records and second-engineer review are complete. |

## Summary

`ComputeFeatureNeighborCAxisMisalignmentsFilter` computes neighbor c-axis angles for equal hexagonal crystal-structure values and can average the accepted angles per feature. Class 1 fixtures verify analytical angles and averages; Class 4 checks verify the range and averaging rule. The filter includes the divisor and phase-index corrections listed below.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* The DREAM.3D v6.5.171 `FindFeatureNeighborCAxisMisalignments::execute()` loop is preserved, with double-precision Eigen calculations, Hexagonal_Low support, the divisor correction, input caches, and phase-index checks.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Pure x-axis rotations give the folded absolute tilt difference; accepted neighbor angles give the arithmetic mean, and no accepted neighbors give NaN.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeFeatureNeighborCAxisMisalignmentsTest.cpp` — `Class 1 - Simple Hex Pair`, `Class 1 - Realistic Microstructure (exposes divisor bug)`, and `Class 1 - Mismatch Last Order` provide three fixtures; `Class 4 - Invariants` reuses the mixed-structure fixture. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-06-09.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1` | Resetting the divisor for each neighbor loses earlier exclusions and produces incorrect averages. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter initializes the divisor once per feature and decrements it for every excluded neighbor. |
| `ComputeFeatureNeighborCAxisMisalignmentsFilter-D7` | Invalid focal or neighbor phase indices can read outside Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter checks each phase index before the lookup and returns error `-1564` for an invalid index. |

## Code path coverage

11 of 24 paths exercised. Each uncovered row states the missing fixture or assertion.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp` (190 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureNeighborCAxisMisalignmentsFilter.cpp` (185 lines).

Test names below omit the `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: ` prefix. The crystal-structure checks run during execution, after preflight.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Feature tuple counts differ → error `-1560`. | `Preflight Error - Feature array tuple count mismatch (-1560)` |
| 2 | Preflight | Average enabled → create list and average outputs. | `Class 1 - Simple Hex Pair` |
| 3 | Optional average | Average disabled → create and write only the list. | *Not directly tested. The only false-option fixture fails preflight before output creation.* |
| 4 | Crystal structures | No hexagonal ensemble → error `-1562`. | *Not directly tested. Every execution fixture includes Hexagonal_High.* |
| 5 | Crystal structures | Mixed hexagonal and non-hexagonal ensembles → warning `-1563`. | `Class 1 - Realistic Microstructure (exposes divisor bug)` exercises this path; it does not assert the warning code. |
| 6 | Crystal structures | All non-background ensembles are hexagonal → continue without the mixed-structure warning. | `Class 1 - Simple Hex Pair` |
| 7 | Phase bounds | Focal phase is negative or beyond the ensemble array → error `-1564`. | `Phase Index Bounds` — `Current Feature Phase returns an error` |
| 8 | Phase bounds | Neighbor phase is negative or beyond the ensemble array → error `-1564`. | `Phase Index Bounds` — `Neighbor Feature Phase returns an error` |
| 9 | Feature loop | Skip background feature zero. | `Phase Index Bounds` — `Feature zero is ignored` permits an invalid unused background phase. |
| 10 | Pair calculation | Equal Hexagonal_High structures → compute angle, retain angles below 90°, and accumulate when averaging is enabled. | `Class 1 - Simple Hex Pair`; `Class 1 - Realistic Microstructure (exposes divisor bug)` |
| 11 | Pair calculation | Equal Hexagonal_Low structures → compute angle. | *Not directly tested. No fixture selects Hexagonal_Low.* |
| 12 | Pair calculation | Different phase IDs resolve to the same hexagonal structure → accept the pair. | *Not directly tested. Each fixture has only one hexagonal phase ID.* |
| 13 | Pair calculation | Unequal structures or a non-hexagonal focal structure → NaN and divisor decrement. | `Class 1 - Realistic Microstructure (exposes divisor bug)`; `Class 1 - Mismatch Last Order`; `Class 4 - Invariants` |
| 14 | Angle folding | Angle exceeds 90° → replace it with 180° minus the angle. | *Not directly tested. Fixture tilt differences do not exceed 30°.* |
| 15 | Numerical guard | Cosine outside [-1, 1] → clamp to an endpoint. | *Not directly tested. No fixture forces cosine saturation.* |
| 16 | Average | At least one accepted neighbor → divide the sum by the accepted count. | All three `Class 1` tests; `Class 4 - Invariants` |
| 17 | Average | Nonempty list has no accepted neighbor → NaN. | `Class 1 - Realistic Microstructure (exposes divisor bug)` — F3; `Class 4 - Invariants` |
| 18 | Average | Empty neighbor list → empty output list and NaN average. | *Not directly tested. Fixtures include empty lists but do not assert both outputs for them.* |
| 19 | Cancellation | Cancel before processing a feature → return. | *Not directly tested. No fixture injects cancellation.* |
| 20 | Input | Crystal Structures bulk read fails → return the error. | *Not directly tested. No fixture injects this read failure.* |
| 21 | Input | Feature Phases bulk read fails → return the error. | *Not directly tested. No fixture injects this read failure.* |
| 22 | Input | Average Quaternions bulk read fails → return the error. | *Not directly tested. No fixture injects this read failure.* |
| 23 | Output | Average bulk write fails → return the error. | *Not directly tested. No fixture injects a write failure.* |
| 24 | Feature loop | No feature after background → skip the loop. | *Not directly tested. Every fixture has non-background features.* |

## Test inventory

All names are in `src/Plugins/OrientationAnalysis/test/ComputeFeatureNeighborCAxisMisalignmentsTest.cpp`. The seven ordinary test cases cover the in-core configuration. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check filter identity, comments, and seven converted parameters. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Simple Hex Pair` | new-for-V&V | Checks both directed pair angles and both averages against 10°, with 0.001° tolerance. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Realistic Microstructure (exposes divisor bug)` | new-for-V&V | Checks 18 list entries and six averages for mixed hexagonal/cubic structures. F2, F5, and F6 expose D1; F3 must have NaN entries and average. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 1 - Mismatch Last Order` | new-for-V&V | Checks F1 values [5°, 10°, NaN] and average 7.5° when the excluded neighbor is last. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Class 4 - Invariants` | new-for-V&V | Three sections check the [0°, 90°] range, the mean of non-NaN entries, and NaN outputs for a non-hexagonal focal feature. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Preflight Error - Feature array tuple count mismatch (-1560)` | kept | Checks error `-1560` for eight neighbor/quaternion tuples and seven phase tuples. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: genuine HDF5 1MiB-chunk tail oracle` | new-for-V&V | Hidden (`[.OocStoreContract]`). Requires HDF5 stores, checks a 65,536-tuple quaternion chunk shape, and asserts 15° and 30° pair angles and averages at the boundary and tail. |
| `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Phase Index Bounds` | kept | Uses phase values -1 and 2 with two ensemble tuples. Checks focal and neighbor errors `-1564`, and accepts an invalid unused background phase. |

## Exemplar archive

- **Archive:** `compute_feature_neighbor_caxis_misalignments.tar.gz` — still registered; no current test consumes it.
- **SHA512:** `955cd35b7ae24579ef9c533df34e1118012a8e5e2a71f8613117c714fc220c5dfa78d91a2964b41752e70684b79d4aa790e488e9a7be4c9dcf7b642ee2897ceb`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` for the root cause, affected users, and recommendation for each deviation. Part 1 contains these source-confirmed differences.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureNeighborCAxisMisalignmentsFilter-D1` | SIMPLNX counts all accepted neighbors in the average divisor; DREAM.3D v6.5.171 resets the divisor inside the neighbor loop. |
| `ComputeFeatureNeighborCAxisMisalignmentsFilter-D4` | SIMPLNX uses double-precision Eigen and EbsdLib calculations instead of the float matrix calculations in DREAM.3D v6.5.171. |
| `ComputeFeatureNeighborCAxisMisalignmentsFilter-D6` | SIMPLNX calculates angles for Hexagonal_Low pairs; DREAM.3D v6.5.171 writes NaN for these pairs. |
