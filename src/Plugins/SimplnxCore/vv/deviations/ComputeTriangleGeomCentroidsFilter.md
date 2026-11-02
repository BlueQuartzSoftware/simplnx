# Deviations from DREAM.3D v6.5.171: ComputeTriangleGeomCentroidsFilter

Part 1 records differences from `FindTriangleGeomCentroids` in the SurfaceMeshing plugin. Part 2 records the two fixed SIMPLNX periodic defects. Both parts use one stable ID sequence.

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeTriangleGeomCentroidsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeTriangleGeomCentroidsFilter-D1` |
| **Filter UUID** | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| **Status** | active |

**Symptom:** SIMPLNX offers an `Is Periodic` option that DREAM.3D v6.5.171 does not provide.

**Root cause:** Algorithmic choice. Both filters calculate the arithmetic mean of each feature's unique vertices. SIMPLNX can also adjust an axis when a feature touches both opposing geometry faces. It calculates a minimum-image mean around the largest empty gap in the vertex coordinates. If the largest gaps tie, it keeps the arithmetic mean. The option defaults to false.

**Affected users:** Users who enable `Is Periodic` for a surface mesh that wraps across domain boundaries.

**Recommendation:** Trust SIMPLNX. The analytical fixture checks symmetric wrapping, asymmetric wrapping, and the tied-gap fallback. DREAM.3D v6.5.171 has no periodic output for comparison.

**Evidence:** The DREAM.3D `v6.5.171` source at `Source/Plugins/SurfaceMeshing/SurfaceMeshingFilters/FindTriangleGeomCentroids.cpp` has no periodic parameter or adjustment. `Periodic Minimum-Image Oracle` in `src/Plugins/SimplnxCore/test/ComputeTriangleGeomCentroidsTest.cpp` checks both settings against literal centroid values.

---

### ComputeTriangleGeomCentroidsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeTriangleGeomCentroidsFilter-D2` |
| **Filter UUID** | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| **Status** | active |

**Symptom:** SIMPLNX grows an undersized feature Attribute Matrix. DREAM.3D v6.5.171 reports `-99500` for a positive face label greater than the matrix tuple count; a label equal to that count receives no centroid.

**Root cause:** Algorithmic choice. With N feature tuples, DREAM.3D v6.5.171 allocates N + 1 vertex sets and rejects labels at least N + 1. It calculates centroids only for indices below N. Thus label N enters a set but has no output tuple. SIMPLNX grows the matrix to `maxFeatureId + 1` tuples before gathering vertices and returns an error if resizing fails.

**Affected users:** Users with a positive face label greater than or equal to the feature-matrix tuple count.

**Recommendation:** Trust SIMPLNX. It provides an output tuple for each positive label. Add a separate size check if a pipeline requires rejection of an undersized matrix.

**Evidence:** DREAM.3D tag `v6.5.171`, `Source/Plugins/SurfaceMeshing/SurfaceMeshingFilters/FindTriangleGeomCentroids.cpp`, contains the N + 1 guard and the N-bound output loop. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeTriangleGeomCentroids.cpp` contains the resize and error propagation. Neither current test directly exercises the resize.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeTriangleGeomCentroidsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeTriangleGeomCentroidsFilter-D3` |
| **Filter UUID** | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Periodic adjustment could give an incorrect centroid outside the geometry bounds.

**Root cause:** Bug. The adjustment added half the domain length to the arithmetic mean without considering the vertex distribution. On X in [0,4], vertices at `{0,3,3.5,4}` gave 4.625 instead of 3.625. The filter now uses the largest-empty-gap minimum-image mean and maps the result into the domain. Equal largest gaps select the arithmetic mean.

**Affected users:** Users who enable `Is Periodic` for features that touch both opposing faces on an axis. DREAM3D-NX v7.0.0 through v7.2.0 and DREAM.3D v6.5.171 have no periodic path.

**Recommendation:** Use the DREAM3D-NX release after v7.4.2 for periodic centroids. Disabling periodic mode avoids this adjustment but gives the ordinary arithmetic mean.

**Evidence:** All release sources from `upstream/release/v7.2.1` through `upstream/release/v7.4.2` add the constant offset in `src/simplnx/Utilities/GeometryHelpers.cpp`. PR #1670 (`4a7b71e5f`) replaces it with `PeriodicMean1D`. The `Periodic Minimum-Image Oracle` test checks F3 = `(3.625,3,0)` and geometry bounds; all pass in CI at `419f9db35`.

---

### ComputeTriangleGeomCentroidsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeTriangleGeomCentroidsFilter-D4` |
| **Filter UUID** | `074c0523-ab7a-4097-b0c3-c4dcbfb9a05e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** Periodic adjustment could stop with `std::bad_function_call` when no message callback was installed.

**Root cause:** Bug. After periodic adjustment, the algorithm invoked `m_MessageHandler.m_Callback` directly. An empty callback throws. The filter now uses `sendMessage`, which checks the callback before invoking it.

**Affected users:** API users who execute periodic adjustment without a message callback. DREAM3D-NX v7.0.0 through v7.2.0 and DREAM.3D v6.5.171 have no periodic path.

**Recommendation:** Use the DREAM3D-NX release after v7.4.2. In affected releases, install a message callback to avoid this exception; this does not correct D3.

**Evidence:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeTriangleGeomCentroids.cpp` directly invokes the callback in all release sources from `upstream/release/v7.2.1` through `upstream/release/v7.4.2`. PR #1670 (`4a7b71e5f`) adds the guarded call. `Periodic Minimum-Image Oracle` executes with no callback argument and verifies successful output; all pass in CI at `419f9db35`.
