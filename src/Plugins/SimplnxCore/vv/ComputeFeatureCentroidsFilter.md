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
| Oracle (confirmed)     | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** (confirmed 2026-07-07). Historical fixtures A–F cover arithmetic centroids and several periodic cases. The 2026-10 correction adds discriminating unequal-population, tied-gap, full-axis, and sorted-coordinate oracles. The historical 2026-07-16 sign-off is retained. |
| Code paths enumerated | 9 of 9 listed numerical/validation paths exercised; cancellation and internal error-site exhaustiveness are outside this campaign. |
| Tests today | The 2026-10-15 Windows run passed all 8 registered cases in in-core Release and OOC Release/Debug. Both hidden HDF5 cases also passed, normally and with optional working-memory probes. The historical 2026-09-21 results remain recorded below. |
| Exemplar archive       | **None — inline analytical fixtures** (provenance sidecar written). The shared `6_6_stats_test_v2.tar.gz` is no longer consumed by this test (kept for 5 other tests); `6_6_find_feature_centroids.tar.gz` kept (used by ExtractComponentAsArray / WriteAbaqusHexahedron). |
| Legacy comparison      | **Source-inspection** (DREAM3D 6.5.171 vs SIMPLNX; local legacy source used for corroboration), backed by the independent Class 1 fixtures. Non-periodic path is an exact Kahan port; 2 deviations (D1 float32→float64 voxel-center precision; D2 SIMPLNX-only `Is Periodic`). Empirical binary A/B available if bit-confirmation is required. |
| Bug flags              | The 2026-10 adversarial review found that the circular estimator introduced in 2026-07 disagreed with the documented unwrapped arithmetic mean for unequal voxel populations. The current periodic path uses the unique largest empty gap and a weighted arithmetic mean. The historical spacing and constant-offset fixes remain recorded below. |
| V&V phase | Historical COMPLETE status and sign-off retained. OOC recertification verifies preserved analytical tests and a real-HDF5 block-tail case. |

## Follow-up (2026-07, issue #1665) — periodic model reimplemented

This section records what was implemented and verified in July 2026. Its circular-estimator description is superseded by the October 2026 correction below; the original verification and sign-off history is unchanged.

The original V&V (#1658) fixed a **spacing** sub-bug in the periodic path but validated the periodic result against the **constant half-domain offset model itself** (Fixture D's expected value was "naive + (dim−1)/2", a circular oracle — the test was designed around the code). An adversarial review (#1665) showed that model is fundamentally wrong: a constant offset is correct only when a wrapped feature's mass is symmetric about the seam, and for asymmetric features it places the centroid in the empty middle of the feature or outside the domain entirely.

This follow-up (on top of merged #1658) corrects it:

- **Periodic path reimplemented as a minimum-image circular mean.** During the existing cell sweep, per feature and axis, the unit vectors of each cell center's angular position around the domain are accumulated (`sumCos`, `sumSin`). At finalize, for any axis on which the feature spans the full extent, the centroid component becomes `atan2(sumSin, sumCos)` mapped back into the domain. A near-zero resultant (domain-filling feature) falls back to the arithmetic mean. O(numFeatures) extra memory; trig only when `Is Periodic` is on.
- **The range-based `ImageGeom` overload of `AdjustCentroidsForPeriodicFaces` was removed** — it received only per-feature index ranges, which are mathematically insufficient to compute a wrapped centroid. Nothing else called it.
- **Fixtures D and E were re-derived against an independent minimum-image oracle** (no longer circular): D → x = 3.5 (was 3.667), E (now asymmetric, non-unit spacing) → x = 11.0 (was 17.0). New Fixture F pins the domain-filling arithmetic-mean fallback. All pass in-core and out-of-core.
- The earlier conclusion that the sibling `BoundingBox` overload was sound is **superseded**: that overload carried the same constant-offset defect and was fixed under issue #1665 in the same branch as this follow-up.

The non-periodic path is unchanged (still the verified Kahan port). Deviation `ComputeFeatureCentroidsFilter-D2` is updated accordingly.

## Follow-up (2026-10) — exact periodic arithmetic

An adversarial five-cell fixture in a 5×3×1 image exposed a difference between the circular estimator (4.75) and the documented arithmetic mean after unwrapping (4.7). The feature's X populations are two cells at index 0, one at 3, and two at 4. The unique largest empty gap lies between indices 0 and 3; shifting index 0 by one period and averaging all five cells gives 4.7. A separate D=8 unequal-population fixture distinguishes this rule from recentering around a circular anchor.

The current implementation records occupied coordinates for actual boundary-spanning feature-axis pairs in bounded bitsets. It carries largest-gap and tie state across ascending words and windows, then makes one sequential weighted Kahan pass for axes with a unique largest gap. Tied gaps and complete axis occupancy retain the original arithmetic mean. The original nonperiodic Kahan summation and unused-feature zero behavior remain. The new occupancy payload requests up to 32 MiB of shared working memory, adapts to the grant, and requires a minimum 8-byte word; validation and pre-final-write failures leave final centroid publication untouched.

The registered tests include the five-cell result, axis permutations, signed/nonunit spacing and origin, all-axis wrapping, ties, full occupancy, deterministic sorted-coordinate random cases, exact small-grant/window behavior, and cancellation/read/resource failures. A separate hidden test checks the wrapped result with actual HDF5-OOC Feature IDs and centroid stores. These cases correct the current oracle claim without retroactively changing the July or September test record.

Verification on 2026-10-15 used Windows MSVC 14.44.35207 with the paired DREAM3D-NX builds: in-core Release (`SIMPLNX_TEST_ALGORITHM_PATH=0`) and OOC Release/Debug (`SIMPLNX_TEST_ALGORITHM_PATH=1`). All eight registered centroid cases passed in each configuration, including the metadata-only arithmetic-overflow guards. The two hidden HDF5 cases passed in Release and Debug with 54 assertions normally and 396 assertions with the optional working-memory probes. Both input and output stores are checked as HDF5-OOC. These are local Windows results, not native macOS/Linux results or a new technical-authority sign-off.

The optional probe uses a 32768-by-16 image and a fixed 256 MiB cache budget. Its occupancy needs only 65,544 bytes, so 8/32/64 MiB of available working memory all fit one batch. A 64 KiB grant requires two batches and an 8 KiB grant requires nine. The resident probe observes the corresponding logical-read counts; the HDF5 probe labels those batch counts as planned and checks every output component. Release samples were approximately 9.6-9.8 ms with ample resident working memory and 35.1 ms at 8 KiB; the HDF5 samples were 14.1-23.4 ms and 39.4 ms respectively. These are single, likely warm-cache samples taken while other builds could run. They demonstrate correct results with smaller grants, not cold-cache throughput, a comparison against the old estimator, large-pipeline performance, or an optimal 32 MiB limit.

## Summary

`ComputeFeatureCentroidsFilter` computes the centroid of each feature as the average X/Y/Z position of all
cells belonging to that feature, using Kahan compensated summation for numerical precision. An optional
`Is Periodic` mode uses the unique largest-gap arithmetic mean for features that span the image boundary. Current verification
uses a **Class 1 (Analytical)** oracle — arithmetic means for non-periodic features and independently derived
sorted-coordinate weighted means for wrapped features — paired with **Class 4 (Invariant)** bounding-box, fire-condition,
and in-domain predicates. SIMPLNX matched the analytical values on every fixture; the legacy diff reduces to
a single float32-vs-float64 precision non-deviation plus the SIMPLNX-only periodic feature, and the
reconciliation surfaced and fixed two SIMPLNX-internal periodic-centroid defects.

## Algorithm Relationship

*Classification:* **Port**

*Evidence:* `ComputeFeatureCentroids::operator()` is a near line-by-line translation of legacy
`FindFeatureCentroids::find_centroids()` (local legacy source `Generic/GenericFilters/FindFeatureCentroids.cpp:144–224`),
preserving the identical Kahan compensated summation, the `float64`/`float64`/`uint64` sum/compensation/count
triplet, and the `count>0` divide-and-store finalize. Same SIMPL UUID retained
(`6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`); SIMPL 6.4/6.5 conversion fixtures at
`test/simpl_conversion/6_*/ComputeFeatureCentroidsFilter.json`.

*Port-time deltas:*

1. **`Is Periodic` option** (SIMPLNX-only) + per-feature index-range tracking and bounded occupied-coordinate
   scans for the unique largest-gap unwrapped arithmetic mean. Changes output when enabled; default `false` reproduces
   legacy exactly. Legacy has no equivalent. Drives deviation D2.
2. **Voxel-center precision**: SIMPLNX fetches the voxel center directly in `float64`
   (`ImageGeom::getCoords` → `Point3Dd`); legacy fetches into a `float[3]` then promotes to `double` inside the
   Kahan step. Both store the final centroid as `float32`. Sub-ULP-at-float32 numeric delta. Drives D1.
3. **`ParallelDataAlgorithm` scaffolding present but `setParallelizationEnabled(false)`** — runs serially, same
   iteration order as legacy. No output impact.
4. **`ValidateFeatureIdsToFeatureAttributeMatrixIndexing` guard** before the sweep — adds an error path legacy
   lacked. No output impact on valid data.

*Material PRs since baseline (2025-10-01):* none change the algorithm. #1547 (doc: `Is Periodic` description),
#1543 (doc: pipeline rename), #1588 (test: SIMPL 6.4/6.5 backwards-compat + 2 fixtures).

## Oracle recorded in the 2026-07 V&V

The following records the oracle and conclusions as signed off in July. Its circular-mean interpretation and periodic adjustment predicate are superseded by the 2026-10 follow-up above.

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. (Confirmed by developer 2026-07-07.)
Class 3 N/A — no published paper for "mean position of the cells in a feature".

*Applied:* Each non-periodic centroid component is `Σ(voxel-center coord) / N` over the feature's
cells, where `voxel-center = origin + (index + 0.5)·spacing`. A wrapped component is the minimum-image
circular mean of those coordinates. Six fixtures cover the code paths: **A** (3×1×1, basic mean +
`count==0` background), **B** (4×2×1, multi-feature + single-cell + empty id), **C** (2×2×2, 3D z-stride),
**D** (4×1×1, asymmetric periodic wrap at unit spacing), **E** (4×1×1, asymmetric periodic wrap at non-unit
spacing and nonzero origin), and **F** (domain-filling distribution, which triggers the arithmetic-mean
fallback). Class 4 invariants assert that `count==0 ⇒ centroid == (0,0,0)`, non-periodic components remain
inside their feature's voxel-center bounding box, periodic adjustment fires **iff** the feature spans the full
extent on that axis, and adjusted centroids remain inside the periodic domain.

*Encoded:* `test/ComputeFeatureCentroidsTest.cpp` (`namespace CentroidToy`) — `Class 1 - Analytical Centroids`
(Fixtures A/B/C) and `Class 1/4 - Periodic Boundary` (Fixtures D/E/F), built + run in `NX-Com-Qt69-Vtk96-Rel`,
all pass at float32 margin 1e-4.

*Second-engineer review:* **Signed off by Michael Jackson (technical authority), 2026-07-16.**

## Bugs found and fixed in the historical V&V

No new defect was found during this OOC recertification. The historical periodic-spacing correction described above remains in the branch.

## Code path coverage recorded in the historical V&V

9 of 9 listed numerical/validation paths exercised. Source:
`src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.cpp`.
Logical phases: (a) per-cell sweep (accumulate + range-track), (b) per-feature finalize, (c) optional periodic
adjust.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Sweep | Range filter `featureId ∈ [min,max)` (parallel off → full range) | All fixtures (valid ids) |
| 2 | (a) Sweep | Per-feature min/max X/Y/Z index-range tracking | Fixtures D, E (drive the periodic condition) |
| 3 | (a) Sweep | Kahan accumulate X/Y/Z + count increment | All fixtures |
| 4 | (b) Finalize | `count>0` → centroid = sum/count | A/B/C/D/E (non-empty features) |
| 5 | (b) Finalize | `count==0` → centroid stays (0,0,0) | A (fid0), B (fid0), C (fid0) |
| 6 | (c) Periodic | `IsPeriodic` + feature spans full extent + nonzero resultant → circular mean | D/E |
| 7 | (c) Periodic | Feature does not span → unchanged; near-zero resultant → arithmetic fallback | D (feature 2), F |
| 8 | (pre) | `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` failure (`maxId ≥ numFeatures`, `-5351`) | `Error - FeatureId exceeds Feature AM` |
| 9 | Bulk I/O | Feature accumulation crosses a 65,536-cell block and one-cell tail | `real HDF5 65536-block tail oracle` — exact centroid 131082, -0.5, 9 in both periodic modes |

*The preflight AttributeMatrix-null path (`-12700`) is guarded by the `AttributeMatrixSelectionParameter` and
cannot be triggered with valid arguments — not separately tested. The OOC dual-build run is deferred: the
algorithm is a feature-indexed serial sweep with no OOC-specific variant.*

## Test inventory recorded in the historical V&V

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeFeatureCentroidsFilter` (consistency-with-self) | retired | Compared a fresh run against the sibling `Centroids` array already in `6_6_stats_test_v2.dream3d` (prior DREAM3D output) — a circular oracle. Replaced by the analytical fixtures. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Class 1 - Analytical Centroids` | new-for-V&V | Fixtures A/B/C (3 SECTIONs); hand-derived centroids at 1e-4 tolerance. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Class 1/4 - Periodic Boundary` | new-for-V&V | Fixtures D/E/F; asymmetric minimum-image means, periodic fire condition, non-unit spacing, and domain-filling fallback. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Error - FeatureId exceeds Feature AM` | new-for-V&V | Pins the `-5351` validation error path. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over SIMPL 6.5 (UUID) + 6.4 (Filter_Name); validates UUID + 3 argument values. Conversion coverage only. |
| `Bulk boundary and failure propagation` | kept | Existing bulk-read boundary and injected-failure regression. |
| `real HDF5 65536-block tail oracle` | new-for-V&V | Reuses CentroidToy::Build and Run; checks exact physical centroids for background, the two-cell feature across the boundary, and an empty feature. |

At the time of this historical inventory, all active TEST_CASEs passed in-core (`NX-Com-Qt69-Vtk96-Rel`). OOC dual-build was deferred then (see historical Code path coverage).

OOC recertification (2026-09-21): 5/5 registered CTest entries pass in each DREAM3DNX build. The new hidden case passes 35 assertions across periodic off/on. Input FeatureIds and output Centroids are asserted HDF5-OOC. The two contributing X coordinates are 131081 and 131083, so the expected mean is exactly 131082. Upstream numerical assertions and tolerances remain unchanged; legacy deviation dispositions are retained without claiming a new binary comparison.

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
  reproduces legacy exactly. (Includes the two SIMPLNX-internal periodic-centroid fixes: spacing and the
  constant-offset model — neither is a legacy diff because legacy has no periodic path.)
- **Retracted candidate** — the DRAFT "Kahan-vs-naive summation" precision deviation: legacy already uses the
  identical Kahan kernel, so no such deviation exists (the residual precision difference is captured by D1).
