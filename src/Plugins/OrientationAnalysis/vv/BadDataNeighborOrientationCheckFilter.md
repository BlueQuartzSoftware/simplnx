# V&V Report: BadDataNeighborOrientationCheckFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| SIMPLNX Human Name | Neighbor Orientation Comparison (Bad Data) |
| DREAM.3D v6.5.171 equivalent | `BadDataNeighborOrientationCheck` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-02-02; second engineer: Michael Jackson, 2026-06-01 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `BadDataNeighborOrientationCheck` in DREAM.3D v6.5.171. Worklist and Scanline preserve the neighbor rule with the documented defect corrections. |
| Oracle (confirmed) | Class 1 expected masks and Class 4 invariants are encoded in `test/BadDataNeighborOrientationCheckTest.cpp`. CI covers 33 ordinary test cases. All pass in CI. |
| Code paths enumerated | 19 of 29 exercised by the listed tests. Ten paths have no direct fixtures. |
| Tests today | 35 ordinary test cases and 2 hidden template variants cover masks, propagation, invariants, input validation and SIMPL conversion. |
| Exemplar archive | `bad_data_neighbor_orientation_check_v2.tar.gz` supplies test inputs. Expected masks are defined in the test source. |
| Legacy comparison | Run against DREAM.3D v6.5.171 on 2026-09-17: 12 of 28 cases match. A local proof version matches all 28 cases. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`BadDataNeighborOrientationCheckFilter` changes bad mask values to good when enough good face neighbors have the same phase and a misorientation below the tolerance. Class 1 expected masks and Class 4 invariants check the result. The five documented defects below are fixed.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* Worklist and Scanline replace the repeated full-volume scans in DREAM.3D v6.5.171. Both implementations retain the neighbor rule and correct the five defects listed below.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Expected masks follow the strict angle limit, same-phase rule and repeated neighbor propagation; invariant checks require no loss of good voxels and an unchanged result after a second run.

*Encoded:* `src/Plugins/OrientationAnalysis/test/BadDataNeighborOrientationCheckTest.cpp` — `Case 1.1.1` through `Case 4` give 27 fixtures; `2D Image Fixture (3x3x1)`, `real HDF5 reverse-slice cascade oracle`, `Class 4 Invariants Sweep` and `Class 4 Idempotence` add analytical and invariant checks. CI covers 33 ordinary test cases. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-06-01, as recorded in the sign-off.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.1 contains the fix for `BadDataNeighborOrientationCheckFilter-D1`. DREAM3D-NX v7.4.1 contains the fix for `BadDataNeighborOrientationCheckFilter-D2`. DREAM3D-NX v7.4.2 contains the fix for `BadDataNeighborOrientationCheckFilter-D3`. DREAM3D-NX v7.4.2 contains the fix for `BadDataNeighborOrientationCheckFilter-D4`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `BadDataNeighborOrientationCheckFilter-D1` | The convergence loop omitted the requested minimum level, so eligible bad voxels stayed bad. | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was affected. | Both implementations include `currentLevel == NumberOfNeighbors`. |
| `BadDataNeighborOrientationCheckFilter-D2` | An angle from a previous neighbor could cause a different-phase neighbor to count as a match. | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was affected. | Both implementations test the angle inside the same-phase condition. |
| `BadDataNeighborOrientationCheckFilter-D3` | Float conversion of the angle tolerance could admit a neighbor at the strict boundary. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter converts degrees to radians with double precision. |
| `BadDataNeighborOrientationCheckFilter-D4` | Invalid crystal-structure values could index outside the orientation-operator array. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter rejects invalid Laue indices with `-54901` and skips the supported unknown-structure sentinel. |
| `BadDataNeighborOrientationCheckFilter-D5` | An invalid phase on a bad voxel could index outside Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter skips nonpositive phases and returns `-54902` for a participating phase beyond the ensemble array. |

## Code path coverage

19 of 29 paths exercised by the listed tests. The table maps source paths to existing fixtures and gives the reason for each of the ten paths without a fixture.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.cpp` (37 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheckWorklist.cpp` (261 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheckScanline.cpp` (381 lines), and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.cpp` (187 lines).

Worklist counts matching neighbors and propagates changes through a queue. Scanline recounts neighbors in rolling slices until each level is stable.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Neighbor check | Mask is already good → skip | `Case 1.1.1`; `2D Image Fixture (3x3x1)` |
| 2 | Neighbor check | Neighbor has a different phase → do not count | `Case 1.2.2`; `Case 4` |
| 3 | Neighbor check | Face lies outside the image, including Z faces in a one-slice image → skip | `2D Image Fixture (3x3x1)`; `real HDF5 reverse-slice cascade oracle` |
| 4 | Neighbor check | Good neighbor has the same phase and angle below tolerance → count | `Case 1.1.1`; `2D Image Fixture (3x3x1)` |
| 5 | Neighbor check | Angle is at or above tolerance → do not count | `Case 1.3.3`; `real HDF5 reverse-slice cascade oracle` |
| 6 | Propagation | Count reaches the current level → flip and propagate | `Case 2.1` through `Case 2.6`; `Case 3.1`; `Case 3.2` |
| 7 | Neighbor check | Positive phase resolves to the unknown-structure sentinel → skip | *Not directly tested. No fixture assigns a participating positive phase to the sentinel.* |
| 8 | Scanline | Reverse propagation crosses five slices and repeats passes | `real HDF5 reverse-slice cascade oracle` — UInt8 and Bool variants assert all 45 mask values |
| 9 | Validation | Participating phase exceeds the ensemble bounds → error `-54902` | `Phase Index Bounds` — bad center voxel variant |
| 10 | Preflight | Cell tuple counts differ → error `-6809` | `Preflight Error - Cell array tuple count mismatch (-6809)` |
| 11 | Cancellation | Stop during Worklist initialization, queue construction, propagation or between levels; stop during a Scanline slice pass | *Not directly tested. No fixture injects a cancellation signal.* |
| 12 | Dispatch | Select Worklist on arrays in memory | `Case 1.1.1` through `Case 4` — algorithm scenario harness and exact masks |
| 13 | Dispatch | Select Scanline on arrays in memory or HDF5 stores | `Case 1.1.1` through `Case 4` — scenario harness; `real HDF5 reverse-slice cascade oracle` — dispatch counter |
| 14 | Neighbor check | Bad voxel has phase zero → skip | `real HDF5 reverse-slice cascade oracle` — background remains bad |
| 15 | Neighbor check | Bad voxel has a negative phase → skip | *Not directly tested. Fixtures do not assign negative phases.* |
| 16 | Neighbor check | Neighbor is bad → do not count it as good | `2D Image Fixture (3x3x1)` — corners stay bad |
| 17 | Propagation | Count is below the current level → leave the mask unchanged | `Case 1.3.2`; `2D Image Fixture (3x3x1)` |
| 18 | Validation | Invalid phase belongs to an already good voxel → skip it | `Phase Index Bounds` — good corner voxel variant checks successful execution only |
| 19 | Validation | Mask cannot be loaded → error `-54900` | *Not directly tested. This requires a direct algorithm call that bypasses parameter validation.* |
| 20 | Validation | Crystal structure is outside the Laue range and is not the sentinel → error `-54901` | *Not directly tested. No fixture supplies an invalid non-sentinel crystal structure.* |
| 21 | Worklist | Discard a duplicate queue entry or an entry below the current level | *Not directly tested. No fixture isolates the queue guard.* |
| 22 | Worklist | Recheck phase and Laue bounds after a flip | *Not directly tested. Preflight restricts the threshold to 1 through 6. No fixture bypasses preflight to isolate these guards.* |
| 23 | Scanline | Read and write UInt8 mask slices | `real HDF5 reverse-slice cascade oracle` — UInt8 variant |
| 24 | Scanline | Convert Bool mask slices for bulk reads and writes | `real HDF5 reverse-slice cascade oracle` — Bool variant |
| 25 | Scanline | Return an ensemble, quaternion, phase or mask I/O error | *Not directly tested. No fixture injects a store read or write failure.* |
| 26 | Scanline | Use fallback per-element mask I/O | *Not directly tested. Accepted masks use the UInt8 or Bool bulk paths.* |
| 27 | Propagation | Requested level exceeds six → no propagation | *Not directly tested. Preflight rejects a neighbor requirement above six. No fixture bypasses preflight to isolate this loop guard.* |
| 28 | Preflight | Neighbor requirement is outside 1 through 6 → error `-6810` | `Preflight Error - Number of neighbors out of range (-6810)` — values 7, 0 and -1 |
| 29 | Preflight | Quaternions, phases and mask have equal tuple counts that differ from the selected geometry's cell count → error `-6810` | `Geometry cell tuple counts` — all three arrays have 4 or 12 tuples for 8 cells |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.1` | kept | Class 1. Neighbor requirement 1; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.2` | kept | Class 1. Neighbor requirement 1; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.1.3` | kept | Class 1. Neighbor requirement 1; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.1` | kept | Class 1. Neighbor requirement 2; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.2` | kept | Class 1. Neighbor requirement 2; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.2.3` | kept | Class 1. Neighbor requirement 2; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.1` | kept | Class 1. Neighbor requirement 3; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.2` | kept | Class 1. Neighbor requirement 3; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.3.3` | kept | Class 1. Neighbor requirement 3; compares all 27 mask values with a literal array. Center stays bad at the tolerance boundary (BadDataNeighborOrientationCheckFilter-D3). |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.1` | kept | Class 1. Neighbor requirement 4; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.2` | kept | Class 1. Neighbor requirement 4; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.4.3` | kept | Class 1. Neighbor requirement 4; compares all 27 mask values with a literal array. Center stays bad at the tolerance boundary (BadDataNeighborOrientationCheckFilter-D3). |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.1` | kept | Class 1. Neighbor requirement 5; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.2` | kept | Class 1. Neighbor requirement 5; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.5.3` | kept | Class 1. Neighbor requirement 5; compares all 27 mask values with a literal array. Center stays bad at the tolerance boundary (BadDataNeighborOrientationCheckFilter-D3). |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.1` | kept | Class 1. Neighbor requirement 6; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.2` | kept | Class 1. Neighbor requirement 6; compares all 27 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 1.6.3` | kept | Class 1. Neighbor requirement 6; compares all 27 mask values with a literal array. Center stays bad at the tolerance boundary (BadDataNeighborOrientationCheckFilter-D3). |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.1` | kept | Class 1. Neighbor requirement 5; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.2` | kept | Class 1. Neighbor requirement 5; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.3` | kept | Class 1. Neighbor requirement 5; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.4` | kept | Class 1. Neighbor requirement 5; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.5` | kept | Class 1. Neighbor requirement 5; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 2.6` | kept | Class 1. Neighbor requirement 5; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.1` | kept | Class 1. Neighbor requirement 1; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 3.2` | kept | Class 1. Neighbor requirement 1; requires every mask value to be 1 after propagation. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Case 4` | kept | Class 1. Neighbor requirement 4; compares all 125 mask values with a literal array. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Preflight Error - Cell array tuple count mismatch (-6809)` | kept | Checks preflight error `-6809` for 9 phase tuples and 10 mask/quaternion tuples. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Preflight Error - Number of neighbors out of range (-6810)` | kept | Checks preflight error `-6810` for neighbor requirements 7, 0 and -1. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: SIMPL Backwards Compatibility` | kept | Checks UUID, comments, scalar values and array paths in `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` dynamic sections. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Invariants Sweep` | new-for-V&V | Checks no loss of good voxels and a nondecreasing good-voxel count in 18 dynamic sections, one for each Case 1 fixture. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Class 4 Idempotence` | new-for-V&V | Runs Case 4 twice; compares every value in the second mask with the first. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: 2D Image Fixture (3x3x1)` | new-for-V&V | Compares all 9 values with `[0,1,0,1,1,1,0,1,0]`; the center flips and the corners remain bad. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Phase Index Bounds` | kept | Checks error `-54902` for an invalid phase on the bad center and success when it is on a good corner (BadDataNeighborOrientationCheckFilter-D5). |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: Geometry cell tuple counts` | new-for-V&V | Checks `-6810` when quaternions, phases and mask each have 4 or 12 tuples for 8 cells. Checks `-6809` when only one array has 4 tuples. Checks diagnostic paths and counts, and accepts 8 tuples per array outside Cell Data. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: real HDF5 reverse-slice cascade oracle` — `uint8` template variant | new-for-V&V | Hidden. Requires HDF5-OOC input stores and Scanline dispatch; compares all 45 mask values with the central-column oracle. A 30° control stays bad. |
| `OrientationAnalysis::BadDataNeighborOrientationCheckFilter: real HDF5 reverse-slice cascade oracle` — `bool` template variant | new-for-V&V | Hidden. Requires HDF5-OOC input stores and Scanline dispatch; compares all 45 mask values with the central-column oracle. A 30° control stays bad. |

## Exemplar archive

- **Archive:** `bad_data_neighbor_orientation_check_v2.tar.gz`
- **SHA512:** `c311d636f56027da8f3b665375005230be83bb9060aed29dd1aada928d7afbce89d7be845626139c19025a89aaf1ac52b099c8efb8b99f246fc0bfad3c4ce128`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/BadDataNeighborOrientationCheckFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/BadDataNeighborOrientationCheckFilter.md` for the root cause, affected users, and recommendation for each deviation. The comparison on 2026-09-17 covers the 27 analytical fixtures and Small IN100. Twelve cases match; 16 differ in Mask. Phases, Quats and Crystal Structures match in all 28 cases.

| Deviation | Observed difference |
|-----------|---------------------|
| `BadDataNeighborOrientationCheckFilter-D1` | SIMPLNX includes the requested minimum neighbor level; DREAM.3D v6.5.171 omits it. |
| `BadDataNeighborOrientationCheckFilter-D2` | SIMPLNX rejects different-phase neighbors without reusing an earlier angle; DREAM.3D v6.5.171 permits stale-angle counts. A local proof version isolates the phase-gating correction. |
