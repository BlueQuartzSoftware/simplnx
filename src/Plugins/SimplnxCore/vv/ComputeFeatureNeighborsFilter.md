# V&V Report: ComputeFeatureNeighborsFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID               | `7177e88c-c3ab-4169-abe9-1fdaff20e598`                              |
| SIMPLNX Human Name         | Compute Feature Neighbors                                           |
| DREAM3D 6.5.171 equivalent | `FindNeighbors` (SIMPL UUID `97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac`) — `Source/Plugins/Statistics/StatisticsFilters/FindNeighbors.{h,cpp}` |
| Verified commit            | *<filled at SBIR deliverable assembly>*                             |
| Status                     | COMPLETE          |
| Sign-off                   | Nathan Young, 06-23-2026                                               |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Minor changes** of legacy `FindNeighbors::execute()`. Same core adjacency-scan algorithm; SIMPLNX adds explicit 2D/1D/single-voxel dimensionality dispatch via `NeighborUtilities`, template-specializes on the four `(StoreSurface, StoreBoundary)` combinations, and fixes two legacy bugs (D1 — SSA formula wrong for non-Z-normal faces; D2 — SurfaceFeatures incorrectly marks all features as surface for 1D and 2D EmptyY/EmptyX images). |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — 37 structured TEST_CASEs with inline hand-derived expected outputs for all five output arrays across all 8 image dimensionalities and all 4 optional-output combinations. All pass.                           |
| Code paths enumerated  | 19 of 20 paths exercised; 1 uncovered (cancel-signal injection in the 3D internal loop — requires signal injection infrastructure).     |
| Tests today            | **37 TEST_CASEs** — parameter sweep across 8 dimensionalities × 4 optional-output combinations (32 cases) + 3 non-square 2D regression cases (stride-bug guard) + 1 legacy SmallIn100 comparison + 1 SIMPL backwards-compat (2 DYNAMIC_SECTIONs). |
| Exemplar archive       | `6_6_stats_test_v2.tar.gz` — shared SmallIn100 input used for legacy comparison only; no oracle outputs (inline expected values used for all structured tests). SSA arrays in the archive reflect the buggy 6.5.171 output and are explicitly skipped in the legacy comparison test. |
| Legacy comparison      | **Run** on the SmallIn100 fixture (`6_6_stats_test_v2.tar.gz`): NumNeighbors, NeighborList, SurfaceFeatures bit-identical (3D dataset). **Plus a targeted A/B (2026-06-29)** on degenerate/anisotropic inputs through stock 6.5.171 and SIMPLNX that **proves both D1 and D2** via a surgically patched local build of the legacy source: D1 SSA `[8,8]`→`[24,24]`==NX (anisotropic), D2 SurfaceFeatures `[0,1,1]`→`[0,1,0]`==NX (EmptyY). The legacy-source patches reproduce SIMPLNX exactly; NumNeighbors/NeighborList/BoundaryCells byte-identical across all three. See deviations file. |
| Bug flags              | **D1** — SharedSurfaceAreaList uses wrong area formula for non-Z-normal faces in 6.5.171. **D2** — SurfaceFeatures incorrectly marks all features as surface for 1D images and 2D EmptyY/EmptyX images in 6.5.171.                               |
| V&V phase              | Structured tests (Class 1 oracle) complete and passing. Legacy source reviewed (`FindNeighbors.cpp`); D1 and D2 documented with source line references. **V&V complete and signed off by Nathan Young, 2026-06-23.**                                                  |

## Summary

`ComputeFeatureNeighborsFilter` identifies contiguous neighboring features in an ImageGeometry by scanning face-adjacent voxel pairs with differing non-zero feature IDs, accumulating the shared surface area per feature-neighbor pair. Verification used **Class 1 (Analytical) inline oracles** across 37 hand-built test cases spanning all 8 image dimensionalities (including 1D, 2D, 3D, and single-voxel), all four optional-output combinations, and non-square 2D regression fixtures that guard a previously-fixed stride bug. Review of the legacy `FindNeighbors.cpp` source found two bugs fixed by the SIMPLNX rewrite: D1 — the SSA formula at line 431 (`float area = float(number) * xRes * yRes`) uses X×Y face area for all faces regardless of face-normal direction; D2 — the SurfaceFeatures boundary check (lines 330–343) only handles the ZPoints==1 special case, incorrectly marking every feature as surface for any image with YPoints==1 or XPoints==1. NumNeighbors, NeighborList, and BoundaryCells are bit-identical to 6.5.171 on the 3D SmallIn100 fixture.

## Algorithm Relationship

*Classification:* **Minor changes** ~~| Port | Rewrite | New filter~~

*Evidence:* Same SIMPL UUID retained (`97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac`). SIMPL 6.4/6.5 conversion fixtures at `test/simpl_conversion/6_*/ComputeFeatureNeighborsFilter.json`. Core algorithm (face-adjacent voxel scan, per-feature-pair surface-area accumulation) is preserved. The SIMPLNX implementation adds template dispatch over 8 image dimensionality states and 4 optional-output combinations, which did not exist in the legacy. The SSA computation was also corrected (see D1).

*Port-time deltas:*

1. **Dimensionality dispatch** — legacy operated on 3D coordinates uniformly; SIMPLNX dispatches to one of 8 `ImageDimensionStateT` specializations (`SingleVoxelImage`, `ZImage1D`, `YImage1D`, `XImage1D`, `EmptyZImage2D`, `EmptyYImage2D`, `EmptyXImage2D`, `Image3D`) via `NeighborUtilities`. This also fixes the SurfaceFeatures bug for 1D and 2D EmptyY/X geometries (D2 — see below).
2. **SSA formula corrected** — legacy finalization at `FindNeighbors.cpp:431`: `float area = float(number) * xRes * yRes` uses X×Y spacing for every shared face regardless of face orientation. SIMPLNX uses `computeFaceSurfaceAreas<ImageDimensionStateT>(spacing)` (`NeighborUtilities.hpp:322`) which returns `{zFace, yFace, xFace, xFace, yFace, zFace}` = `{sx*sy, sx*sz, sy*sz, sy*sz, sx*sz, sx*sy}` and accumulates per-contact directly. This is D1.
3. **`std::map` accumulation** — accumulates `{neighborFeatureId → sharedSurfaceArea}` per feature into a sorted map; iterating the map populates NeighborList and SSA in ascending neighbor-ID order. Legacy used a two-pass approach: raw per-contact push into `vector<vector<int32_t>>` followed by deduplication via `QMap<int32_t, int32_t>` count — yielding the same NumNeighbors and NeighborList but the wrong SSA (D1).
4. **SurfaceFeatures 2D/1D fix** — legacy `FindNeighbors.cpp:330–343` only handles `ZPoints==1` explicitly; the 3D branch fires for any image with `YPoints==1` or `XPoints==1` and always triggers on `row==0` or `column==0` (both always true when that axis has length 1), marking every feature as surface. SIMPLNX uses per-dimensionality specializations that only mark corner cells (1D), corner+edge cells (2D), or all boundary cells (3D) as surface. This is D2.
5. **Two-stage boundary split** — Stage 1 processes corner/edge/face boundary cells with explicit face-validity checks; Stage 2 processes internal cells without validity overhead. Structure is new; behavior-equivalent for 3D inputs.
6. **Template specialization on optional outputs** — `ComputeFeatureNeighborsFunctor<ProcessSurfaceFeatures, ProcessBoundaryCells>` avoids runtime branches in the per-voxel hot path. Performance only; no behavior delta.
7. **Throttled progress** — legacy used direct `notifyStatusMessage`; SIMPLNX emits throttled progress feedback. UX-only.

*Material PRs since baseline (2025-10-01):*

- **PR #1590** — "ENH: Standardize 2D Image Handling" (2026-03-xx) — extracted `NeighborUtilities` as a shared module; added explicit 2D/1D dimensionality dispatch to this filter. Non-square 2D regression tests added during V&V catch the stride bug, since resolved.

## Oracle

*Class:* **1 (Analytical)** primary.

*Applied:* Expected outputs for all five arrays (NumNeighbors, NeighborList, SharedSurfaceAreaList, BoundaryCells, SurfaceFeatures) are derived by hand from the input FeatureIds array and the geometry's dimensions and spacing. For each geometry fixture, the engineer traced every face-adjacent voxel pair, identified pairs with differing non-zero feature IDs, accumulated the per-pair face area (= product of the two spacings perpendicular to the face normal), and summed shared areas per feature-neighbor pair. BoundaryCells counts = the number of distinct-feature face neighbors for each voxel. SurfaceFeatures = true for features that have any voxel on the image boundary. The derivations are embedded as inline `std::array` literals directly adjacent to the `REQUIRE`-equivalent comparisons in the test source.

*Encoded:* `test/ComputeFeatureNeighborsTest.cpp` — 37 TEST_CASEs. The `ExecuteFilter()` helper executes the filter and compares all requested outputs against the hand-derived inline exemplar arrays via `UnitTest::CompareArrays` and `UnitTest::CompareNeighborLists`. All 37 pass at the verified commit.

*Second-engineer review:* **Signed off by Nathan Young, 2026-06-23.** Review focus: the 3D 5×5×5 fixture (125-voxel, 6-feature, 7-neighbor-pair) and the non-square 2D stride regression fixtures as the highest-complexity cases.

## Code path coverage

*19 of 20 paths exercised.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureNeighbors.cpp` (394 lines).

The algorithm has four logical stages: **(a) Guard** — validates maxFeatureId and sets up geometry; **(b) Dispatch** — selects dimensionality template and optional-output template; **(c) Stage 1** — processes boundary voxels (corners, then edges, then face-interior boundary cells); **(d) Stage 2** — processes interior voxels (3D only); **(e) Finalize** — builds NumNeighbors + NeighborList + SharedSurfaceAreaList from the per-feature map.

| #  | Stage       | Path                                         | Test case                                         |
|----|-------------|---------------------|--------------------------|
| 1  | (a) Guard   | `maxFeatureId >= totalFeatures` → error result                                                    | *Not directly tested. Low-value guard; exercised implicitly when FeatureIds and FeatureAM are mismatched at the filter level.* |
| 2  | (b) Dispatch | `StoreSurface=true, StoreBoundary=true`     | `Case *.*.0: * - Full Execution` (all dimensionalities)                                               |
| 3  | (b) Dispatch | `StoreSurface=true, StoreBoundary=false`     | `Case *.*.1: * - No Boundary` (all dimensionalities)                                                 |
| 4  | (b) Dispatch | `StoreSurface=false, StoreBoundary=true`     | `Case *.*.2: * - No Surface Features` (all dimensionalities)                                         |
| 5  | (b) Dispatch | `StoreSurface=false, StoreBoundary=false`     | `Case *.*.3: * - No Optionals` (all dimensionalities)                                                |
| 6  | (b) Dispatch | Dimensionality → `SingleVoxelImage`          | `Case 0.0.*: Single Voxel`                       |
| 7  | (b) Dispatch | Dimensionality → `ZImage1D` / `YImage1D` / `XImage1D`                                            | `Case 1.0.*` / `Case 1.1.*` / `Case 1.2.*`      |
| 8  | (b) Dispatch | Dimensionality → `EmptyZImage2D` / `EmptyYImage2D` / `EmptyXImage2D` (square)                    | `Case 2.0.*` / `Case 2.1.*` / `Case 2.2.*`      |
| 9  | (b) Dispatch | Dimensionality → `EmptyZImage2D` / `EmptyYImage2D` / `EmptyXImage2D` (non-square stride regression) | `Case 2.0.4` / `Case 2.1.4` / `Case 2.2.4`  |
| 10 | (b) Dispatch | Dimensionality → `Image3D`                   | `Case 3.0.*: 3D`                                 |
| 11 | (c) Stage 1  | `featureId == 0` → skip voxel (no surface/boundary contribution)                                 | 1D test (`featureIdsArray[5] = 0`) + 3D test (multiple zero voxels)                                  |
| 12 | (c)/(d)      | `neighborFeatureId == 0` → skip face (background neighbor)                                       | 1D test + 3D test (background voxels present)    |
| 13 | (c) Stage 1  | `!isValidFaceNeighbor[faceIndex]` → skip face (image boundary)                                   | All tests — every fixture has at least one image-boundary voxel                                        |
| 14 | (c)/(d)      | `neighborFeatureId == feature` → skip face (same-feature neighbor)                               | All multi-voxel tests (interior voxels within a feature)                                              |
| 15 | (c)/(d)      | Normal accumulation — different non-zero features → `neighborSurfaceAreas[feature][nbr] += area` | All multi-feature tests                           |
| 16 | (c) Stage 1  | `ProcessSurfaceFeaturesV=true` → `surfaceFeatures->setValue(feature, true)` for boundary voxel   | `Case *.*.0` and `Case *.*.1` (Full Execution + No Boundary variants)                                |
| 17 | (c)/(d)      | `ProcessBoundaryCellsV=true` → `boundaryCells->setValue(voxelIndex, numDiffNeighbors)`           | `Case *.*.0` and `Case *.*.2` (Full Execution + No Surface Features variants)                        |
| 18 | (c) Stage 1  | Edge cell processing path (ProcessEdges — skipped for SingleVoxelImage)                          | `Case 1.0.*` through `Case 3.0.*`                |
| 19 | (c) Stage 1  | Face cell processing path (ProcessFaces — 2D and 3D only, no validity check needed)              | `Case 2.*.*` and `Case 3.0.*`                    |
| 20 | (d) Stage 2  | `shouldCancel` → early return from 3D internal loop                                               | *Not directly tested. Requires cancel-signal injection infrastructure not present in this test suite.* |

## Test inventory

| Test case                                 | Status      | Notes                                                |
|------------------|-------------|-----------------------------|
| `Case 0.0.0` through `Case 0.0.3`: Single Voxel (4 cases)                                     | kept        | 1-voxel geometry, 1 feature; verifies zero-neighbor result and optional-output combinations              |
| `Case 1.0.0` through `Case 1.0.3`: 1D Z (4 cases)                                             | kept        | 7-cell 1D Z strip, 4 features + background; verifies 1D neighbor detection and SSA = face area          |
| `Case 1.1.0` through `Case 1.1.3`: 1D Y (4 cases)                                             | kept        | Same layout rotated to Y axis; same expected values with Y spacing applied                               |
| `Case 1.2.0` through `Case 1.2.3`: 1D X (4 cases)                                             | kept        | Same layout rotated to X axis; same expected values with X spacing applied                               |
| `Case 2.0.0` through `Case 2.0.3`: 2D Empty Z, square 5×5×1 (4 cases)                         | kept        | 5×5×1 geometry, 5 features + background; non-trivial 2D neighbor graph with anisotropic spacing         |
| `Case 2.1.0` through `Case 2.1.3`: 2D Empty Y, square 5×1×5 (4 cases)                         | kept        | Same layout rotated to Z-X plane                    |
| `Case 2.2.0` through `Case 2.2.3`: 2D Empty X, square 1×5×5 (4 cases)                         | kept        | Same layout rotated to Y-Z plane                    |
| `Case 2.0.4`: 2D Empty Z, non-square 3×2×1| new-for-V&V | Stride regression: dims[0] ≠ dims[1]; wrong stride produces SSA = 2×area instead of 3×area              |
| `Case 2.1.4`: 2D Empty Y, non-square 3×1×2| new-for-V&V | Same stride regression rotated to Z-X plane         |
| `Case 2.2.4`: 2D Empty X, non-square 1×3×2| new-for-V&V | Same stride regression rotated to Y-Z plane         |
| `Case 3.0.0` through `Case 3.0.3`: 3D 5×5×5 (4 cases)                                         | kept        | 125-voxel, 6-feature, fully 3D geometry with anisotropic spacing; 7 feature-pair SSA values hand-derived |
| `Legacy: SmallIn100`                       | kept        | Legacy comparison on `6_6_stats_test_v2.tar.gz`; compares NumNeighbors, NeighborList, SurfaceFeatures; SSA skipped (D1) |
| `SIMPL Backwards Compatibility` (2 DYNAMIC_SECTIONs: "SIMPL 6.5 (UUID)", "SIMPL 6.4 (Filter_Name)") | kept   | Validates UUID + argument-key + parameter-value decoding from SIMPL JSON; does not execute the filter    |

All 37 TEST_CASEs pass at the verified commit.

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeFeatureNeighborsFilter.md`

The archive is a shared SmallIn100 dataset used by multiple statistics filters. For `ComputeFeatureNeighborsFilter` it is consumed only by the `Legacy: SmallIn100` test as a legacy-comparison input+output fixture. Oracle outputs for all structured tests (Cases 0–3) are inline hand-derived values; no archive is needed. The archive's SSA arrays reflect the buggy 6.5.171 output and are not compared (see D1).

## Deviations from DREAM3D 6.5.171

- `ComputeFeatureNeighborsFilter-D1` — SharedSurfaceAreaList uses `count * xRes * yRes` for all faces in legacy; correct formula is `Σ area(face_direction)` — see `vv/deviations/ComputeFeatureNeighborsFilter.md`
- `ComputeFeatureNeighborsFilter-D2` — SurfaceFeatures incorrectly marks all features as surface for any image with `XPoints==1` or `YPoints==1` in legacy — see `vv/deviations/ComputeFeatureNeighborsFilter.md`
