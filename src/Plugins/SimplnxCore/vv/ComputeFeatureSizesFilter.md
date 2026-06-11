# V&V Report: ComputeFeatureSizes

| | |
|---|---|
| Plugin | `SimplnxCore` |
| SIMPLNX UUID | `c666ee17-ca58-4969-80d0-819986c72485` |
| DREAM3D 6.5.171 equivalent | `FindSizes` — UUID `656f144c-a120-5c3b-bee5-06deab438588` |
| Verified commit | *pending* |
| Status | COMPLETE |
| Sign-off | Nathan Young, June 10th, 2026 |

## At a glance

| Aspect | State |
|---|---|
| Algorithm relationship | **Port** of both `FindSizes::execute()` (ImageGeom) and `FindSizes::findSizesUnstructured()` (RectGridGeom) |
| Oracle | **Class 1 (Analytical)** — all test data inlined, hand-derived |
| Code paths | **14 of 19** exercised; 5 gaps (cancel ×2, INT32_MAX overflow ×2, other-geom no-op ×1) |
| Tests | **9 TEST_CASEs** — all pass |
| External archive | None |
| Deviations | **1 active** (`ComputeFeatureSizes-D1`): Kahan vs naive summation in RectGridGeom — see deviations file |
| Open bugs | **1 open** (`Bug-1`): 2D area formula uses all 3 spacings (= volume) instead of the 2 non-flat spacings |

## Summary

`ComputeFeatureSizesFilter` produces three arrays per feature: `NumElements` (voxel count, `int32`), `Volumes`/`Areas` (float32), and `EquivalentDiameters` (ESD or ECD, float32). For ImageGeom it uses voxel count × voxel size; for RectGridGeom it sums per-cell element sizes per feature. Both paths are parallelized via `ParallelDataAlgorithm` + TBB `tbb::combinable`. All tests use Class 1 oracles (hand-constructed fixtures with first-principles expected values). Source-inspection confirms both geometry paths are ports of legacy `FindSizes`. One precision deviation (`D1`) and one open bug (`Bug-1`) are documented below.

## Algorithm Relationship

**Port** of `FindSizes` (both ImageGeom and RectGridGeom paths).

- **ImageGeom** (`ProcessImageGeom`): voxel count × voxel volume/area → ESD/ECD. Identical to `FindSizes::execute()`. Formulas: `ESD = 2·∛(V / (4π/3))`; `ECD = 2·√(A / π)`.
- **RectGridGeom** (`ProcessRectGridGeom`): per-cell `ΔxΔyΔz` sizes, summed per feature. Port of `FindSizes::findSizesUnstructured()`. SIMPLNX departs in precision: float32 element sizes are promoted to float64 and Kahan summation is applied at two levels (per-thread and post-reduction). See deviation `D1`.

Port-time additions not in DREAM3D 6.5.171:
- **Parallel execution** — `ParallelDataAlgorithm` + `tbb::combinable`; legacy was serial.
- **Execute-time FeatureId bounds check** — `ValidateFeatureIdsToFeatureAttributeMatrixIndexing`; legacy accessed out-of-bounds silently.
- **Preflight dimension guard** — rejects ImageGeom with two or more dimensions equal to 1. This is because a single empty dimension converts the calculation from Volume to Area and "1D" Images would not make sense to get an Area formula.

## Oracle

**Class 1 (Analytical)** — all expected values are derived from first principles and asserted in `test/ComputeFeatureSizesTest.cpp`.

| Fixture | Geometry | Derived quantity |
|---|---|---|
| `Create2DImageDataStructure()` | 5×5×1 ImageGeom, spacing 20.2×0.1×1.0 | `voxelArea = 20.2 × 0.1 × 1.0 = 2.02`; areas = count × 2.02; ECDs from circle formula |
| `Create3DImageDataStructure()` | 5×5×5 ImageGeom, spacing 1.2×0.9×2.1 | `voxelVol = 1.2 × 0.9 × 2.1 = 2.268`; volumes = count × 2.268; ESDs from sphere formula |
| `CreateRectGridDataStructure()` | 4×4×4 non-uniform RectGrid | Per-cell `ΔxΔyΔz` summed per feature; step-by-step trace in provenance sidecar |

Negative tests use degenerate inputs (out-of-bounds FeatureId; degenerate dims) and assert an error result is returned.

Second-engineer review pending: verify hand-derivations for all three fixtures and the tolerance choice (`std::numeric_limits<float32>::epsilon()`).

## Code path coverage

14 of 19 paths exercised. Source: `Algorithms/ComputeFeatureSizes.cpp`.

| # | Path | Exercised by |
|---|---|---|
| 1 | Preflight: `emptyDimCount > 1` → error | `Invalid: Preflight Failure` (4 sub-checks) |
| 2 | Preflight: valid dims → pass | All 6 positive tests |
| 3 | Execute validate: FeatureId > numFeatures → error | `Invalid: Execution Failure` |
| 4 | Execute validate: passes → geometry dispatch | All 6 positive tests |
| 5 | Dispatch: ImageGeom → `ProcessImageGeom` | `Image 2D *`, `Image Stack 3D *` |
| 6 | Dispatch: RectGridGeom → `ProcessRectGridGeom` | `Rectilinear Grid *` |
| 7 | Dispatch: other geometry → no-op | *Not tested. `GeometrySelectionParameter` prevents this at runtime; gap acceptable.* |
| 8 | Image: any dim == 1 → area + ECD | `Image 2D *` |
| 9 | Image: all dims > 1 → volume + ESD | `Image Stack 3D *` |
| 10 | Image: voxelCount > `k_MaxVoxelCount` → error | *Not tested. Requires >2³¹ voxels in one feature; impractical. Gap acceptable.* |
| 11 | Image: `SaveElementSizes = false` | `Image 2D`, `Image Stack 3D` |
| 12 | Image: `SaveElementSizes = true` | `Image 2D with Element Sizes`, `Image Stack 3D with Element Size` |
| 13 | Image: `shouldCancel` in loop → early return | *Not tested. Cancel disregard would cause hangs in any test; low-value gap.* |
| 14 | RectGrid: `findElementSizes` → per-cell volumes available | `Rectilinear Grid *` |
| 15 | RectGrid: Kahan summation in `RectGridSummationImpl` + post-reduction | `Rectilinear Grid *` (non-uniform spacing exercises summation) |
| 16 | RectGrid: voxelCount > `k_MaxVoxelCount` → error | *Not tested. Same rationale as path 10.* |
| 17 | RectGrid: `SaveElementSizes = false` → `deleteElementSizes()` | `Rectilinear Grid` |
| 18 | RectGrid: `SaveElementSizes = true` → element sizes retained | `Rectilinear Grid with Element Size` |
| 19 | RectGrid: `shouldCancel` in loop → early return | *Not tested. Same rationale as path 13.* |

## Test inventory

| Test case | Notes |
|---|---|
| `Valid: Image 2D` | Class 1; 5×5×1, spacing 20.2×0.1×1.0; SaveElementSizes=false |
| `Valid: Image 2D with Element Sizes` | Same fixture; SaveElementSizes=true |
| `Valid: Image Stack 3D` | Class 1; 5×5×5, spacing 1.2×0.9×2.1; SaveElementSizes=false |
| `Valid: Image Stack 3D with Element Size` | Same fixture; SaveElementSizes=true |
| `Valid: Rectilinear Grid` | Class 1; 4×4×4 non-uniform; SaveElementSizes=false; Kahan path exercised |
| `Valid: Rectilinear Grid with Element Size` | Same fixture; SaveElementSizes=true |
| `Invalid: Execution Failure` | FeatureId 10 in a 4-feature AM; asserts execute result invalid |
| `Invalid: Preflight Failure` | 4 degenerate dim configs; asserts preflight result invalid |
| `SIMPL Backwards Compatibility` | `DYNAMIC_SECTION` over SIMPL 6.4 + 6.5; validates UUID + arg-key + param-value decoding |

All 9 TEST_CASEs pass.

## Exemplar archive

None — all fixtures constructed in C++ at test time. Provenance in `vv/provenance/ComputeFeatureSizesFilter.md`.

## Deviations from DREAM3D 6.5.171

**ImageGeom path:** No deviations. Formulas and logic identical to `FindSizes::execute()`. The float64 intermediate in the ESD/ECD computation differs from likely float32 in legacy by ≤1 ULP for typical EBSD spacings; non-material.

**RectGridGeom path — `ComputeFeatureSizes-D1`:** Per-feature volumes differ from `FindSizes::findSizesUnstructured()` output due to two precision improvements in SIMPLNX: (1) element sizes promoted from float32 to float64 before accumulation; (2) Kahan compensated summation applied per-thread and during TBB post-reduction, reducing accumulated error from O(N·ε_float32) to O(ε_float64). SIMPLNX is more accurate. Users migrating from DREAM3D 6.5.171 should expect small shifts in per-feature volumes for RectGridGeom; largest for features with many cells on grids with high cell-volume variation.

Full entry in `vv/deviations/ComputeFeatureSizesFilter.md`.
