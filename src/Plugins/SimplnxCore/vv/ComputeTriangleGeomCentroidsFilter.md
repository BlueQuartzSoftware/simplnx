# V&V Report: ComputeTriangleGeomCentroidsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | 074c0523-ab7a-4097-b0c3-c4dcbfb9a05e |
| DREAM3D 6.5.171 equivalent | FindTriangleGeomCentroids (DREAM3DReview) — no `FromSIMPLJson` conversion in SimplnxCore |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *pending second-engineer review* |

## At a glance

A scannable dashboard for reviewers. Each row is one sentence to one short paragraph — enough that a reader can decide whether they need to read the long-form sections below.

| Aspect                 | Current state                                                                                                                |
|------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Minor changes** — Port of the non-periodic core from legacy `FindTriangleGeomCentroids` (DREAM3DReview); `Is Periodic` is a SIMPLNX-only addition, now reimplemented as a minimum-image mean. |
| Oracle (confirmed)     | **Class 1 + Class 4** — hand-built wrapping mesh with closed-form minimum-image centroids; encoded in `ComputeTriangleGeomCentroidsTest.cpp::"Periodic Minimum-Image Oracle"`, passes in-core **and out-of-core**. |
| Code paths enumerated  | **8 of 9 exercised**; the feature-AM resize guard (path 9) is left untested (pre-sized AM, low risk). |
| Tests today            | **2 test cases** — the kept `12_IN625_GBCD` exemplar (non-periodic) + the new Class 1/4 periodic oracle (`GENERATE` over `IsPeriodic`). |
| Exemplar archive       | `12_IN625_GBCD.tar.gz` — non-periodic exemplar inputs+output (kept). Periodic oracle is inline (no archive). |
| Legacy comparison      | **Not run yet** — legacy `FindTriangleGeomCentroids` has no periodic mode; non-periodic diff pending Step 8. |
| Bug flags              | **2 SIMPLNX bugs found & fixed:** (1) constant half-domain periodic offset (issue #1665) → minimum-image mean; (2) `m_MessageHandler.m_Callback()` direct call → `std::bad_function_call` when periodic branch fired. |
| V&V phase              | **All Triangle-filter V&V work complete:** discovery, Class 1/4 oracle, SIMPLNX-vs-oracle reconciliation + 2 bug fixes, algorithm review, dual-build (in-core + OOC) test pass, legacy diff (2 deviations), docs, provenance. **Status:** READY FOR REVIEW, pending second-engineer sign-off at PR. The sibling `ComputeFeatureCentroids` `ImageGeom` overload carried the same constant-offset defect; it was reimplemented as a circular mean and the overload removed in the same branch (see `vv/ComputeFeatureCentroidsFilter.md` Follow-up). |

For worked instances see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md` and `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md` (on `topic/vv/compute_avg_caxis`).

## Summary

`ComputeTriangleGeomCentroids` computes the centroid of each surface-mesh feature as the mean position of the unique vertices of the triangles carrying that feature's face label, with an optional `Is Periodic` mode for features that wrap the domain boundary. V&V used a **Class 1 (Analytical)** hand-built wrapping mesh (four features: non-wrapping, symmetric wrap, asymmetric wrap, domain-filling) with closed-form minimum-image centroids, plus a **Class 4 (Invariant)** in-domain bound. The periodic path was found to use a **constant half-domain offset** that is only correct for mass symmetric about the seam (issue #1665); it was reimplemented as a largest-empty-gap minimum-image mean. A second latent bug — an unguarded `m_Callback` invocation that threw `std::bad_function_call` when the periodic branch fired — was also fixed.

## Algorithm Relationship

**Minor changes** (Port of the non-periodic core + SIMPLNX-only periodic addition)

*Evidence:* No `FromSIMPLJson` conversion / not in `SimplnxCoreLegacyUUIDMapping`; the non-periodic centroid (arithmetic mean of a feature's unique vertices) mirrors legacy `FindTriangleGeomCentroids` (DREAM3DReview). The `Is Periodic` option and its wrap adjustment are SIMPLNX-only, so there is no legacy periodic behavior to preserve — the periodic path is judged on correctness alone. *(Legacy inspection pending Step 8.)*

## Oracle

*Class:* **1 (Analytical)** + **4 (Invariant)**

*Applied:* A hand-built triangle mesh on periodic domain X∈[0,4] (all z=0, one distinct y per feature, so only X trips the periodic condition). Expected minimum-image centroids derived by hand via the largest-empty-gap method: F1 non-wrapping (2,1,0); F2 symmetric wrap (0,2,0); F3 asymmetric wrap (3.625,3,0) — **the discriminating case: the old constant offset yields 4.625, outside the domain**; F4 domain-filling → arithmetic-mean fallback (2,1,0). Class 4: every non-empty centroid lies within the geometry bounding box.

*Encoded:* `test/ComputeTriangleGeomCentroidsTest.cpp::"Periodic Minimum-Image Oracle"` — `GENERATE(false, true)` over `IsPeriodic`, 4 features × 3 components + in-bounds invariant. Passes **in-core and out-of-core** (`simplnx-ooc-Rel`).

*Second-engineer review:* *pending — the oracle design + hand-derived F3=3.625 value were confirmed with the requesting engineer before encoding; formal second-engineer review at PR.*

## Code path coverage

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeTriangleGeomCentroids.cpp` (~108 lines) + the `AdjustCentroidsForPeriodicFaces` `BoundingBox` overload in `src/simplnx/Utilities/GeometryHelpers.cpp`. Phases: (a) gather per-feature vertex sets from face labels, (b) per-feature arithmetic centroid, (c) optional periodic minimum-image adjustment.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) gather | `faceLabel > 0` inserts triangle vertices into the feature set | both oracle sections (all features) |
| 2 | (a) gather | `faceLabel == 0/negative` ignored | Oracle — second label 0 on every triangle; feature 0 stays empty |
| 3 | (b) finalize | non-empty feature → arithmetic mean | `IsPeriodic=false` section (F1–F4) |
| 4 | (b) finalize | empty feature set → default (0,0,0) | Oracle — F0 asserted (0,0,0) |
| 5 | (c) periodic | axis spans both faces → minimum-image mean | `IsPeriodic=true` — F2 (unique wrap gap), F3 (unique interior gap) |
| 6 | (c) periodic | axis does not span → component unchanged | `IsPeriodic=true` — F1, and y/z of F2–F4 |
| 7 | (c) periodic | domain-filling (no dominant gap) → arithmetic fallback | `IsPeriodic=true` — F4 |
| 8 | (c) periodic | any axis adjusted → emit info message | Exercised by F2–F4 (regression pin for the `bad_function_call` fix) |
| 9 | resize | feature AM smaller than maxFeatureId+1 → resize | *Not directly tested — the exemplar test's AM is pre-sized; low-risk resize guard.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeTriangleGeomCentroids` (exemplar, `12_IN625_GBCD`) | kept | Non-periodic exemplar comparison; unchanged. |
| `SimplnxCore::ComputeTriangleGeomCentroids: Periodic Minimum-Image Oracle` | new-for-V&V | Class 1 + Class 4; covers periodic + non-periodic; pins both bug fixes. |

## Exemplar archive

- **Archive:** `12_IN625_GBCD.tar.gz` (pre-existing; declared in `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`, shared with several surface-mesh tests). Not regenerated by this V&V.
- **SHA512:** `f696a8af181505947e6fecfdb1a11fda6c762bba5e85fea8d484b1af00bf18643e1d930d48f092ee238d1c19c9ce7c4fb5a8092d17774bda867961a1400e9cea`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/12_IN625_GBCD.md`
- **Note:** The archive's `Centroids` reference array is a legacy-derived regression pin (circular oracle) for the non-periodic path only. The authoritative oracle is the inline Class 1 + Class 4 fixture; the periodic branch is not exercised by this archive.

## Deviations from DREAM3D 6.5.171

Comparison method: source inspection against legacy `FindTriangleGeomCentroids` (SurfaceMeshing plugin). The non-periodic path is a line-for-line match (identical `> 0` label guard, identical float32 arithmetic-mean of set-ordered vertices), so it is bit-identical; two behavioral deviations are documented.

- `ComputeTriangleGeomCentroidsFilter-D1` — `Is Periodic` is a SIMPLNX-only mode; its wrapped centroid (minimum-image mean) has no 6.5.171 analog — see `vv/deviations/ComputeTriangleGeomCentroidsFilter.md`
- `ComputeTriangleGeomCentroidsFilter-D2` — out-of-range face label: 6.5.171 errors (`-99500`), SIMPLNX resizes the Feature Attribute Matrix — see `vv/deviations/ComputeTriangleGeomCentroidsFilter.md`
