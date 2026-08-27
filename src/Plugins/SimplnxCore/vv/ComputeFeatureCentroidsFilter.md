# V&V Report: ComputeFeatureCentroidsFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| SIMPLNX Human Name | Compute Feature Centroids |
| DREAM3D 6.5.171 equivalent | `FindFeatureCentroids` (SIMPL UUID `6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`) — `Source/Plugins/Generic/GenericFilters/FindFeatureCentroids.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE — 2026-07-16 |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-07-16 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** — the non-periodic path of `ComputeFeatureCentroids::operator()` is a near line-by-line translation of legacy `FindFeatureCentroids::find_centroids()`, including the identical Kahan compensated summation and `float64`/`float64`/`uint64` sum/compensation/count triplet. Port-time additions: SIMPLNX-only `Is Periodic` option, disabled `ParallelDataAlgorithm` scaffolding, `float64` voxel-center fetch (`Point3Dd`), and a FeatureIds→AttributeMatrix indexing guard. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** (confirmed 2026-07-07). 5 hand-derived toy fixtures A–E (centroid = mean of voxel centers, `voxel-center = origin + (index+0.5)·spacing`). Signed off by Michael Jackson (technical authority) 2026-07-16. |
| Code paths enumerated  | **8 of 8 exercised** — range filter, range tracking, Kahan accumulate, `count>0` finalize, `count==0` skip, periodic-fires, periodic-not-fires, validation-error. |
| Tests today            | **4 TEST_CASEs (5 ctest entries), all pass** in-core — Class 1 Analytical (A/B/C, 3 SECTIONs), Class 1/4 Periodic (D/E), validation-error (`-5351`), and SIMPL 6.4/6.5 backwards-compat (`DYNAMIC_SECTION`). Retired the prior circular consistency test. |
| Exemplar archive       | **None — inline analytical fixtures** (provenance sidecar written). The shared `6_6_stats_test_v2.tar.gz` is no longer consumed by this test (kept for 5 other tests); `6_6_find_feature_centroids.tar.gz` kept (used by ExtractComponentAsArray / WriteAbaqusHexahedron). |
| Legacy comparison      | **Source-inspection** (6.5.171/6.5.172 vs SIMPLNX), backed by the independent Class 1 fixtures. Non-periodic path is an exact Kahan port; 2 deviations (D1 float32→float64 voxel-center precision; D2 SIMPLNX-only `Is Periodic`). Empirical binary A/B available if bit-confirmation is required. |
| Bug flags              | **None outstanding.** One SIMPLNX-internal bug found and resolved: the `ImageGeom` overload of `AdjustCentroidsForPeriodicFaces` added `(dim−1)/2` in cell-index units to a physical-coordinate centroid (ignoring spacing); fixed to scale by spacing (`GeometryHelpers.cpp:245`), with Fixture E as the regression pin. Not a legacy deviation (legacy has no periodic path). |
| V&V phase              | Oracle design + reconciliation, algorithm review, code-path coverage, test inventory, legacy comparison, deviations, and provenance complete. **V&V complete and signed off by Michael Jackson (technical authority) 2026-07-16.** Outstanding: OOC dual-build run (deferred — feature-indexed serial algorithm, no OOC-specific variant). |

## Summary

`ComputeFeatureCentroidsFilter` computes the centroid of each feature as the average X/Y/Z position of all
cells belonging to that feature, using Kahan compensated summation for numerical precision. An optional
`Is Periodic` mode wraps features that span the image boundary. Verification used a **Class 1 (Analytical)**
oracle — the centroid is the arithmetic mean of voxel-center coordinates, hand-derivable on small grids —
paired with **Class 4 (Invariant)** bounding-box and fire-condition predicates. SIMPLNX matched the analytical
values on every fixture; the legacy diff reduces to a single float32-vs-float64 precision non-deviation plus
the SIMPLNX-only periodic feature, and the reconciliation surfaced and fixed one SIMPLNX-internal periodic
spacing bug.

## Algorithm Relationship

*Classification:* **Port**

*Evidence:* `ComputeFeatureCentroids::operator()` is a near line-by-line translation of legacy
`FindFeatureCentroids::find_centroids()` (6.5.172 `Generic/GenericFilters/FindFeatureCentroids.cpp:144–224`),
preserving the identical Kahan compensated summation, the `float64`/`float64`/`uint64` sum/compensation/count
triplet, and the `count>0` divide-and-store finalize. Same SIMPL UUID retained
(`6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`); SIMPL 6.4/6.5 conversion fixtures at
`test/simpl_conversion/6_*/ComputeFeatureCentroidsFilter.json`.

*Port-time deltas:*

1. **`Is Periodic` option** (SIMPLNX-only) + per-feature index-range tracking + the call to
   `GeometryHelpers::Topology::AdjustCentroidsForPeriodicFaces`. Changes output when enabled; default `false`
   reproduces legacy exactly. Legacy has no equivalent. Drives deviation D2.
2. **Voxel-center precision**: SIMPLNX fetches the voxel center directly in `float64`
   (`ImageGeom::getCoords` → `Point3Dd`); legacy fetches into a `float[3]` then promotes to `double` inside the
   Kahan step. Both store the final centroid as `float32`. Sub-ULP-at-float32 numeric delta. Drives D1.
3. **`ParallelDataAlgorithm` scaffolding present but `setParallelizationEnabled(false)`** — runs serially, same
   iteration order as legacy. No output impact.
4. **`ValidateFeatureIdsToFeatureAttributeMatrixIndexing` guard** before the sweep — adds an error path legacy
   lacked. No output impact on valid data.

*Material PRs since baseline (2025-10-01):* none change the algorithm. #1547 (doc: `Is Periodic` description),
#1543 (doc: pipeline rename), #1588 (test: SIMPL 6.4/6.5 backwards-compat + 2 fixtures).

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. (Confirmed by developer 2026-07-07.)
Class 3 N/A — no published paper for "mean position of the cells in a feature".

*Applied:* Each centroid component is `Σ(voxel-center coord) / N` over the feature's cells, where
`voxel-center = origin + (index + 0.5)·spacing` — a closed form on any toy grid, derived independently of both
codebases. Five fixtures cover the code paths: **A** (3×1×1, basic mean + `count==0` background),
**B** (4×2×1, multi-feature + single-cell + empty id), **C** (2×2×2, 3D z-stride), **D** (4×1×1 spacing 1,
periodic wrap fires on the spanning feature only), and **E** (4×1×1 spacing 2, origin 10 — periodic offset
must scale with spacing; the regression pin for the fixed periodic bug). Class 4 invariants asserted on all
fixtures: `count==0 ⇒ centroid == (0,0,0)`; each non-periodic component lies within its feature's voxel-center
bounding box `[origin+(idxMin+0.5)·spacing, origin+(idxMax+0.5)·spacing]`; the periodic adjustment fires **iff**
the feature spans the full extent on that axis.

*Encoded:* `test/ComputeFeatureCentroidsTest.cpp` (`namespace CentroidToy`) — `Class 1 - Analytical Centroids`
(Fixtures A/B/C) and `Class 1/4 - Periodic Boundary` (Fixtures D/E), built + run in `NX-Com-Qt69-Vtk96-Rel`,
all pass at float32 margin 1e-4.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Code path coverage

*8 of 8 paths exercised.* Source:
`src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.cpp` (~222 lines).
Logical phases: (a) per-cell sweep (accumulate + range-track), (b) per-feature finalize, (c) optional periodic
adjust.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Sweep | Range filter `featureId ∈ [min,max)` (parallel off → full range) | All fixtures (valid ids) |
| 2 | (a) Sweep | Per-feature min/max X/Y/Z index-range tracking | Fixtures D, E (drive the periodic condition) |
| 3 | (a) Sweep | Kahan accumulate X/Y/Z + count increment | All fixtures |
| 4 | (b) Finalize | `count>0` → centroid = sum/count | A/B/C/D/E (non-empty features) |
| 5 | (b) Finalize | `count==0` → centroid stays (0,0,0) | A (fid0), B (fid0), C (fid0) |
| 6 | (c) Periodic | `IsPeriodic` + feature spans full extent → offset applied (spacing-aware) | D (spacing 1), E (spacing 2, regression pin) |
| 7 | (c) Periodic | `IsPeriodic` + feature does not span → unchanged | D (feature 2), all non-periodic runs |
| 8 | (pre) | `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` failure (`maxId ≥ numFeatures`, `-5351`) | `Error - FeatureId exceeds Feature AM` |

*The preflight AttributeMatrix-null path (`-12700`) is guarded by the `AttributeMatrixSelectionParameter` and
cannot be triggered with valid arguments — not separately tested. The OOC dual-build run is deferred: the
algorithm is a feature-indexed serial sweep with no OOC-specific variant.*

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeFeatureCentroidsFilter` (consistency-with-self) | retired | Compared a fresh run against the sibling `Centroids` array already in `6_6_stats_test_v2.dream3d` (prior DREAM3D output) — a circular oracle. Replaced by the analytical fixtures. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Class 1 - Analytical Centroids` | new-for-V&V | Fixtures A/B/C (3 SECTIONs); hand-derived centroids at 1e-4 tolerance. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Class 1/4 - Periodic Boundary` | new-for-V&V | Fixtures D/E; periodic fire-condition + spacing-aware offset (Fixture E pins the bug fix). |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Error - FeatureId exceeds Feature AM` | new-for-V&V | Pins the `-5351` validation error path. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over SIMPL 6.5 (UUID) + 6.4 (Filter_Name); validates UUID + 3 argument values. Conversion coverage only. |

All active TEST_CASEs pass in-core (`NX-Com-Qt69-Vtk96-Rel`). OOC dual-build deferred (see Code path coverage).

## Exemplar archive

- **Archive:** None — the oracle is encoded as inline Class 1 (Analytical) + Class 4 (Invariant) fixtures in
  `test/ComputeFeatureCentroidsTest.cpp`; there is no `.dream3d` gold-master to hash.
- **SHA512:** N/A (no archive).
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeFeatureCentroidsFilter.md`

The prior circular consistency test and its `TestFileSentinel` dependency were retired. `6_6_stats_test_v2.tar.gz`
is kept in `test/CMakeLists.txt` (five other tests still consume it) but is no longer used by this filter's
tests; `6_6_find_feature_centroids.tar.gz` is kept (used by the ExtractComponentAsArray and
WriteAbaqusHexahedron tests — not an orphan).

## Deviations from DREAM3D 6.5.171

Comparison by source inspection of 6.5.171/6.5.172 `FindFeatureCentroids.cpp` vs SIMPLNX, backed by the Class 1
analytical fixtures that verify SIMPLNX independently. Full entries in
`vv/deviations/ComputeFeatureCentroidsFilter.md`:

- `ComputeFeatureCentroidsFilter-D1` — *precision* — SIMPLNX fetches voxel centers as `float64`, legacy as
  `float32` then promotes; both store `float32`, residual ≤ 1 float32 ULP. Trust SIMPLNX.
- `ComputeFeatureCentroidsFilter-D2` — *feature addition* — `Is Periodic` is SIMPLNX-only; default `false`
  reproduces legacy exactly. (Includes the note that a SIMPLNX-internal periodic spacing bug was fixed this
  cycle — not a legacy diff, since legacy has no periodic path.)
- **Retracted candidate** — the DRAFT "Kahan-vs-naive summation" precision deviation: legacy already uses the
  identical Kahan kernel, so no such deviation exists (the residual precision difference is captured by D1).
