# Deviations from DREAM.3D v6.5.171: ComputeFeatureSizesFilter

This file has two parts. Part 1 lists differences from DREAM.3D v6.5.171 (D1–D3). Part 2 lists SIMPLNX defects (D4–D5).

Entries use the stable ID sequence `ComputeFeatureSizes-D<N>`. The Filter UUID is the permanent cross-reference anchor. Source and release evidence are in `../provenance/ComputeFeatureSizesFilter.md`.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeFeatureSizes-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D1` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Status** | active |

**Symptom:** RectGrid feature volumes can differ from DREAM.3D v6.5.171 because the sums use different precision.

**Root cause:** Precision. DREAM.3D v6.5.171 `FindSizes::findSizesUnstructured()` adds float32 cell sizes to a float32 feature volume. SIMPLNX promotes each cell size to float64 and uses Kahan compensated summation. Direct execution uses thread-local sums and a compensated reduction. Its per-worker compensation resets at each `RectGridSummationImpl::convert()` call. Scanline execution keeps one compensation value per feature across input chunks. These mechanisms reduce accumulation error, but do not guarantee a strictly more accurate result for every input.

**Affected users:** Users who compare RectGrid feature volumes or derived diameters with DREAM.3D v6.5.171 output.

**Recommendation:** Trust the analytical volume definition and compare results with an appropriate numerical tolerance. SIMPLNX retains more precision during accumulation; `ComputeFeatureSizes-D2` also affects the diameter.

**Evidence:** Source comparison uses `Source/Plugins/Statistics/StatisticsFilters/FindSizes.cpp` at the DREAM.3D `v6.5.171` tag and the current `ComputeFeatureSizesDirect.cpp` and `ComputeFeatureSizesScanline.cpp`.

---

### ComputeFeatureSizes-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D2` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Status** | active |

**Symptom:** Equivalent diameters can differ from DREAM.3D v6.5.171 even when the stored feature sizes agree.

**Root cause:** Precision and library. DREAM.3D v6.5.171 reads the stored float32 size into the diameter calculation and uses `powf` or `sqrtf`. SIMPLNX uses the unrounded float64 feature size with `std::cbrt` or `std::sqrt`. For RectGrid, the denominator also changes from float32 `(4/3)*pi` to float64 `(4*pi)/3`. The same stored-size versus unrounded-size distinction applies to Image geometry.

**Affected users:** Users who compare equivalent spherical or circular diameters with DREAM.3D v6.5.171 output.

**Recommendation:** Trust the analytical diameter formula. Use a numerical tolerance when comparing implementations with different intermediate precision.

**Evidence:** The DREAM.3D `v6.5.171` source and the current Direct and Scanline sources use the evaluation order and types described above.

---

### ComputeFeatureSizes-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D3` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | Not fixed. |

**Symptom:** For a 2D image, the reported area includes the flat dimension's spacing, and the circular diameter uses that value.

**Root cause:** Bug in the area and diameter interpretation. PR #1590 (`e76ad508e`) replaced the two non-flat spacings with the product of all three spacings. Both current algorithms keep this slab convention and then apply `2*sqrt(value/pi)`. With physical length units, the product is a volume, so it is not an area for the circle formula. DREAM.3D v6.5.171 uses only the two non-flat spacings.

**Affected users:** Users who need true 2D area and circular diameter, especially when flat spacing is not 1. For four cells with non-flat spacings 2 and 3 and flat spacing 5, the algorithms calculate 120; the true area is 24. Relative to true area, the size scales by flat spacing and the diameter scales by its square root.

**Recommendation:** Do not interpret these outputs as true area and circular diameter when flat spacing is not 1. For true 2D measurements, use the product of the two non-flat spacings for area and `2*sqrt(area/pi)` for diameter.

**Evidence:** `SimplnxCore::ComputeFeatureSizes: 2D area excludes the flat-dimension spacing` asserts 24 for flat X, Y, and Z and has `[!shouldfail]`. The Scanline `bounded output writes` Image 2D variant instead asserts the current slab result. Source in `upstream/release/v7.0.0` through `upstream/release/v7.4.1` excludes the flat spacing. Source in `upstream/release/v7.4.2` includes all three spacings.

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeFeatureSizes-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D4` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1638 (`8d0b0ed22`), DREAM3D-NX v7.4.2. |

**Symptom:** RectGrid feature-volume reduction could lose the intended compensation for floating-point rounding.

**Root cause:** Bug. PR #1592 (`aaad2c4e2`) added the parallel reduction. It subtracted the compensation from the accumulated total, added that value to the incoming partial sum, and measured the rounding residual relative to the incoming sum. PR #1638 applies the usual Kahan update: subtract the compensation from the incoming partial sum, add it to the total, and calculate the residual relative to the total. The corrected code is in `ComputeFeatureSizesDirect.cpp`.

**Affected users:** Users of development revisions between PR #1592 and PR #1638 who calculate RectGrid feature volumes. DREAM3D-NX v7.0.0 through v7.4.2 were not affected.

**Recommendation:** Use a revision that contains PR #1638. The RectGrid analytical tests check feature volumes and diameters, but do not isolate the compensation correction with an adversarial rounding fixture.

---

### ComputeFeatureSizes-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D5` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Status** | active |
| **Affected released versions** | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1638 (`8d0b0ed22`), DREAM3D-NX v7.4.2. |

**Symptom:** Parallel Image and RectGrid workers could read nonresident FeatureIds without first disabling parallel scheduling.

**Root cause:** Bug. PR #1592 introduced parallel reads without a residency guard. PR #1638 added `requireStoresInMemory()` for FeatureIds before each `ParallelDataAlgorithm::execute()` call. The current Direct implementation retains both calls. The guard controls scheduling; it does not pin data or synchronize store access. The RectGrid guard checks FeatureIds only, not element sizes.

**Affected users:** Users of development revisions between PR #1592 and PR #1638 with nonresident FeatureIds and parallel execution. DREAM3D-NX v7.0.0 through v7.4.2 were not affected.

**Recommendation:** Use a revision that contains PR #1638. Current real-store tests require Scanline dispatch and do not directly exercise this Direct scheduling fallback.
