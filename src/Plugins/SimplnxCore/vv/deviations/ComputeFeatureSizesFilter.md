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
| **Status** | Active — precision improvement; SIMPLNX output is more accurate |

**Symptom:** Per-feature volumes for features in `RectGridGeom` geometries differ from DREAM3D 6.5.171 `FindSizes::findSizesUnstructured()` output. The discrepancy grows with feature size and with the variation in cell volumes across the grid, scaling approximately as `O(N · ε_float32)` for the legacy result vs `O(ε_float64)` for the SIMPLNX result, where N is the number of cells in the feature.

**Root cause:** Precision — two compounding improvements in SIMPLNX's `ProcessRectGridGeom` over the legacy `findSizesUnstructured`:

1. **float32 → float64 promotion.** Element sizes are stored as `float32` (the output of `findElementSizes`). The legacy `findSizesUnstructured` accumulated these directly in `float32`, so each per-cell addition carried a relative error of ~`ε_float32 ≈ 1.2×10⁻⁷`. SIMPLNX promotes each element size to `float64` before accumulation (`static_cast<float64>(m_ElemSizes.getValue(voxelIdx))`), reducing the per-operation rounding error to `ε_float64 ≈ 2.2×10⁻¹⁶`.

2. **Kahan summation.** SIMPLNX applies Kahan compensated summation at two levels:
   - *Per-thread, per-cell* in `RectGridSummationImpl::convert()` (lines 274–283): standard Kahan with `y = elemSize - c`, `t = sum + y`, `c′ = (t - sum) - y`. For a feature spanning N cells, naive float64 accumulation still has O(N · ε_float64) error; Kahan reduces this to O(ε_float64) by carrying a compensation term `c` that recovers the low-order bits lost in each `sum + y` operation.
   - *Post-reduction, per thread-local vector* in `threadLocalVolumes.combine_each()` (lines 341–358): the same Kahan scheme is applied when combining the TBB thread-local partial sums into the final per-feature volume.

   For a non-uniform rectilinear grid where cell volumes span several orders of magnitude (e.g., a grid with fine near-surface resolution and coarse interior), naive summation of N cells can lose ~`log₂(V_max / V_min)` bits of precision in the smaller cell contributions. Kahan summation recovers those bits regardless of N or the dynamic range of cell volumes.

**Affected users:** Any workflow that runs `ComputeFeatureSizes` on a `RectGridGeom` and then compares per-feature volume or ESD values against DREAM3D 6.5.171 output. The deviation is largest for large features (high N) on grids with high volume variation. On uniform RectGrids (all cell volumes equal), Kahan has no practical effect and the only deviation is the float32→float64 promotion.

**Recommendation:** Trust SIMPLNX. The legacy `findSizesUnstructured` accumulated unnecessary floating-point error that grew with feature size and grid non-uniformity. The SIMPLNX result is strictly more accurate. Users migrating pipelines should expect small positive or negative shifts in per-feature volumes; ESD values are affected proportionally through the `cbrt` formula.

---

*If a future comparison run against DREAM3D 6.5.171 output reveals additional deviations, add them here as `ComputeFeatureSizes-D2` etc.*
