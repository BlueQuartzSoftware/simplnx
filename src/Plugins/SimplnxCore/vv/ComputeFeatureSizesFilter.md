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
| Algorithm relationship | **Port** of both `FindSizes::findSizesImage()` (ImageGeom) and `FindSizes::findSizesUnstructured()` (RectGridGeom) |
| Oracle | **Class 1 (Analytical)** — all test data inlined, hand-derived |
| Code paths | **14 of 19** exercised; 5 gaps (cancel ×2, INT32_MAX overflow ×2, other-geom no-op ×1) |
| Tests | **10 TEST_CASEs** — all pass |
| External archive | None |
| Deviations | **2 active**, both A/B-verified 2026-06-27: `ComputeFeatureSizes-D1` (float64+Kahan vs naive summation → `Volumes`), `ComputeFeatureSizes-D2` (float64 `std::cbrt` vs float32 `powf` → `EquivalentDiameters`) — see deviations file |
| Open bugs | **None.** `Bug-1` (2D area formula multiplied all 3 spacings instead of the 2 non-flat spacings) was **fixed this cycle** and pinned by the `2D area excludes the flat-dimension spacing` characterization test; the ImageGeom 2D path now matches legacy (no deviation). |

## Summary

`ComputeFeatureSizesFilter` produces three arrays per feature: `NumElements` (voxel count, `int32`), `Volumes`/`Areas` (float32), and `EquivalentDiameters` (ESD or ECD, float32). For ImageGeom it uses voxel count × voxel size; for RectGridGeom it sums per-cell element sizes per feature. Both paths are parallelized via `ParallelDataAlgorithm` + TBB `tbb::combinable`. All tests use Class 1 oracles (hand-constructed fixtures with first-principles expected values). Source-inspection confirms both geometry paths are ports of legacy `FindSizes`. Two precision deviations (`D1`, `D2`) — both confirmed by a direct A/B run against DREAM3D 6.5.171 — are documented below. A latent 2D-area bug (`Bug-1`: the flat dimension's spacing was included in the per-voxel area) was found during this V&V and **fixed**; the 2D path now matches legacy and is pinned by a dedicated characterization test with a non-unit flat-dimension spacing.

## Algorithm Relationship

**Port** of `FindSizes` (both ImageGeom and RectGridGeom paths).

- **ImageGeom** (`ProcessImageGeom`): voxel count × voxel volume/area → ESD/ECD. Port of `FindSizes::findSizesImage()` (legacy `execute()` dispatches to it via `findSizes()`). Formulas: `ESD = 2·∛(V / (4π/3))`; `ECD = 2·√(A / π)`.
- **RectGridGeom** (`ProcessRectGridGeom`): per-cell `ΔxΔyΔz` sizes, summed per feature. Port of `FindSizes::findSizesUnstructured()`. SIMPLNX departs in precision: float32 element sizes are promoted to float64 and Kahan summation is applied at two levels (per-thread and post-reduction). See deviation `D1`.

Port-time additions not in DREAM3D 6.5.171:
- **Parallel execution** — `ParallelDataAlgorithm` + `tbb::combinable`; legacy was serial.
- **Execute-time FeatureId bounds check** — `ValidateFeatureIdsToFeatureAttributeMatrixIndexing`; legacy accessed out-of-bounds silently.
- **Preflight dimension guard** — rejects ImageGeom with two or more dimensions equal to 1. This is because a single empty dimension converts the calculation from Volume to Area and "1D" Images would not make sense to get an Area formula.
- **Tighter overflow guard** — SIMPLNX errors when a feature exceeds `INT32_MAX` voxels on both geometry paths. Legacy `findSizesImage` errored only above 2⁵³ (so counts between 2³¹ and 2⁵³ silently overflowed the int32 `NumElements`), and legacy `findSizesUnstructured` had no guard at all.
- **Narrower geometry scope** — the `GeometrySelectionParameter` allows only Image and RectGrid. Legacy `FindSizes` routed every non-image geometry (vertex/edge/triangle/quad/tet) through `findSizesUnstructured`; in SIMPLNX those are served by dedicated filters (e.g. `ComputeTriangleGeomVolumesFilter`).

## Oracle

**Class 1 (Analytical)** — all expected values are derived from first principles and asserted in `test/ComputeFeatureSizesTest.cpp`.

| Fixture | Geometry | Derived quantity |
|---|---|---|
| `Create2DImageDataStructure()` | 5×5×1 ImageGeom, spacing 20.2×0.1×1.0 | `voxelArea = 20.2 × 0.1 = 2.02` (flat-Z spacing excluded); areas = count × 2.02; ECDs from circle formula |
| `Create3DImageDataStructure()` | 5×5×5 ImageGeom, spacing 1.2×0.9×2.1 | `voxelVol = 1.2 × 0.9 × 2.1 = 2.268`; volumes = count × 2.268; ESDs from sphere formula |
| `CreateRectGridDataStructure()` | 4×4×4 non-uniform RectGrid | Per-cell `ΔxΔyΔz` summed per feature; step-by-step trace in provenance sidecar |

Negative tests use degenerate inputs (out-of-bounds FeatureId; degenerate dims) and assert an error result is returned.

Second-engineer review pending: verify hand-derivations for all three fixtures and the tolerance choice (relative `1e-6` via Catch2 `Approx` — a few float32 ULPs of slack so the pins survive platform and TBB reduction-order differences).

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

| Test case | Status | Notes |
|---|---|---|
| `Valid: Image 2D` | kept | Class 1; 5×5×1, spacing 20.2×0.1×1.0 (flat-Z spacing 1.0); SaveElementSizes=false |
| `2D area excludes the flat-dimension spacing` | new-for-V&V | Class 1 characterization pin for the Bug-1 fix. 2×2 slab, non-flat spacings 2.0×3.0, flat-dimension spacing **5.0**; asserts area == 24.0 (not 120.0). `GENERATE`s all three flat orientations (X/Y/Z) so the flat spacing is proven excluded regardless of axis. |
| `Valid: Image 2D with Element Sizes` | kept | Same fixture as `Valid: Image 2D`; SaveElementSizes=true |
| `Valid: Image Stack 3D` | kept | Class 1; 5×5×5, spacing 1.2×0.9×2.1; SaveElementSizes=false |
| `Valid: Image Stack 3D with Element Size` | kept | Same fixture; SaveElementSizes=true |
| `Valid: Rectilinear Grid` | kept | Class 1; 4×4×4 non-uniform; SaveElementSizes=false; Kahan path exercised |
| `Valid: Rectilinear Grid with Element Size` | kept | Same fixture; SaveElementSizes=true |
| `Invalid: Execution Failure` | kept | FeatureId 10 in a 4-feature AM; asserts execute result invalid |
| `Invalid: Preflight Failure` | kept | 4 degenerate dim configs; asserts preflight result invalid |
| `SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over SIMPL 6.4 + 6.5; validates UUID + arg-key + param-value decoding |
| `Legacy: Small IN100 Test` | retired | Real-data comparison against legacy-produced `6_6_stats_test_v2.dream3d` arrays. Retired because it was a legacy-output regression check (not an independent oracle) and the D1/D2 precision deviations intentionally change those exact values; the RectGrid A/B (deviations file) now provides the legacy comparison and the inline Class 1 fixtures provide the correctness oracle. |

All 10 TEST_CASEs pass.

## Exemplar archive

None — all fixtures constructed in C++ at test time. Provenance in `vv/provenance/ComputeFeatureSizesFilter.md`.

## Deviations from DREAM3D 6.5.171

Both deviations below were confirmed by a direct A/B run (2026-06-27), not source inspection alone: the exact RectGrid fixture was authored as a shared legacy `.dream3d` and run through stock DREAM3D 6.5.171, DREAM3D-NX, and a 6.5.172 proof-patch build. Applying **both** the D1 (summation) and D2 (ESD-evaluation) changes to legacy `findSizesUnstructured` made `Volumes` and `EquivalentDiameters` **bit-identical** to SIMPLNX; each change alone closed only its corresponding array.

**`ComputeFeatureSizes-D1` (RectGridGeom → `Volumes`):** Per-feature volumes differ from `FindSizes::findSizesUnstructured()` output due to two precision improvements in SIMPLNX: (1) element sizes promoted from float32 to float64 before accumulation; (2) Kahan compensated summation. Note the Kahan compensator is a local reset on each TBB body invocation (the `combinable` per-thread volume persists, but the compensation term does not carry across chunk boundaries within a thread), so the accumulated-error reduction is per-chunk rather than the full O(ε_float64) a single continuous Kahan pass would give; the dominant improvement is the float64 accumulation. SIMPLNX is still more accurate than the legacy naive float32 sum. Users migrating from DREAM3D 6.5.171 should expect small shifts in per-feature volumes for RectGridGeom; largest for features with many cells on grids with high cell-volume variation.

**`ComputeFeatureSizes-D2` (RectGridGeom → `EquivalentDiameters`):** Even with identical `Volumes`, the equivalent spherical diameter differs because legacy evaluates the cube root with `powf` (float32) on the float32-rounded volume, while SIMPLNX uses `std::cbrt` (float64) on the float64 volume. SIMPLNX is more accurate. The same `powf`/`sqrtf`-on-float32 pattern exists on the ImageGeom path; it was previously noted only as a non-flagged precision difference and is now captured by D2.

Full entry in `vv/deviations/ComputeFeatureSizesFilter.md`.
