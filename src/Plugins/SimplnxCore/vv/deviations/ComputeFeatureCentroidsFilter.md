# Deviations from DREAM3D 6.5.171: ComputeFeatureCentroidsFilter

This file lists every documented behavioral difference between the SIMPLNX `ComputeFeatureCentroidsFilter` and its DREAM3D 6.5.171 equivalent `FindFeatureCentroids` (SIMPL UUID `6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`, `Source/Plugins/Generic/GenericFilters/FindFeatureCentroids.{h,cpp}`).

Entries are referenced by stable ID (`ComputeFeatureCentroidsFilter-D<N>`) from the V&V report and public migration guidance. The Filter UUID field is the permanent cross-reference anchor.

**Algorithm Relationship:** *Port.* The non-periodic path is a near line-by-line translation of `FindFeatureCentroids::find_centroids()`, preserving the identical Kahan compensated summation and `double` accumulators. Comparison basis: **source inspection of both implementations** (6.5.171/6.5.172 `FindFeatureCentroids.cpp` vs SIMPLNX `Algorithms/ComputeFeatureCentroids.cpp`), backed by the Class 1 analytical fixtures in `test/ComputeFeatureCentroidsTest.cpp` that verify SIMPLNX against hand-derived values independently of either DREAM3D version. An empirical three-way binary A/B (6.5.171 PipelineRunner + 6.5.172 backport + nxrunner) can be run to bit-confirm D1 if the SBIR deliverable requires it; source analysis is conclusive for a verified port.

---

## ComputeFeatureCentroidsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureCentroidsFilter-D1` |
| **Filter UUID** | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| **Status** | active |

**Symptom:** Centroid components may differ from DREAM3D 6.5.171 in the lowest bit(s) of the `float32` result on features with large voxel counts and/or large coordinate magnitudes. On typical data the outputs are bit-identical.

**Root cause:** *Precision.* Both implementations accumulate voxel-center coordinates into `double` with the identical Kahan compensated-summation kernel and store the final centroid as `float32`. The only difference is how the per-voxel center is fetched before accumulation: SIMPLNX computes it directly in `float64` (`ImageGeom::getCoords` → `Point3Dd`), whereas 6.5.171 computes it into a `float[3]` and then promotes to `double` inside the Kahan step (`FindFeatureCentroids.cpp:161,175–199`). SIMPLNX therefore carries full double precision through the whole accumulation; legacy carries a `float32`-rounded coordinate into each add. After averaging and the final cast back to `float32`, the residual difference is at or below one `float32` ULP of the coordinate value.

**Affected users:** Anyone requiring bit-exact reproducibility of centroid values across the DREAM3D → DREAM3D-NX migration, on datasets with very large coordinate magnitudes or features spanning millions of voxels. Users who visualize or do downstream morphology at `float32` precision will not observe a difference.

**Recommendation:** *Trust SIMPLNX.* The `float64` voxel-center path is strictly more accurate; 6.5.171 is not more correct — it simply discards precision earlier. No legacy patch needed.

---

## ComputeFeatureCentroidsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeFeatureCentroidsFilter-D2` |
| **Filter UUID** | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| **Status** | active |

**Symptom:** SIMPLNX exposes an `Is Periodic` option that, when enabled, offsets the centroid of any feature spanning the full extent of an axis (wrap-aware centroid). DREAM3D 6.5.171 `FindFeatureCentroids` has no such option and always produces the literal (truncated) centroid.

**Root cause:** *Algorithmic choice (feature addition).* The `Is Periodic` parameter and the periodic centroid computation are SIMPLNX-only. Legacy has no periodic path, so there is nothing to diff against — this is an additive capability, not a divergence on shared behavior. The SIMPLNX default is `false`, which reproduces legacy exactly. SIMPL 6.4/6.5 pipelines convert with `Is Periodic` defaulted to `false`, so auto-converted legacy pipelines match 6.5.171.

**Affected users:** Only users who explicitly enable `Is Periodic`. Auto-converted legacy pipelines are unaffected (default `false`).

**Recommendation:** *Trust SIMPLNX.* Opt-in feature with a legacy-preserving default.

**Note (SIMPLNX-internal bugs fixed during V&V — not legacy deviations):** The periodic centroid had two SIMPLNX-internal defects, both fixed. (1) *Spacing:* the offset in the `ImageGeom` overload of `AdjustCentroidsForPeriodicFaces` was added in cell-index units (`(dim−1)/2`) to a physical-coordinate centroid, correct only when spacing == 1 (fixed during #1658). (2) *Model (issue #1665, 2026-07 follow-up):* the constant half-domain offset is itself only correct for a feature whose mass is symmetric about the seam, and can place asymmetric wrapped features outside the domain. The periodic path was reimplemented as a **minimum-image circular mean** accumulated during the cell sweep, and the range-based `ImageGeom` overload (insufficient inputs to compute a wrapped centroid) was removed. `test/ComputeFeatureCentroidsTest.cpp` Fixtures D/E (independent minimum-image oracle) and F (domain-filling fallback) pin the corrected behavior. Because 6.5.171 has no periodic path, neither fix is a change relative to legacy; recorded here for any SIMPLNX user who ran `Is Periodic` before the fixes.

---

## Retracted candidate deviations

- **~~D: Kahan-vs-naive summation precision~~** — The retroactive DRAFT report (`docs/vv_retroactive_reports/ComputeFeatureCentroidsFilter.md`) hypothesized that SIMPLNX added Kahan summation over a legacy *naive* running sum. **Retracted:** source inspection shows 6.5.171 `FindFeatureCentroids::find_centroids()` already uses the identical Kahan kernel with identical `double` accumulators. No such deviation exists; the residual precision difference is captured by D1 (voxel-center fetch width), not summation method.
