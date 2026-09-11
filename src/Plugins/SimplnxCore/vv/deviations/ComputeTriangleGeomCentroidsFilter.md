# Deviations from DREAM3D 6.5.171: ComputeTriangleGeomCentroidsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ComputeTriangleGeomCentroidsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

Legacy equivalent: `FindTriangleGeomCentroids` (DREAM3D 6.5.171, SurfaceMeshing plugin). Comparison method: source inspection of `FindTriangleGeomCentroids.cpp` against the SIMPLNX algorithm; the non-periodic path is a line-for-line match, so no empirical binary A/B was required (it can be run for bit-confirmation if desired).

---

## ComputeTriangleGeomCentroidsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeTriangleGeomCentroidsFilter-D1` |
| **Filter UUID** | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| **Status** | active |

**Symptom:** SIMPLNX exposes an `Is Periodic` option that DREAM3D 6.5.171 does not; when enabled, features that wrap the domain boundary receive a different (and correct) centroid than the naive arithmetic mean.

**Root cause:** Algorithmic choice (SIMPLNX-only feature). Legacy `FindTriangleGeomCentroids` has no periodic mode — it always reports the plain arithmetic mean of a feature's unique vertices. SIMPLNX adds `Is Periodic`; for any axis on which a feature touches both opposing periodic faces, the centroid component is computed as a minimum-image (largest-empty-gap) mean of the feature's vertex coordinates (`GeometryHelpers.cpp`, `AdjustCentroidsForPeriodicFaces` `BoundingBox3Df` overload) instead of the arithmetic mean. With `Is Periodic = false` (the default) SIMPLNX reproduces the legacy result.

**Affected users:** Only users who enable `Is Periodic` on a periodic surface mesh with grains that wrap the boundary. The default (off) path is unaffected and matches 6.5.171.

**Recommendation:** Trust SIMPLNX. 6.5.171 has no comparable output; the SIMPLNX periodic centroid is verified against an independent Class 1 analytical oracle (see the report's Oracle section).

---

## ComputeTriangleGeomCentroidsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeTriangleGeomCentroidsFilter-D2` |
| **Filter UUID** | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| **Status** | active |

**Symptom:** When a face label is larger than the number of tuples in the target Feature Attribute Matrix, 6.5.171 aborts with error `-99500`; SIMPLNX instead grows the Feature Attribute Matrix to fit and completes successfully.

**Root cause:** Algorithmic choice. Legacy sizes `vertexSets` to `numFeatures + 1` and errors (`-99500`) if any label meets or exceeds that bound (`FindTriangleGeomCentroids.cpp:185-192`). SIMPLNX resizes the Feature Attribute Matrix to `maxFeatureId + 1` tuples up front (`ComputeTriangleGeomCentroids.cpp:48-52`) and proceeds. The centroid values that both produce for in-range labels are identical.

**Affected users:** Anyone who supplied a Feature Attribute Matrix smaller than `max(faceLabel) + 1`. Under 6.5.171 the run failed; under SIMPLNX it succeeds with an enlarged matrix.

**Recommendation:** Either acceptable. SIMPLNX is strictly more permissive; the computed centroids agree for all valid labels. Users porting a pipeline that relied on the `-99500` error as a validation gate should add an explicit size check.

---

## Note on the periodic bug fix (internal, not a legacy deviation)

Prior to this V&V, SIMPLNX's periodic path added a **constant `|max − min| / 2` offset** to the naive centroid (issue #1665). That is only correct for a feature whose mass is symmetric about the seam and can place the centroid outside the domain for asymmetric wrapped features. It was corrected to the minimum-image mean described in D1. This is a SIMPLNX-internal correction, **not** a deviation from 6.5.171 (which has no periodic path), so it is recorded here only for provenance.
