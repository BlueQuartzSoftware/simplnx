# Deviations from DREAM3D 6.5.171: ComputeFeatureSizes

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`ComputeFeatureSizes-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

---

## ImageGeom path — no deviations

Source-inspection comparison of `ProcessImageGeom` against DREAM3D 6.5.171 `FindSizes::execute()` confirmed identical algorithmic structure:

- Per-feature voxel count: identical parallel accumulation logic (serial in legacy).
- 3D volume formula: `volume = voxelCount × voxelVolume`; ESD: `2·∛(volume / (4π/3))` — both use the standard sphere formula.
- 2D area formula (any dim == 1): `area = voxelCount × voxelArea`; ECD: `2·√(area / π)` — both use the standard circle formula.

**Precision non-deviation (not flagged):** SIMPLNX promotes `voxelVolume` to `float64` and evaluates `cbrt` / `sqrt` in `float64` before casting to `float32` for storage. The legacy filter likely used `float32` throughout. The difference is within ~1 ULP of `float32` for typical EBSD spacings and is non-material for downstream morphological statistics.

---

## ComputeFeatureSizes-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D1` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Affected array** | `Volumes` (RectGridGeom) |
| **Status** | Active — precision improvement; SIMPLNX output is more accurate. **Verified by A/B (2026-06-27)** — see "A/B verification" below. |

**Symptom:** Per-feature volumes for features in `RectGridGeom` geometries differ from DREAM3D 6.5.171 `FindSizes::findSizesUnstructured()` output. The discrepancy grows with feature size and with the variation in cell volumes across the grid, scaling approximately as `O(N · ε_float32)` for the legacy result vs `O(ε_float64)` for the SIMPLNX result, where N is the number of cells in the feature.

**Root cause:** Precision — two compounding improvements in SIMPLNX's `ProcessRectGridGeom` over the legacy `findSizesUnstructured`:

1. **float32 → float64 promotion.** Element sizes are stored as `float32` (the output of `findElementSizes`). The legacy `findSizesUnstructured` accumulated these directly in `float32`, so each per-cell addition carried a relative error of ~`ε_float32 ≈ 1.2×10⁻⁷`. SIMPLNX promotes each element size to `float64` before accumulation (`static_cast<float64>(m_ElemSizes.getValue(voxelIdx))`), reducing the per-operation rounding error to `ε_float64 ≈ 2.2×10⁻¹⁶`.

2. **Kahan summation.** SIMPLNX applies Kahan compensated summation at two levels:
   - *Per-thread, per-cell* in `RectGridSummationImpl::convert()` (lines 274–283): standard Kahan with `y = elemSize - c`, `t = sum + y`, `c′ = (t - sum) - y`. For a feature spanning N cells, naive float64 accumulation still has O(N · ε_float64) error; Kahan reduces this to O(ε_float64) by carrying a compensation term `c` that recovers the low-order bits lost in each `sum + y` operation.
   - *Post-reduction, per thread-local vector* in `threadLocalVolumes.combine_each()` (lines 341–358): the same Kahan scheme is applied when combining the TBB thread-local partial sums into the final per-feature volume.

   For a non-uniform rectilinear grid where cell volumes span several orders of magnitude (e.g., a grid with fine near-surface resolution and coarse interior), naive summation of N cells can lose ~`log₂(V_max / V_min)` bits of precision in the smaller cell contributions. Kahan summation recovers those bits regardless of N or the dynamic range of cell volumes.

**Affected users:** Any workflow that runs `ComputeFeatureSizes` on a `RectGridGeom` and then compares per-feature volume or ESD values against DREAM3D 6.5.171 output. The deviation is largest for large features (high N) on grids with high volume variation. On uniform RectGrids (all cell volumes equal), Kahan has no practical effect and the only deviation is the float32→float64 promotion.

**Recommendation:** Trust SIMPLNX. The legacy `findSizesUnstructured` accumulated unnecessary floating-point error that grew with feature size and grid non-uniformity. The SIMPLNX result is strictly more accurate. Users migrating pipelines should expect small positive or negative shifts in per-feature volumes. The `EquivalentDiameters` array is affected both by this volume shift **and** by a separate ESD-evaluation deviation — see `ComputeFeatureSizes-D2`.

**A/B verification (2026-06-27):** A direct comparison was run, not just source inspection. The exact RectGrid unit-test fixture was authored as a shared legacy `.dream3d` and run through stock DREAM3D 6.5.171, DREAM3D-NX, and a 6.5.172 proof-patch build. Stock 6.5.171 `Volumes` differed from SIMPLNX (≈1 float32 ULP on this small fixture; grows with N). Applying **only** the float64 + Kahan summation change to legacy `findSizesUnstructured` made `Volumes` **bit-identical** to SIMPLNX, confirming summation precision as the sole root cause of the `Volumes` deviation.

---

## ComputeFeatureSizes-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureSizes-D2` |
| **Filter UUID** | `c666ee17-ca58-4969-80d0-819986c72485` |
| **Affected array** | `EquivalentDiameters` (RectGridGeom; same evaluation pattern on ImageGeom) |
| **Status** | Active — precision improvement; SIMPLNX output is more accurate. **Verified by A/B (2026-06-27).** |

**Symptom:** Per-feature `EquivalentDiameters` for `RectGridGeom` geometries differ from DREAM3D 6.5.171 output by ≈1 float32 ULP **even after the `Volumes` arrays are made identical**. This is a second precision source, independent of the summation in D1.

**Root cause:** The equivalent spherical diameter evaluation differs:
- **Legacy `findSizesUnstructured`:** `rad = m_Volumes[i] / vol_term; diameter = 2.0f * powf(rad, 0.3333333333f)` — the cube root is taken with `powf` (float32) on the **float32-rounded** stored volume, and `vol_term` is `(4/3)·k_Pif` (float32 π).
- **SIMPLNX `ProcessRectGridGeom`:** `2.0 * std::cbrt(featureVolumes[i] / k_ESDVolumeDenominator)` — `std::cbrt` (float64) on the **float64** accumulated volume, with `k_ESDVolumeDenominator = (4·π)/3` in float64.

So even with identical `Volumes`, the ESD differs because the legacy path rounds the volume to float32 first and uses a float32 `powf` cube root, while SIMPLNX evaluates the cube root in float64 on the unrounded sum.

**A/B verification (2026-06-27):** After making `Volumes` identical (D1 patch), `EquivalentDiameters` still differed by ≈1 ULP. Additionally changing legacy to compute the diameter as `2.0 * std::cbrt(volumeSums[i] / ((4.0·k_Pi)/3.0))` (double `std::cbrt` on the double sum) made `EquivalentDiameters` **bit-identical** to SIMPLNX. Both changes together — D1 (summation) and D2 (ESD evaluation) — are required for legacy to fully reproduce SIMPLNX output.

**Affected users:** Any workflow comparing `EquivalentDiameters` against DREAM3D 6.5.171. The same `powf`-on-float32 vs `cbrt`-on-float64 pattern exists on the ImageGeom path (`FindSizes::findSizesImage` uses `powf`/`sqrtf`); it was previously noted only as a non-flagged precision difference and is folded into this entry for the equivalent-diameter/ECD evaluation.

**Recommendation:** Trust SIMPLNX. The float64 `std::cbrt`/`std::sqrt` evaluation is more accurate than the legacy float32 `powf`/`sqrtf` on a pre-rounded value.

---

*If a future comparison run against DREAM3D 6.5.171 output reveals additional deviations, add them here as `ComputeFeatureSizes-D3` etc.*
