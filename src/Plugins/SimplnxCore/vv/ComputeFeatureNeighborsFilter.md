# V&V Report: ComputeFeatureNeighborsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `7177e88c-c3ab-4169-abe9-1fdaff20e598` |
| SIMPLNX Human Name | Compute Feature Neighbors |
| DREAM.3D v6.5.171 equivalent | `FindNeighbors` (SIMPL UUID `97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-06-23; second engineer: Michael Jackson, 2026-06-30 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindNeighbors` in DREAM.3D v6.5.171. SIMPLNX keeps face adjacency and corrects face areas and surface flags. |
| Oracle (confirmed) | Class 1 analytical expectations are encoded in 35 ordinary cases in `test/ComputeFeatureNeighborsTest.cpp`. All pass in CI. |
| Code paths enumerated | 24 of 34 exercised by the mapped tests. Error returns, cancellation, negative IDs and the unreachable dispatch fallback have no direct fixture. |
| Tests today | 38 test cases: 35 analytical cases, one archive regression, one SIMPL conversion case and one hidden HDF5 case. The analytical cases cover eight dimensionality states and all four optional-output combinations. |
| Exemplar archive | `6_6_stats_test_v2.tar.gz` supplies Small IN100 inputs and three regression reference arrays. Shared surface areas use independent inline expectations. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`ComputeFeatureNeighborsFilter` finds features that share voxel faces and sums their shared face areas. Class 1 fixtures check neighbor counts, neighbor lists, areas, boundary-cell counts and surface-feature flags. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `FindNeighbors::execute()` and the current Direct and Scanline implementations use distinct positive face-neighbor IDs; SIMPLNX adds dimensionality dispatch and the two corrections below.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Hand-specified FeatureIds define adjacency, face counts, direction-specific face areas and image-boundary contact in each active dimension.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeFeatureNeighborsTest.cpp::SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.0: Single Voxel - Full Execution` and the other `Case` tests encode 11 geometry fixtures in 35 cases. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-06-30.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `ComputeFeatureNeighborsFilter-D1` and `ComputeFeatureNeighborsFilter-D2`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureNeighborsFilter-D1` | Shared areas used X spacing times Y spacing for every face direction. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Each contact contributes the product of the two spacings perpendicular to its face normal. |
| `ComputeFeatureNeighborsFilter-D2` | A unit X or Y dimension caused interior features to be marked as surface features. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Only boundary contact in active dimensions sets the flag; a single voxel is a surface feature. |

## Code path coverage

24 of 34 paths exercised. Each uncovered path states why it has no direct fixture.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureNeighbors.cpp` (26 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureNeighborsDirect.cpp` (394 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureNeighborsScanline.cpp` (250 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureNeighborsFilter.cpp` (223 lines), and `src/simplnx/Utilities/NeighborUtilities.hpp` (634 lines).

Test names below omit the common `SimplnxCore::ComputeFeatureNeighborsFilter:` prefix. Direct separates boundary and interior cells; Scanline uses three rolling slices.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Validation | Maximum FeatureId exceeds the feature array bounds; return `-24500` | *Not directly tested. No fixture supplies an out-of-range positive ID.* |
| 2 | Optional outputs | Store both surface flags and boundary counts | `Case 0.0.0` through `Case 3.0.0` — Full Execution variants |
| 3 | Optional outputs | Store surface flags only | `Case 0.0.1` through `Case 3.0.1` — No Boundary variants |
| 4 | Optional outputs | Store boundary counts only | `Case 0.0.2` through `Case 3.0.2` — No Surface Features variants |
| 5 | Optional outputs | Store neither optional output | `Case 0.0.3` through `Case 3.0.3` — No Optionals variants |
| 6 | Dimensions | Single voxel | `Case 0.0.0` through `Case 0.0.3` |
| 7 | Dimensions | 1D along Z, Y or X | `Case 1.0.0`, `Case 1.1.0`, `Case 1.2.0` |
| 8 | Dimensions | Square 2D with unit Z, Y or X | `Case 2.0.0`, `Case 2.1.0`, `Case 2.2.0` |
| 9 | Dimensions | Non-square 2D with unit Z, Y or X | `Case 2.0.4`, `Case 2.1.4`, `Case 2.2.4` |
| 10 | Dimensions | Full 3D image | `Case 3.0.0` through `Case 3.0.3` |
| 11 | Cell scan | FeatureId zero contributes no contacts or surface flag | `Case 1.0.0`, `Case 3.0.0` |
| 12 | Neighbor scan | Neighbor ID zero contributes no face | `Case 1.0.0`, `Case 3.0.0` |
| 13 | Neighbor scan | Face outside the image is skipped | `Case 0.0.0`, `Case 3.0.0` |
| 14 | Neighbor scan | Same-feature face is skipped | `Case 1.0.0`, `Case 3.0.0` |
| 15 | Accumulation | Distinct positive neighbor adds its direction-specific face area | `Case 2.0.0`, `Case 2.1.0`, `Case 2.2.0`, `Case 3.0.0` |
| 16 | Surface flags | Mark boundary features and leave interior features false | `Case 1.0.0`, `Case 2.1.0`, `Case 2.2.0`, `Case 3.0.0` |
| 17 | Boundary counts | Write the number of distinct-feature face contacts for each cell | `Case 1.0.0`, `Case 3.0.0` |
| 18 | Direct boundary | Process edge cells; omit them for a single voxel | `Case 0.0.0`, `Case 1.0.0`, `Case 2.0.0`, `Case 3.0.0` |
| 19 | Direct boundary | Process face interiors in 2D and 3D | `Case 2.0.0`, `Case 3.0.0` |
| 20 | Cancellation | Return during the Direct interior sweep or Scanline slice loop | *Not directly tested. No fixture injects cancellation.* |
| 21 | Scanline | Read first and second slices, rotate buffers, then read later slices | `Case 3.0.0` — in-memory algorithm scenarios |
| 22 | Direct validation | Missing surface store returns `-789620` | *Not directly tested. Normal filter execution creates the requested output.* |
| 23 | Direct validation | Missing boundary store returns `-789621` | *Not directly tested. Normal filter execution creates the requested output.* |
| 24 | Scanline I/O | First input-slice read fails | *Not directly tested. Requires an injected store read failure.* |
| 25 | Scanline I/O | Second input-slice read fails | *Not directly tested. Requires an injected store read failure.* |
| 26 | Scanline I/O | Later input-slice read fails after buffer rotation | *Not directly tested. Requires an injected store read failure.* |
| 27 | Scanline I/O | Boundary-slice write fails | *Not directly tested. Requires an injected store write failure.* |
| 28 | Cell scan | Negative feature or neighbor ID contributes no contact | *Not directly tested. Fixtures use nonnegative IDs.* |
| 29 | Finalize | Write sorted neighbor IDs, counts and areas; isolated features have empty lists | `Case 1.0.0`, `Case 3.0.0` |
| 30 | Dispatch | Select Direct and assert output values | `Case 0.0.0` through `Case 3.0.3` — in-memory algorithm scenarios |
| 31 | Dispatch | Select Scanline and assert output values | `Case 0.0.0` through `Case 3.0.3` — in-memory algorithm scenarios |
| 32 | Preflight | Create required outputs and selected optional outputs | `Case 0.0.0` through `Case 0.0.3` — preflight succeeds and execution checks requested outputs |
| 33 | Direct interior | Accumulate all six faces without image-boundary guards | `Case 3.0.0` |
| 34 | Direct dispatch | Return after no dimensionality specialization matches | *Not directly tested. The eight dimensionality states exhaust all unit-axis combinations.* |

## Test inventory

The 37 ordinary test cases cover analytical expectations, archive regression and conversion. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeFeatureNeighborsFilter: real HDF5 five-slice analytical oracle` | new-for-V&V | Hidden. Reuses the 5×5×5 analytical fixture; checks all five outputs, HDF5 stores and the selected algorithm counter. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.0: Single Voxel - Full Execution` | kept | One voxel: zero neighbors, empty lists and a surface feature. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.1: Single Voxel - No Boundary` | kept | One voxel: zero neighbors, empty lists and a surface feature. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.2: Single Voxel - No Surface Features` | kept | One voxel: zero neighbors, empty lists and a surface feature. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 0.0.3: Single Voxel - No Optionals` | kept | One voxel: zero neighbors, empty lists and a surface feature. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.0: 1D Z - Full Execution` | kept | Seven cells along Z: two neighbor pairs and one isolated feature. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.1: 1D Z - No Boundary` | kept | Seven cells along Z: two neighbor pairs and one isolated feature. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.2: 1D Z - No Surface Features` | kept | Seven cells along Z: two neighbor pairs and one isolated feature. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.0.3: 1D Z - No Optionals` | kept | Seven cells along Z: two neighbor pairs and one isolated feature. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.0: 1D Y - Full Execution` | kept | Seven cells along Y: two neighbor pairs and one isolated feature. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.1: 1D Y - No Boundary` | kept | Seven cells along Y: two neighbor pairs and one isolated feature. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.2: 1D Y - No Surface Features` | kept | Seven cells along Y: two neighbor pairs and one isolated feature. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.1.3: 1D Y - No Optionals` | kept | Seven cells along Y: two neighbor pairs and one isolated feature. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.0: 1D X - Full Execution` | kept | Seven cells along X: two neighbor pairs and one isolated feature. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.1: 1D X - No Boundary` | kept | Seven cells along X: two neighbor pairs and one isolated feature. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.2: 1D X - No Surface Features` | kept | Seven cells along X: two neighbor pairs and one isolated feature. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 1.2.3: 1D X - No Optionals` | kept | Seven cells along X: two neighbor pairs and one isolated feature. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.0: 2D Empty Z - Full Execution` | kept | 5×5×1 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.1: 2D Empty Z - No Boundary` | kept | 5×5×1 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.2: 2D Empty Z - No Surface Features` | kept | 5×5×1 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.3: 2D Empty Z - No Optionals` | kept | 5×5×1 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.0: 2D Empty Y - Full Execution` | kept | 5×1×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.1: 2D Empty Y - No Boundary` | kept | 5×1×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.2: 2D Empty Y - No Surface Features` | kept | 5×1×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.3: 2D Empty Y - No Optionals` | kept | 5×1×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.0: 2D Empty X - Full Execution` | kept | 1×5×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.1: 2D Empty X - No Boundary` | kept | 1×5×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.2: 2D Empty X - No Surface Features` | kept | 1×5×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.3: 2D Empty X - No Optionals` | kept | 1×5×5 cells: areas 22.6, 6.8 and 23.8; two interior features. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.0: 3D - Full Execution` | kept | 125 cells: ten feature-pair areas, an isolated feature and an interior feature. Checks all five outputs. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.1: 3D - No Boundary` | kept | 125 cells: ten feature-pair areas, an isolated feature and an interior feature. Checks counts, lists, areas and surface flags. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.2: 3D - No Surface Features` | kept | 125 cells: ten feature-pair areas, an isolated feature and an interior feature. Checks counts, lists, areas and boundary counts. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 3.0.3: 3D - No Optionals` | kept | 125 cells: ten feature-pair areas, an isolated feature and an interior feature. Checks counts, lists and areas. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.0.4: 2D Empty Z - Non-Square {3,2,1}` | new-for-V&V | Six cells in two feature rows: checks one neighbor per feature, shared area 6, all boundary counts 1 and both surface flags true. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.1.4: 2D Empty Y - Non-Square {3,1,2}` | new-for-V&V | Six cells in two feature rows: checks one neighbor per feature, shared area 6, all boundary counts 1 and both surface flags true. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Case 2.2.4: 2D Empty X - Non-Square {1,3,2}` | new-for-V&V | Six cells in two feature rows: checks one neighbor per feature, shared area 6, all boundary counts 1 and both surface flags true. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: Legacy: SmallIn100` | kept | Compares SurfaceFeatures, NumNeighbors and NeighborList with the Small IN100 archive. SharedSurfaceAreaList and BoundaryCells are not compared. |
| `SimplnxCore::ComputeFeatureNeighborsFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check identity, comments and ten converted arguments. |

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeFeatureNeighborsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureNeighborsFilter.md` for the root cause, affected users and recommendation for each Part 1 entry.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureNeighborsFilter-D1` | Shared areas use each face direction's area instead of X spacing times Y spacing for all contacts. |
| `ComputeFeatureNeighborsFilter-D2` | Surface flags use only active dimensions; interior features in 1D and unit-X or unit-Y 2D images remain false. |
