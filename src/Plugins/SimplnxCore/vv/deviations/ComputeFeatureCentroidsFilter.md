# Deviations from DREAM.3D v6.5.171: ComputeFeatureCentroidsFilter

Part 1 records the coordinate-precision difference and the added periodic mode. Part 2 records the two fixed SIMPLNX periodic-centroid defects. Both parts use the stable `ComputeFeatureCentroidsFilter-D<N>` sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureCentroidsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureCentroidsFilter-D1` |
| **Filter UUID** | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| **Status** | active |

**Symptom:** Non-periodic centroid components can differ from DREAM.3D v6.5.171 because the implementations use different coordinate precision.

**Root cause:** Precision. Both implementations use double-precision Kahan sums and store float32 centroids. DREAM.3D v6.5.171 reads each cell center into `std::array<float, 3>` before accumulation. SIMPLNX calculates `origin + (index + 0.5) * spacing` with double-precision operands in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.cpp`.

**Affected users:** Users who require identical floating-point centroid values when migrating from DREAM.3D v6.5.171.

**Recommendation:** Trust SIMPLNX. Its coordinate calculation avoids the earlier float32 rounding; compare numerical results with a tolerance appropriate to the data.

**Evidence:** `Source/Plugins/Generic/GenericFilters/FindFeatureCentroids.cpp` at the DREAM.3D `v6.5.171` tag contains the float coordinate buffer and Kahan kernel. The current SIMPLNX algorithm contains explicit double-precision coordinate expressions. Both sources divide each accumulated coordinate by its cell count.

---

### ComputeFeatureCentroidsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureCentroidsFilter-D2` |
| **Filter UUID** | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| **Status** | active |

**Symptom:** Enabling `Is Periodic` can change centroid components for features that touch both ends of an image axis. DREAM.3D v6.5.171 always calculates arithmetic means.

**Root cause:** Algorithmic choice. SIMPLNX adds a circular mean for each spanning axis. A normalized unit-vector resultant below `1e-6` preserves the arithmetic mean. `Is Periodic` defaults to false, including after SIMPL pipeline conversion. With periodic mode off, the precision difference in D1 still applies.

**Affected users:** Users who enable `Is Periodic`. Automatic conversion of a DREAM.3D pipeline does not enable it.

**Recommendation:** Trust SIMPLNX. Enable periodic mode for periodic geometry; keep it disabled for ordinary arithmetic centroids. D3 and D4 describe corrections to this added mode.

**Evidence:** `ComputeFeatureCentroidsFilter::parameters()` defines the false default, and `FromSIMPLJson()` starts from default arguments. DREAM.3D `v6.5.171` `FindFeatureCentroids` has no periodic parameter or calculation. Fixtures D–F in `src/Plugins/SimplnxCore/test/ComputeFeatureCentroidsTest.cpp` check the periodic behavior independently.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureCentroidsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureCentroidsFilter-D3` |
| **Filter UUID** | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.1 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** With non-unit spacing, the periodic centroid offset used cell-index units instead of physical units.

**Root cause:** Bug. The image overload of `AdjustCentroidsForPeriodicFaces` added `(dimension - 1) / 2` directly to a physical centroid. DREAM3D-NX v7.4.2 multiplies that offset by the axis spacing. The current circular-mean calculation also includes spacing in cell coordinates and domain lengths. DREAM.3D v6.5.171 has no periodic code path.

**Affected users:** Users who enable periodic mode with non-unit spacing on a spanning axis in the affected releases.

**Recommendation:** Trust SIMPLNX. DREAM3D-NX v7.4.2 corrects the spacing units, but D4 still affects its periodic results; the release after v7.4.2 contains both corrections.

**Evidence:** Direct inspection of `ComputeFeatureCentroids.cpp` and `src/simplnx/Utilities/GeometryHelpers.cpp` on all ten release branches shows no periodic mode through v7.2.0, an unscaled offset from v7.2.1 through v7.4.1, and the scaled offset in v7.4.2. PR #1658 (`ded8bf33f`) applies the spacing correction. Current fixture E checks physical X = 11 with spacing 2 and origin 10 after both D3 and D4 corrections.

---

### ComputeFeatureCentroidsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureCentroidsFilter-D4` |
| **Filter UUID** | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** The periodic mode could return an incorrect centroid for an asymmetric wrapped feature or place the centroid outside the image domain.

**Root cause:** Bug. A constant half-span offset cannot represent the distribution of a feature's cells across a periodic boundary. The current calculation accumulates cell-center unit vectors, converts their mean angle to a physical coordinate and retains the arithmetic mean when the resultant is below `1e-6`. DREAM.3D v6.5.171 has no periodic code path.

**Affected users:** Users who enable periodic mode in the affected releases, especially for asymmetric wrapped features or features that fill the domain.

**Recommendation:** Trust SIMPLNX. Use the DREAM3D-NX release after v7.4.2 for the corrected periodic calculation. Disable periodic mode only when an ordinary arithmetic centroid is appropriate.

**Evidence:** The periodic implementation in `upstream/release/v7.2.1` through `upstream/release/v7.4.2` calls the constant-offset image helper. PR #1670 (`4a7b71e5f`) replaces that model with the circular mean and removes the image helper. Current fixtures D and E independently unwrap coordinates to obtain X = 3.5 and X = 11; fixture F requires the arithmetic fallback X = 2.
