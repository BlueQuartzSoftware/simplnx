# Exemplar Provenance: ComputeFeatureCentroidsFilter

**No exemplar archive.** As of this V&V cycle (2026-07-07) the filter's correctness tests are **inline Class 1 (Analytical) + Class 4 (Invariant) fixtures** built directly in `test/ComputeFeatureCentroidsTest.cpp` (`namespace CentroidToy`). There is no `.dream3d` gold-master to hash — the oracle lives in the test code as hand-derived `REQUIRE` values.

## Retired test / circular-oracle removal

The prior test `SimplnxCore::ComputeFeatureCentroidsFilter` compared a freshly computed `Centroids NX` array against a **sibling `Centroids` array in the same `6_6_stats_test_v2.dream3d` file**. That reference array was itself produced by an earlier DREAM3D run, making the check a **consistency-with-self / circular oracle** (see `docs/vv_templates/oracle_classes.md` "What is NOT an oracle"). It was **retired** and replaced by the analytical fixtures below.

- `6_6_stats_test_v2.tar.gz` — the centroids test no longer consumes it. The `download_test_data()` entry stays in `test/CMakeLists.txt` because five other tests still use it (`ComputeEuclideanDistMap`, `ComputeSurfaceAreaToVolume`, `ComputeFeatureNeighbors`, `ComputeFeatureSizes`, `ComputeFeaturePhases`).
- `6_6_find_feature_centroids.tar.gz` — **kept**; used by `ExtractComponentAsArray` and `WriteAbaqusHexahedron` tests (the retroactive report's "orphan" claim was incorrect).

## Canonical oracle output (Class 1 — hand derivation)

Centroid component = `Σ(voxel-center coord) / N` over the feature's cells, where `voxel-center = origin + (index + 0.5)·spacing`. Fixtures (all in `test/ComputeFeatureCentroidsTest.cpp`):

| Fixture | Grid / spacing | Purpose | Key expected value |
|---|---|---|---|
| A | 3×1×1 | basic mean + empty (count==0) background | centroid[1] = (1.5, 0.5, 0.5) |
| B | 4×2×1 | multi-feature, single-cell, empty id | centroid[1] = (5/6, 5/6, 0.5); centroid[2] = (3.0, 1.0, 0.5) |
| C | 2×2×2 | 3D z-stride | centroid[1] = (1,1,0.5); centroid[2] = (1,1,1.5) |
| D | 4×1×1 (spacing 1) | periodic wrap fires on spanning feature only | centroid[1] = 2.16667 + 1.5 = 3.66667 |
| E | 4×1×1 (spacing 2, origin 10) | periodic offset scales with spacing (regression pin for the D2 bug fix) | centroid[1].x = 17.0 (was 15.5 before fix) |
| (error) | 2×1×1, id 5 of 2 | validation error path (-5351) | execute invalid |

## Second-engineer oracle review

- **Reviewer:** *to be named* (developer selected "Yes — someone else" at the Phase 4/5 checkpoint; recommend a SimplnxCore-domain engineer).
- **Date:** *pending.*
- Fixtures A–E are small enough to walk in ~20 minutes; the key item to confirm is the Fixture E spacing-aware periodic value (17.0) and the D2 bug-fix rationale.

## Reproduction

The fixtures require no external data. Build the `SimplnxCoreUnitTest` target and run:

```
ctest -R "ComputeFeatureCentroids"
```

All hand derivations are in the code comments beside each `RequireCentroid` call and in `src/Plugins/SimplnxCore/vv/ComputeFeatureCentroidsFilter.md` (Phase 5).
