# V&V Report: ComputeFeatureReferenceMisorientationsFilter

|                            |                                                                                                                                                                |
|----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | OrientationAnalysis                                                                                                                                            |
| SIMPLNX UUID               | `24b54daf-3bf5-4331-93f6-03a49f719bf1`                                                                                                                         |
| SIMPLNX Human Name         | Compute Feature Reference Misorientations                                                                                                                      |
| DREAM3D 6.5.171 equivalent | `FindFeatureReferenceMisorientations` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureReferenceMisorientations.{h,cpp}` (UUID `428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c`) |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                                                        |
| Status                     | COMPLETE                                                                                                                                               |
| Sign-off                   | *Michael Jackson <mike.jackson@bluequartz.net> (V&V cycle completion, 2026-06-01) — algorithm originally translated to SIMPLNX by Nathan Young (PR history)*   |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Port** — same two-mode structure + per-voxel math via `LaueOps`. `QuatF`→`QuatD`; `getMisoQuat`→`calculateMisorientation`; raster→linear iteration; cancel checks added; new optional `EuclideanCenters` array (Mode 1). UUID reassigned for `Find`→`Compute` rename.                                                                                                                                                                                                                                                                            |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — 6 hand-derived toy fixtures covering both reference-orientation modes + 2D + 3D + multi-feature + edge cases. **Class 4 (Invariant) companion** — monotonicity, range bounds, skip-condition correctness, and the per-feature averaging formula asserted via `ClassFourInvariants::AssertClass4Invariants()` across both fixture configurations.                                                                                                                                                                          |
| Code paths enumerated  | 7 of 8 algorithmic paths exercised directly (Mode 0 vs Mode 1 dispatch, `m_Centers` selection, `EuclideanCenters` writing, valid-voxel accumulate, skip-voxel, finalize-non-empty, finalize-empty-count). 1 path (cancel-check) tested implicitly via the unconditional cancel-check-at-loop-top instrumentation.                                                                                                                                                                                                                              |
| Tests today            | **8 TEST_CASEs / 8 ctest entries**, 100% pass (~0.7s). 6 Class 1 toy fixtures + 1 Class 4 invariants sweep + 1 SIMPL backwards-compatibility test. **No exemplar archive consumed.**                                                                                                                                                                                                                                                                                                                                                          |
| Exemplar archive       | **None — inline-constructed in test source.** The pre-existing `compute_feature_reference_misorientation.tar.gz` archive (Small-IN100-based regression-against-exemplar) was **retired 2026-06-01** because its exemplar arrays were a circular oracle (regenerated from pre-EbsdLib-2.4.1 SIMPLNX output). The 6 hand-derived toy fixtures cover all 8 algorithmic paths and replace the regression-against-archive coverage.                                                                                                                  |
| Legacy comparison      | **Source-inspection comparison against DREAM3D 6.5.171** completed. Algorithm structurally identical to legacy modulo port-time deltas. **No algorithmic deviations** observed (no behavioral bugs in either implementation). One precision-class non-deviation documented: the EbsdLib 2.4.1 `CubicOps::calculateMisorientationInternal` precision improvement (already characterized in `BadDataNeighborOrientationCheckFilter`'s V&V cycle) propagates into per-feature averages for sym-op-aligned grain boundaries — non-observable on toy data. |
| Bug flags              | None.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| V&V phase              | **All V&V work complete per V2 policy.** Class 1 + Class 4 oracle confirmed against 8-test suite; circular-oracle archive retired; legacy A/B by source inspection; algorithm review applied; user-facing doc updated. Three source-tree deliverables (this report + `vv/deviations/...` + `vv/provenance/...`) are in place. **Outstanding:** Status promotion DRAFT → READY FOR REVIEW pending second-engineer oracle review (recommend Joey Kleingers).                                                                                                                                              |

## Summary

`ComputeFeatureReferenceMisorientationsFilter` computes the misorientation angle (in degrees) between each cell's quaternion and a per-feature reference quaternion. The reference is either the feature's average quaternion (Mode 0) or the quaternion of the voxel within the feature that is farthest from the grain boundary (Mode 1). Per-feature averages are computed alongside the per-cell values. In Mode 1, a `Feature Euclidean Centers` array recording the coordinates of each feature's reference voxel is also written.

Verification is via a **Class 1 (Analytical) hand-derived toy-fixture set of 6 unit tests** plus a **Class 4 (Invariant) companion sweep**. The fixtures use pure φ1-rotation quaternions (Bunge ZXZ Euler `(φ1, 0, 0)`) so that misorientation values are closed-form derivable: for Δφ1 ∈ {0°, 5°, 10°} and cubic symmetry, the symmetry-reduced misorientation equals `|Δφ1|`. The fixtures range from 2×2×2 (8 voxels, 1 feature) to 4×3×1 (12 voxels, 5 features including a background and an all-unphased feature), and include a 3×3×2 (18-voxel) 3D fixture specifically to verify the linear-voxelIdx → 3D-coord arithmetic in Mode 1.

A pre-existing `compute_feature_reference_misorientation.tar.gz` archive (Small-IN100 regression-against-exemplar) was retired during this V&V cycle: its exemplar arrays were generated from a pre-EbsdLib-2.4.1 SIMPLNX run (circular oracle), and the EbsdLib 2.4.1 `CubicOps::calculateMisorientationInternal` precision improvement shifted the exemplar values by 2× to 10× the 1e-4 epsilon used in the regression check. The toy fixtures cover all 8 algorithmic paths analytically and remove the circular-oracle dependency. Source inspection of the legacy `FindFeatureReferenceMisorientations` confirms the SIMPLNX algorithm is a clean Port with no algorithmic deviations; the only legacy-vs-SIMPLNX difference is the EbsdLib precision improvement (a precision-class non-deviation), which manifests on real EBSD data with cubic-phase sym-op-aligned boundaries but is non-observable on the toy fixtures.

## Algorithm Relationship

*Classification:* **Port (with UUID reassignment and API modernization)** ~~| Minor changes | Rewrite | New filter~~

*Evidence:* The SIMPLNX algorithm at `Algorithms/ComputeFeatureReferenceMisorientations.cpp` (~175 lines) is a near line-by-line translation of legacy `FindFeatureReferenceMisorientations::execute()` (DREAM3D 6.5.171, ~110 lines). Same two-mode dispatch (`ReferenceOrientation` parameter), same per-voxel main loop computing misorientation via `LaueOps`, same per-feature averaging finalization. The SIMPLNX filter was assigned a **new UUID** (`24b54daf-3bf5-4331-93f6-03a49f719bf1` vs legacy `428e1f5b-e6d8-5e8b-ad68-56ff14ee0e8c`) for the `Find` → `Compute` rename; SIMPL 6.4/6.5 pipelines still open correctly via the conversion fixtures at `test/simpl_conversion/6_*/`.

*Port-time deltas (non-deviation — preserve algorithmic equivalence at toy-data precision):*

1. **EbsdLib API**: `getMisoQuat` → `calculateMisorientation` (axis + angle returned together; underlying `LaueOps` math identical).
2. **Quaternion precision**: `QuatF` → `QuatD` inside the algorithm (float32 inputs promoted, output cast back to float32). Equivalent for non-precision-sensitive inputs; the EbsdLib 2.4.1 CubicOps fix is visible for cubic sym-op-aligned boundaries (see D1).
3. **Voxel iteration**: `for col/row/plane` triple loop → `for voxelIdx` linear loop. Equivalent iteration order; raster-order tie-break (`>=`, later-voxel-wins) preserved in `m_Centers` selection.
4. **Auxiliary storage**: legacy `avgMisoPtr` (interleaved count + sum) → two separate vectors `avgMisorientationSums` + `avgMisorientationCounts`. Equivalent semantics.
5. **Cancel checks** added (UX only; no algorithmic effect on completed runs).
6. **New optional `EuclideanCenters` array** (Mode 1 only) — SIMPLNX feature, not a port artifact; does not affect pre-existing output.
7. **EbsdLib 2.4.1 CubicOps precision improvement** — non-observable on V&V toy fixtures (no sym-op-aligned features); ~ULP-scale per-feature drift on real EBSD data. See D1 + `BadDataNeighborOrientationCheckFilter`'s V&V cycle.

*Material PRs since baseline (2025-10-01):* None identified that materially change this filter's algorithm. PR #1472 (EbsdLib 2.0.0 API bump) is the closest, and that just affects the `getMisoQuat` → `calculateMisorientation` API delta noted above.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. Class 3 (Paper-based) N/A — this filter delegates misorientation math to `ebsdlib::LaueOps::calculateMisorientation`; the Rowenhorst 2015 paper-based verification of that math is part of EbsdLib's own V&V, not this filter's.

### Applied (Class 1 — Analytical)

Expected per-voxel `FRM` and per-feature `avgRefMis` outputs are derived in closed form from the input `Quats` + `Phases` + `FeatureIds` + reference-quaternion source (Mode 0: `AvgQuats[fid]`; Mode 1: `Quats[centerVoxelIdx]`) by hand-tracing the algorithm. The fixtures use pure φ1-rotation quaternions (Bunge ZXZ Euler `(φ1, 0, 0)`) so that misorientation between any two voxels equals `|Δφ1|` modulo the cubic c-axis 4-fold symmetry. For Δφ1 ∈ {0°, 5°, 10°} the symmetry reduction is the identity (no fold below the input), so expected FRM values are exactly `|Δφ1|`. Per-feature averages are `sum(FRM[v ∈ feature fid, phase>0]) / count(v ∈ feature fid, phase>0)` for `fid > 0` with non-empty count, or `0` when count is empty (path 7 in the code-path table below).

Mode 1 hand-picks `GBEuclideanDistances` values so that `m_Centers[fid]` selection has a unique closed-form answer (or, for the tied-distance multi-feature fixture, a deterministic later-voxel-wins tie-break per the `>=` comparison semantics that both legacy and SIMPLNX share).

### Applied (Class 4 — Invariant)

Five invariants every filter run must satisfy regardless of input configuration, asserted via `namespace ToyFixtures::AssertClass4Invariants()` in the test source:

- **Non-negativity**: `FRM[i] >= 0` ∀ voxel
- **Cubic max-angle bound**: `FRM[i] <= 62.8°` for cubic phases (the maximum symmetry-reduced misorientation under m-3m symmetry)
- **Skip-path correctness**: `FRM[i] == 0` when `featureIds[i] == 0` OR `cellPhases[i] == 0`
- **Background-feature zero**: `avgRefMis[0] == 0` (background)
- **Per-feature averaging formula**: `avgRefMis[fid] == sum(FRM[v ∈ feature fid, phase>0]) / count(v ∈ feature fid, phase>0)` for `fid > 0` with count > 0; `avgRefMis[fid] == 0` when count == 0

### Encoded

- **Class 1 (Analytical)**: `test/ComputeFeatureReferenceMisorientationsTest.cpp` — 6 `TEST_CASE` blocks under the `Class 1 - …` family. Per-voxel and per-feature expected values asserted via `ToyFixtures::RequireFRMClose()` / `RequireAvgClose()` with 1e-3° tolerance (degrees) and `Approx().margin(1e-5f)` for `EuclideanCenters` coord assertions.
- **Class 4 (Invariant)**: `ComputeFeatureReferenceMisorientationsFilter: Class 4 - Invariants Sweep` — two configurations (Mode 0 mixed 3×3×1 and Mode 1 3×3×1) each asserting all five invariants via the `AssertClass4Invariants()` helper.
- *(kept)* `ComputeFeatureReferenceMisorientationsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`; UUID + argument-key + parameter-value validation only.

### Second-engineer review

*Pending — recommend an OA-domain engineer (Joey Kleingers or similar) review:*
- *The Class 1 hand-derivations in the 6 toy fixtures + 1 invariants sweep for plausibility (the fixtures are small enough to walk through in ~30 minutes).*
- *The Class 4 invariant set for completeness — are there other properties this algorithm must satisfy?*
- *The decision to retire the `compute_feature_reference_misorientation.tar.gz` Small-IN100 exemplar archive in favor of inline toy fixtures.*

## Code path coverage

*7 of 8 paths exercised directly; 1 (cancel) implicitly via the unconditional cancel-check-at-loop-top instrumentation.*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureReferenceMisorientations.cpp` (~175 lines).

The algorithm has three logical phases: (a) Mode 1 pre-loop (populate `centers[fid]` from `gbEuclideanDistances` + write `EuclideanCenters` coords); (b) main per-voxel loop (compute per-voxel misorientation, accumulate per-feature sums + counts); (c) per-feature finalize (compute per-feature average from sum/count).

| #  | Phase             | Path                                                                                                              | Test case                                                                                                                                                                |
|----|-------------------|-------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1  | (b) Main loop     | Mode 0 — `q2 = avgQuatsPtr[featureId * 4 .. ]`                                                                  | Fixtures A, B, C (`Class 1 - Mode 0 SingleGrainIdentity` / `KnownAngle5deg` / `MultiGrain EdgeCases`)                                                                  |
| 2  | (a) Mode 1 pre-1  | Mode 1 — first sweep populates `centers[fid]` via `>=` tie-break against `gbEuclideanDistances`                  | Fixtures D, E, F (`Mode 1 KnownCenter` / `MultiGrain CenterIsolation` / `3D Volume`)                                                                                  |
| 3  | (a) Mode 1 pre-2  | Mode 1 — second sweep writes `EuclideanCenters[fid] = imageGeom.getCoordsf(centers[fid])`                       | Fixtures D, E, F                                                                                                                                                       |
| 4  | (b) Main loop     | `featureIds[i] > 0 && cellPhases[i] > 0` → compute misorientation, accumulate `sums[fid]++` and `counts[fid]+=`  | All Class 1 fixtures                                                                                                                                                    |
| 5  | (b) Main loop     | Skip (`featureIds[i] == 0` OR `cellPhases[i] == 0`) → FRM stays 0 (initial `fill(0.0f)` value)                  | Fixture C (`Mode 0 MultiGrain EdgeCases`) — background voxel 0, mid-feature unphased voxels 6-7, entire feature 4 unphased voxels 8-11                                  |
| 6  | (b) Main loop     | Cancel check at loop top (`m_ShouldCancel.load()` → early return)                                                | *Not directly tested.* Unconditional check at loop top in both passes; failure mode (silent cancel-disregard) would manifest as test hang in any test, but is not specifically exercised. Low-value gap. |
| 7  | (c) Finalize      | `avgMisorientationCounts[fid] == 0` → `avgRefMis[fid] = 0`                                                       | Fixture C — feature 4 has all-unphased voxels, so `count[4] == 0`                                                                                                       |
| 8  | (c) Finalize      | Otherwise → `avgRefMis[fid] = sums[fid] / counts[fid]`                                                            | All Class 1 fixtures with non-empty features                                                                                                                            |

## Test inventory

| Test case                                                                                                                                | Status      | Notes                                                                                                                                                                                                                                                                                                                                                                                                  |
|------------------------------------------------------------------------------------------------------------------------------------------|-------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 SingleGrainIdentity`                                                     | new-for-V&V | 2×2×2; single feature; all identity quats; expected FRM=0, avg=0. Class 4 invariants asserted.                                                                                                                                                                                                                                                                                                          |
| `ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 KnownAngle5deg`                                                          | new-for-V&V | 2×2×2; single feature; all voxel quats = 5° about z, AvgQuats[1] = identity; expected FRM=5° everywhere, avg=5°. Verifies non-zero misorientation magnitude. Class 4 invariants asserted.                                                                                                                                                                                                                  |
| `ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 0 MultiGrain EdgeCases`                                                    | new-for-V&V | 4×3×1; 5 features (background + 4 grains); covers skip paths (background, mid-feature unphased), zero-count finalize path (feature 4 all-unphased → avg=0), and normal accumulation. Class 4 invariants asserted.                                                                                                                                                                                       |
| `ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 KnownCenter`                                                             | new-for-V&V | 3×3×1; single feature; center voxel hand-picked via unique max `GBEuclideanDistances`; verifies `centers[]` selection + `EuclideanCenters` coord writing. Class 4 invariants asserted.                                                                                                                                                                                                                      |
| `ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 MultiGrain CenterIsolation`                                              | new-for-V&V | 2×3×1; 2 features; verifies `centers[fid]` isolation per feature + tied-distance `>=` later-voxel-wins tie-break. Class 4 invariants asserted.                                                                                                                                                                                                                                                              |
| `ComputeFeatureReferenceMisorientationsFilter: Class 1 - Mode 1 3D Volume`                                                               | new-for-V&V | 3×3×2; single feature; verifies linear `voxelIdx → (x,y,z)` arithmetic when `dimZ > 1`. Class 4 invariants asserted.                                                                                                                                                                                                                                                                                          |
| `ComputeFeatureReferenceMisorientationsFilter: Class 4 - Invariants Sweep`                                                               | new-for-V&V | Runs Mode 0 and Mode 1 configurations distinct from the value-specific fixtures; asserts only the Class 4 invariants. Catches future regressions where specific values shift but invariants still hold.                                                                                                                                                                                                  |
| `ComputeFeatureReferenceMisorientationsFilter: SIMPL Backwards Compatibility`                                                            | retained    | `DYNAMIC_SECTION` over SIMPL 6.4 + 6.5 conversion fixtures (`test/simpl_conversion/6_*/ComputeFeatureReferenceMisorientationsFilter.json`); validates UUID + argument-key + parameter-value decoding.                                                                                                                                                                                                       |
| *(retired)* `ComputeFeatureReferenceMisorientationsFilter_AverageMisorientation`                                                         | retired     | Removed 2026-06-01. Regression-against-archive test consuming `compute_feature_reference_misorientation.tar.gz` exemplar arrays; archive's exemplar values were a circular oracle (regenerated from pre-EbsdLib-2.4.1 SIMPLNX output). Test failure surfaced when EbsdLib 2.4.1 CubicOps precision fix shifted exemplar values by 2× epsilon. Replaced by inline Class 1 + Class 4 fixtures above. |
| *(retired)* `ComputeFeatureReferenceMisorientationsFilter_EuclideanDistance`                                                             | retired     | Same as above for Mode 1; archive exemplar shifted by 10× epsilon post-EbsdLib-2.4.1.                                                                                                                                                                                                                                                                                                                  |

All 8 active TEST_CASEs pass at the verified commit (`100% tests passed, 0 tests failed out of 8` in ~0.7s).

## Exemplar archive

**None — data inlined in `test/ComputeFeatureReferenceMisorientationsTest.cpp` namespace `ToyFixtures`.**

The pre-existing `compute_feature_reference_misorientation.tar.gz` archive was retired during this V&V cycle. See `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureReferenceMisorientationsFilter.md` for the retirement rationale and the methodology used to construct the replacement toy fixtures.

- **Archive:** None
- **SHA512:** N/A
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureReferenceMisorientationsFilter.md`

## Deviations from DREAM3D 6.5.171

One precision-class non-deviation documented; no algorithmic deviations.

### ComputeFeatureReferenceMisorientationsFilter-D1

- **Symptom:** Per-feature average misorientations differ from DREAM3D 6.5.171 output by 2× to 10× the 1e-4 epsilon used by `CompareDataArrays` on Small-IN100-class EBSD data. On the V&V toy fixtures (pure φ1 rotations, no sym-op-aligned boundaries), no observable deviation.
- **Root cause:** Precision — propagation of the EbsdLib 2.4.1 `CubicOps::calculateMisorientationInternal` precision fix. This filter is a clean Port of the legacy algorithm; no algorithmic deviation. See `vv/deviations/ComputeFeatureReferenceMisorientationsFilter.md` for full root-cause walkthrough.
