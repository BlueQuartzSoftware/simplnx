# V&V Report: DBSCANFilter

<!-- Write the report in ASD-STE100 Simplified Technical English. Use short sentences, active voice, one meaning per sentence, and consistent technical terms. -->

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `763dad44-fad7-4606-808f-617867257b98` |
| SIMPLNX Human Name | DBSCAN |
| DREAM.3D v6.5.171 equivalent | `DBSCAN` (DREAM3DReview plugin, SIMPL UUID `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-08-07; second engineer: Michael Jackson, 2026-08-28 |

## At a glance

| Aspect   | Current state|
|----------|--------------|
| Algorithm Relationship | Rewrite. The legacy equivalent is `DBSCAN` in DREAM.3D v6.5.171. SIMPLNX uses grid-based DBSCAN (GDCF), so a core object is a grid cell, not a point. |
| Oracle (confirmed)     | Class 2 (scikit-learn 1.7.1 DBSCAN), with Class 1 and Class 4 companions. Encoded in 15 test cases in `test/DBSCANTest.cpp`, plus 1 hidden OOC test. All 15 pass in CI at `65a3038e`. |
| Code paths enumerated  | 20 of 23 exercised. The 3 uncovered paths are guarded by preflight or by a forest check, or have no isolating fixture. |
| Tests today            | 15 registered test cases plus 1 hidden OOC test: 6 two-dimensional sklearn datasets, 1 three-dimensional regression case, 4 analytical fixtures, 3 preflight rejections, 1 preflight allocation check and 1 SIMPL conversion test. |
| Exemplar archive       | `dbscan_test.tar.gz` gives inputs and outputs. The six 2D label arrays are regression fixtures that the sklearn oracle confirms. The 3D label array has no external oracle. |
| Legacy comparison      | Run against DREAM.3D v6.5.171 on the six 2D sklearn datasets. Four datasets match exactly. Two datasets differ as `DBSCAN-D1` predicts. |
| Bug flags| `DBSCAN-D4`. Open: `upgradeParametersImpl()` is not implemented, so SIMPLNX v1 pipelines lose a non-default parse order. |
| V&V phase| Complete. The OOC test suite was not run, because CI does not build the OOC configuration. |

## Summary

`DBSCANFilter` puts points into clusters by density. It uses the GDCF algorithm (Boonchoo et al. 2019), which defines core objects on grid cells. A Class 2 scikit-learn oracle verifies the filter, with Class 1 fixtures and Class 4 invariants. Four of six reference datasets match exactly, two datasets differ as designed (`DBSCAN-D1`), and the V&V found and fixed four SIMPLNX defects.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* PR #1421 (commit `c1bc9114b7`, 2025-09-04) replaced the point-by-point DBSCAN from PR #994 with GDCF.

## Oracle

*Class:* 2 (Reference — scikit-learn 1.7.1 `sklearn.cluster.DBSCAN`), with 1 (Analytical) and 4 (Invariant) companions.

*Applied:* scikit-learn ran on the six sklearn input datasets with the same epsilon and minimum points as the tests. The comparison uses the cluster count and the sorted cluster sizes. Hand-derived fixtures give exact labels for the paths that scikit-learn cannot reach. `CheckClusterInvariants()` checks that IDs are not negative, that IDs 1 to N have no gaps, and that the AttributeMatrix has N + 1 tuples.

*Encoded:* `src/Plugins/SimplnxCore/test/DBSCANTest.cpp` — 6 Class 2 regression fixtures (`2D Test: *`), 4 Class 1 fixtures (`Analytical Fixture F1`, `F2`, `F3`, `genuine HDF5 65536-batch tail oracle`) and Class 4 invariants in the 2D helpers and F1 to F3. All registered tests pass in CI at `65a3038e`.

*Second-engineer review:* *Michael Jackson, 2026-08-28*

## Bugs found and fixed

This branch fixes all defects in this table. PR #1702 fixed `DBSCAN-D5`, `DBSCAN-D7` and `DBSCAN-D8`, and DREAM3D-NX v7.4.2 contains those fixes. PR #1766 fixed `DBSCAN-D6`, and the fix will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|-----------|--------|----------------------------|---------------------------|
| `DBSCAN-D5` | The `SeededRandom` shuffle was biased. It never selected the last index, and it drew partners from the full range. | DREAM3D-NX v7.2.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. | `std::shuffle` replaces the hand-written shuffle. |
| `DBSCAN-D6` | The filter accepted a multi-component mask or a mask with fewer tuples than the input. The in-core algorithm then read wrong mask values or read past the end of the mask. | DREAM3D-NX v7.0.0 through v7.4.2. | Preflight rejects a mask with a component shape other than `{1}` (`-208`) and a mask with a different tuple count (`-7587`). |
| `DBSCAN-D7` | The `Random` parse order used the user seed, not a time-based seed. `Random` gave the same result as `SeededRandom` with the default seed. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The filter gives the generated seed to the algorithm. |
| `DBSCAN-D8` | A mask with no true values left the grid bounds as NaN. The algorithm then cast NaN to `usize`, which is undefined behavior. | DREAM3D-NX v7.2.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. | An explicit check returns warning `-85640` with a message about the mask. |

## Code path coverage

20 of 23 paths exercised. Path 3 is not reachable because preflight accepts only `{2}` and `{3}` component shapes. Path 14 has no fixture that isolates it. Path 18 is not reachable because the filter skips labeling when no forest exists.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/DBSCANDirect.cpp` (1273 lines) and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/DBSCANScanline.cpp` (3643 lines). `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/DBSCAN.cpp` (34 lines) selects one of the two.

The phases are (p) preflight, (a) grid construction, (b) core identification, (c) cluster merge, (d) expansion, (e) cleanup and label, and (f) external storage I/O.

| #  | Phase           | Path    | Test case      |
|----|-----------------|---------|----------------|
| 1  | (a) Grid | Input has 2 components → 2D grid | All `2D Test: *` cases |
| 2  | (a) Grid | Input has 3 components → 3D grid | `3D Test (LowDensityFirst)` |
| 3  | (a) Grid | Input has another component count → error `-54060` | *Not directly tested. Preflight rejects the input through `AllowedComponentShapes{{2},{3}}`.* |
| 4  | (a) Grid | Mask is enabled → the algorithm skips masked points in the bounds and in binning | `Analytical Fixture F2 - Mask Exclusion` |
| 5  | (a) Grid | No point passes the mask → warning `-85640` | `Analytical Fixture F3 - All Points Masked` |
| 6  | (b) Core | No grid has the minimum number of points → warning `-85640` | `Analytical Fixture F1 - No Clusters Warning` |
| 7  | (b) Core | `LowDensityFirst` → sort core grids by occupancy, ascending | All `2D Test: *` cases, `3D Test (LowDensityFirst)` |
| 8  | (b) Core | `Random` → shuffle with a time-based seed | All `2D Test: *` cases (Class 4 invariants only) |
| 9  | (b) Core | `SeededRandom` → shuffle with the user seed | All `2D Test: *` cases |
| 10 | (c) Merge | Neighbor grid is already in the same cluster → skip | All `2D Test: *` cases |
| 11 | (c) Merge | Neighbor grid is an unvisited border grid → join the current cluster | All `2D Test: *` cases |
| 12 | (c) Merge | Neighbor grid is a core grid or a visited border grid → union-find merge | All `2D Test: *` cases |
| 13 | (d) Expansion | Non-core grid has a density-reachable neighbor → join that cluster | `2D Test: Aniso` |
| 14 | (d) Expansion | Grid and neighbor are both unvisited border grids → skip | *Not directly tested. No fixture isolates two adjacent unvisited border grids.* |
| 15 | (e) Label | Root grid is a core grid → number the cluster 1 to N | All `2D Test: *` cases, `3D Test (LowDensityFirst)` |
| 16 | (e) Label | Root grid is not a core grid → label as noise (0) | `2D Test: Varied`, `2D Test: Aniso` |
| 17 | (e) Label | Write the cluster ID of each grid to its points | All `2D Test: *` cases, `3D Test (LowDensityFirst)` |
| 18 | (e) Label | `label()` runs with an empty forest → warning `-85640` | *Not directly tested. The filter does not call `label()` when `forestBuilt()` is false.* |
| 19 | (f) External | Input has a full 65,536-point batch and a partial tail batch | `genuine HDF5 65536-batch tail oracle` |
| 20 | (f) External | Adjacent core grids merge across a batch boundary | `genuine HDF5 65536-batch tail oracle` |
| 21 | (p) Preflight | Mask is disabled → no cell-sized temporary mask | `No-mask preflight does not create a cell-sized temporary mask` |
| 22 | (p) Preflight | Mask component shape is not `{1}` → error `-208` | `Reject multi-component mask in preflight` |
| 23 | (p) Preflight | Mask tuple count is not equal to the input tuple count → error `-7587` | `Reject short mask in preflight`, `Reject short StringArray mask in preflight` |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::DBSCAN: 2D Test: Aniso` | kept | Class 2 regression fixture. `LowDensityFirst` compares all labels with the exemplar. `SeededRandom` compares the cluster sizes. `Random` checks the Class 4 invariants only. Runs the Direct and Scanline algorithms. |
| `SimplnxCore::DBSCAN: 2D Test: Blobs` | kept | Same checks as `Aniso`. Matches scikit-learn exactly. |
| `SimplnxCore::DBSCAN: 2D Test: Noisy Circles` | kept | Same checks as `Aniso`. Matches scikit-learn exactly. |
| `SimplnxCore::DBSCAN: 2D Test: Noisy Moons` | kept | Same checks as `Aniso`. Matches scikit-learn exactly. |
| `SimplnxCore::DBSCAN: 2D Test: No Structure` | kept | Same checks as `Aniso`. Matches scikit-learn exactly. |
| `SimplnxCore::DBSCAN: 2D Test: Varied` | kept | Same checks as `Aniso`. Differs from scikit-learn as `DBSCAN-D1` predicts. |
| `SimplnxCore::DBSCAN: 3D Test (LowDensityFirst)` | kept | Compares all labels with the 3D exemplar and checks the AttributeMatrix tuple count. The exemplar has no external oracle. |
| `SimplnxCore::DBSCAN: Analytical Fixture F1 - No Clusters Warning` | new-for-V&V | Class 1. Four corner points, epsilon 0.1, minimum points 5. Expects warning `-85640` and all IDs 0. |
| `SimplnxCore::DBSCAN: Analytical Fixture F2 - Mask Exclusion` | new-for-V&V | Class 1. Three points, one masked, epsilon 1.0, minimum points 2. Expects IDs `[1, 1, 0]`. |
| `SimplnxCore::DBSCAN: Analytical Fixture F3 - All Points Masked` | new-for-V&V | Class 1. The F2 points with all points masked. Expects warning `-85640`, all IDs 0 and 1 AttributeMatrix tuple. Pins the `DBSCAN-D8` fix. |
| `SimplnxCore::DBSCAN: genuine HDF5 65536-batch tail oracle` | new-for-V&V | Hidden (`[.OocStoreContract]`); runs in the OOC build only. Class 1. Of 65,538 points, 5 are active. Four points form one cluster across the batch boundary, and all other labels are 0. |
| `SimplnxCore::DBSCANFilter: No-mask preflight does not create a cell-sized temporary mask` | kept | Checks the preflight actions when the mask is disabled. |
| `SimplnxCore::DBSCAN: Reject multi-component mask in preflight` | new-for-V&V | Mask component shape `{2}`. Expects `-208` with the mask enabled, and no error with the mask disabled. Pins the `DBSCAN-D6` fix. |
| `SimplnxCore::DBSCAN: Reject short mask in preflight` | new-for-V&V | Four input tuples and three mask tuples. Expects `-7587` with the mask enabled, and no error with the mask disabled. Pins the `DBSCAN-D6` fix. |
| `SimplnxCore::DBSCAN: Reject short StringArray mask in preflight` | new-for-V&V | A `StringArray` mask with a different tuple count. Expects a preflight error and no exception. |
| `SimplnxCore::DBSCANFilter: SIMPL Backwards Compatibility` | kept | Converts the 6.4 and 6.5 SIMPL pipeline fixtures. Checks the parameters only. |

## Exemplar archive

- **Archive:** `dbscan_test.tar.gz`
- **SHA512:** `77d7886e2550b63176b564e827d7de320b5a28b1c8a55bf107d53acd6962757275bc86b3382ff789f612a6838f55ab8f4af29435aec83a7a804b9487e57a6386`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/dbscan_test.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/DBSCANFilter.md` for the root cause, affected users, and recommendation for each deviation. DREAM.3D v6.5.171 ran on the six 2D sklearn datasets in `dbscan_test.tar.gz`.

| Deviation | Observed difference |
|-----------|---------------------|
| `DBSCAN-D1` | Sparse data can give fewer clusters and more noise points, because a core object is a grid cell and not a point. |
| `DBSCAN-D2` | Cluster ID numbers can be different, because `LowDensityFirst` is the new default parse order. |
| `DBSCAN-D3` | The filter ignores the legacy `use_precaching` parameter. |
