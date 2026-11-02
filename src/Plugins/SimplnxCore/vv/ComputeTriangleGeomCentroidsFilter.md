# V&V Report: ComputeTriangleGeomCentroidsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| SIMPLNX Human Name | Compute Feature Centroids from Triangle Geometry |
| DREAM.3D v6.5.171 equivalent | `FindTriangleGeomCentroids` (SurfaceMeshing plugin) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-13; second engineer: Jared Duffey, 2026-10-06 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes. SIMPLNX retains the unique-vertex mean from `FindTriangleGeomCentroids` in DREAM.3D v6.5.171 and adds periodic adjustment and feature-matrix growth. |
| Oracle (confirmed) | Classes 1 and 4: analytical centroids and geometry bounds on one constructed mesh in `test/ComputeTriangleGeomCentroidsTest.cpp`. All pass in CI. |
| Code paths enumerated | 11 of 22 exercised. Gaps include resize, cancellation, invalid matrix selection, Y/Z periodic adjustment, and defensive helper paths. |
| Tests today | 2 test cases: one non-periodic archive comparison and one analytical mesh with periodic mode off and on. All pass in CI. |
| Exemplar archive | `12_IN625_GBCD.tar.gz` supplies a surface mesh and non-periodic reference centroids. The independent oracle is constructed in the test code. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None |
| V&V phase | Source review, analytical verification, and regressions for both fixed defects are complete. Second-engineer review is complete. |

## Summary

`ComputeTriangleGeomCentroidsFilter` calculates each feature's centroid from its unique triangle vertices, with optional periodic adjustment. Analytical centroid values and geometry-bound invariants verify both mode settings, and an archive supplies a non-periodic regression comparison. The V&V fixed two periodic defects; all pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* Both implementations average the unique vertices of positive face labels; SIMPLNX adds periodic adjustment and grows the feature Attribute Matrix when a label needs another tuple.

## Oracle

*Class:* 1 (Analytical) and 4 (Invariant).

*Applied:* One mesh has four features with hand-derived non-periodic and periodic centroids; every non-empty centroid must remain inside the geometry bounds.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeTriangleGeomCentroidsTest.cpp::SimplnxCore::ComputeTriangleGeomCentroids: Periodic Minimum-Image Oracle` — one fixture, four populated features and one empty feature, with `IsPeriodic = false` and `IsPeriodic = true`. All pass in CI.

*Second-engineer review:* Jared Duffey, 2026-10-06.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeTriangleGeomCentroidsFilter-D3` | A constant half-domain offset gave incorrect periodic centroids and could put them outside the geometry bounds. | DREAM3D-NX v7.2.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. | The filter calculates a minimum-image mean from vertex coordinates and uses the arithmetic mean when the largest gaps tie. |
| `ComputeTriangleGeomCentroidsFilter-D4` | Periodic adjustment could throw `std::bad_function_call` when no message callback was installed. | DREAM3D-NX v7.2.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. | The filter sends the message through a handler that checks whether a callback exists. |

## Code path coverage

11 of 22 paths exercised. Each uncovered row states the missing input or the guard that prevents entry.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeTriangleGeomCentroids.cpp` (121 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeTriangleGeomCentroidsFilter.cpp` (131 lines), and `src/simplnx/Utilities/GeometryHelpers.cpp` (351 lines).

`Oracle` below means `SimplnxCore::ComputeTriangleGeomCentroids: Periodic Minimum-Image Oracle`. `Exemplar` means `SimplnxCore::ComputeTriangleGeomCentroids`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Gather | A positive first face label adds the triangle's vertices to its feature set | Oracle — F1 through F4 |
| 2 | Gather | Ignore zero or negative face labels | Oracle — second label 0; Exemplar — first label -1 |
| 3 | Finalize | A non-empty feature gets its arithmetic mean with periodic mode disabled | Oracle — `IsPeriodic = false`, F1 through F4 |
| 4 | Finalize | An empty feature keeps `(0,0,0)`; periodic-face detection returns no faces | Oracle — F0 |
| 5 | Periodic | X touches both opposing faces; a unique largest gap gives the minimum-image mean | Oracle — `IsPeriodic = true`, F2 and F3 |
| 6 | Periodic | An axis does not touch both opposing faces; keep its component | Oracle — F1 and all Y/Z components |
| 7 | Periodic | Multiple largest gaps tie; use the arithmetic mean | Oracle — `IsPeriodic = true`, F4 |
| 8 | Message | Periodic adjustment requests an information message with no installed callback | Oracle — `IsPeriodic = true`, F2 through F4 |
| 9 | Resize | Grow the feature Attribute Matrix to `maxFeatureId + 1` tuples | *Not directly tested. Both fixtures already have enough feature tuples.* |
| 10 | Resize | Return a feature-matrix resize error | *Not directly tested. Requires an injected resize failure.* |
| 11 | Gather | Return when cancellation is requested during triangle traversal | *Not directly tested. Neither test requests cancellation.* |
| 12 | Finalize | Return when cancellation is requested during feature traversal | *Not directly tested. Neither test requests cancellation.* |
| 13 | Preflight | Missing or wrong-type feature Attribute Matrix returns `-12901` | *Not directly tested. Both fixtures select a valid Attribute Matrix.* |
| 14 | Preflight | Valid feature Attribute Matrix creates a float32 centroid array with three components | Oracle; Exemplar |
| 15 | Periodic | Y touches both opposing faces; adjust its centroid component | *Not directly tested. Each oracle feature has a single Y coordinate.* |
| 16 | Periodic | Z touches both opposing faces; adjust its centroid component | *Not directly tested. All oracle vertices have Z = 0.* |
| 17 | Helper | An empty coordinate list returns the domain minimum | *Not directly tested. The filter calls the adjustment only for non-empty feature sets.* |
| 18 | Helper | A non-positive domain length returns the arithmetic mean | *Not directly tested. Periodic-face detection cannot select both faces of a zero-length axis.* |
| 19 | Helper | Normalize a negative coordinate remainder into the domain | *Not directly tested. The geometry minimum bounds every feature coordinate.* |
| 20 | Helper | Normalize a negative final mean remainder into the domain | *Not directly tested. Normalized coordinates and a positive domain length give a non-negative sum.* |
| 21 | Gather | A positive second face label adds the triangle's vertices to its feature set | Exemplar — positive second labels and full centroid-array comparison |
| 22 | Resize | Keep the existing feature-matrix size when it is sufficient | Oracle; Exemplar |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeTriangleGeomCentroids` | kept | Loads `12_IN625_GBCD.tar.gz`, compares the full non-periodic centroid array with the stored array, and checks tuple dimensions. |
| `SimplnxCore::ComputeTriangleGeomCentroids: Periodic Minimum-Image Oracle` | new-for-V&V | `IsPeriodic = false` and `IsPeriodic = true` each check 15 centroid components within absolute tolerance `1e-4`, bounds for four populated features, and tuple dimensions. Covers D3 and D4. |

## Exemplar archive

- **Archive:** `12_IN625_GBCD.tar.gz`
- **SHA512:** `f696a8af181505947e6fecfdb1a11fda6c762bba5e85fea8d484b1af00bf18643e1d930d48f092ee238d1c19c9ce7c4fb5a8092d17774bda867961a1400e9cea`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/12_IN625_GBCD.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeTriangleGeomCentroidsFilter.md` for the source-based differences in Part 1.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeTriangleGeomCentroidsFilter-D1` | SIMPLNX provides periodic centroid adjustment; DREAM.3D v6.5.171 has no periodic mode. |
| `ComputeTriangleGeomCentroidsFilter-D2` | SIMPLNX grows an undersized feature Attribute Matrix. DREAM.3D v6.5.171 rejects labels above its tuple count and omits the centroid for a label equal to that count. |
