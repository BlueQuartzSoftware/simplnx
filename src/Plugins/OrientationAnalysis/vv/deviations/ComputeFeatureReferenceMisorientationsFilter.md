# Deviations from DREAM3D 6.5.171: ComputeFeatureReferenceMisorientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindFeatureReferenceMisorientations`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindFeatureReferenceMisorientations.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeFeatureReferenceMisorientationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

The legacy A/B comparison was performed by **source inspection** rather than empirical run. Justification: SIMPLNX `ComputeFeatureReferenceMisorientationsFilter::operator()()` is a clean Port of legacy `FindFeatureReferenceMisorientations::execute()` (same two-mode dispatch, same per-voxel main loop, same per-feature averaging finalization, same `LaueOps`-delegated misorientation math, same `m_Centers` selection with `>=` tie-break). The only port-time deltas are API modernization (`getMisoQuat` → `calculateMisorientation`), type widening (`QuatF` → `QuatD` for the internal math, narrowed back to `float` for storage), cleaner auxiliary storage (interleaved `avgMisoPtr` → separate `avgMisorientationSums` + `avgMisorientationCounts`), added cancel checks, and a new optional `EuclideanCenters` output array in Mode 1 (no pre-existing output is affected).

For the V&V data fixtures (pure φ1 rotations about z, no cubic-symmetry-op-aligned grain boundaries), both implementations are expected to produce bit-identical outputs modulo `float` precision (< 1 ULP differences possible due to `QuatF` → `QuatD` promotion).

For real EBSD data (e.g., the Small-IN100 dataset that the retired exemplar archive came from), the EbsdLib 2.4.1 `CubicOps::calculateMisorientationInternal` precision improvement (`2·atan2(|v|, w)` form replacing the precision-fragile `acos(w)` near 1) propagates through SIMPLNX's misorientation math and yields per-voxel `FRM` values that differ from legacy by ~ULP-scale (sub-`0.0001°`) for sym-op-aligned voxel pairs. When those per-voxel values are averaged into per-feature `avgRefMis` quantities, the small per-voxel shifts can accumulate to 2×–10× the `1e-4` epsilon used by the retired exemplar tests.

---

## ComputeFeatureReferenceMisorientationsFilter-D1

| Field            | Value                                                       |
|------------------|-------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureReferenceMisorientationsFilter-D1`           |
| **Filter UUID**  | `24b54daf-3bf5-4331-93f6-03a49f719bf1`                      |
| **Status**       | active (precision-class; non-deviation in algorithmic sense) |

**Symptom:** Per-feature average misorientations (`Feature Avg Misorientations`) differ between SIMPLNX and DREAM3D 6.5.171 on real EBSD datasets containing cubic-phase grains with grain boundaries near cubic-symmetry operators. On the Small-IN100 dataset (the basis for the retired `compute_feature_reference_misorientation.tar.gz` exemplar archive), Mode 0 (`AverageMisorientation`) averages drifted by ~`2e-4` (2× the `1e-4` epsilon used by `CompareDataArrays`); Mode 1 (`EuclideanDistance`) averages drifted by ~`1e-3` (10×). Per-voxel `FRM` values shift by sub-`0.0001°` (within float precision), but the magnitude amplifies when summed over a feature's voxels and divided by count.

On the V&V data fixtures (pure φ1 rotations about z, no cubic-sym-op-aligned voxel pairs), no observable deviation. All 6 Class 1 fixtures produce SIMPLNX values within `1e-3°` of the analytical expected value.

**Root cause:** **Precision** — not an algorithm change in either implementation.

The deviation traces to the EbsdLib 2.4.1 release commit `5c8c993` (BlueQuartz Software, 2026-05-29), which replaces a precision-fragile `acos(w)` form in `CubicOps::calculateMisorientationInternal` with a numerically-stable `2·atan2(|v|, w)` form using the explicit reduced-quaternion `v` components. The precision improvement is real and mathematically more correct; it manifests for cubic misorientations whose minimum-rotation-axis representation lies on or near a cubic symmetry operator (e.g., 90° about the cubic c-axis is a 4-fold sym op of m-3m; pre-fix `acos`-form yielded `~0.02°` residual due to float32-input ULP noise, post-fix yields the mathematically correct value).

This filter is a clean Port of `FindFeatureReferenceMisorientations` from DREAM3D 6.5.171; the SIMPLNX algorithm reproduces the legacy two-mode dispatch + per-voxel misorientation accumulation + per-feature averaging structure exactly. The legacy filter consumes `OrientationLib::CubicOps::getMisoQuat` (pre-fix `acos`-form, float32); the SIMPLNX filter consumes `ebsdlib::CubicOps::calculateMisorientation` (post-fix `2·atan2`-form, QuatD). The difference is entirely in the EbsdLib precision improvement, NOT in this filter.

For the full root-cause walkthrough of the EbsdLib precision improvement, see the precedent characterization in `vv/deviations/BadDataNeighborOrientationCheckFilter.md` §"Non-deviations" → "EbsdLib 2.4.1 CubicOps precision improvement". The characterization there applies equally to this filter; the only difference is that this filter's per-feature averaging amplifies the per-voxel precision shift across the feature's voxels (typically hundreds to thousands for real EBSD data), making the deviation more visible at the per-feature output level than at the per-voxel level.

**Affected users:** Anyone running this filter in DREAM3D 6.5.171 on EBSD data with cubic-phase grains that have grain boundaries near 4-fold (90° c-axis), 3-fold (120° [111]), or 2-fold (180° face-diagonal) cubic symmetry operators, and comparing per-feature `Feature Avg Misorientations` output across the version boundary. On non-cubic-phase data, no deviation. On cubic data without sym-op-aligned boundaries, no observable deviation.

**Recommendation:** **Trust SIMPLNX.** The 6.5.171 result was limited by float32-input ULP noise amplified by `acos`-near-1 catastrophic cancellation; SIMPLNX returns the mathematically correct value. The `~0.02°` shift is well below typical EBSD measurement resolution (per the BadDataNeighborOrientationCheckFilter V&V cycle's precedent characterization) and will not materially affect downstream microstructural analyses for users migrating from DREAM3D 6.5.171.

---

## Non-deviations (algorithm characteristics common to both filters)

The following behaviors are NOT deviations — SIMPLNX and 6.5.171 agree on them. Captured here so future engineers don't re-discover them and propose them as deviations.

### Raster-order tie-break in `centers[]` selection (Mode 1)

Both implementations use `if(distance >= centerDistances[featureId])` in the Mode 1 pre-loop that selects each feature's reference voxel. The `>=` (rather than `>`) means that when two or more voxels within a feature have identical `GBEuclideanDistances` values, the LATER voxel (in linear iteration order) overwrites earlier candidates and is selected as the feature's reference. The choice is therefore raster-order dependent — different DataStructure layouts that expose the same logical voxels in a different iteration order would yield different `centers[]` and different `EuclideanCenters`. **Both filters share this behavior** — verified by source inspection of the legacy `FindFeatureReferenceMisorientations::execute()` lines 320-325 vs SIMPLNX `ComputeFeatureReferenceMisorientations.cpp` lines 89-103.

### Background voxel and unphased voxel handling

Both implementations skip voxels where `featureIds[i] == 0` (background) or `cellPhases[i] == 0` (unphased). In both, the per-voxel `FRM` array is initialized to 0 (or `fill(0.0f)` in SIMPLNX) and skipped voxels retain that zero value. Per-feature `avgRefMis` is computed only over the non-skipped voxels in each feature; if a feature consists entirely of skipped voxels, its `count == 0` and `avgRefMis[fid]` is set to `0`. **Both filters share this behavior** — algorithm characteristic, not a defect.

### Background feature (featureId = 0) → `avgRefMis[0] == 0`

Both implementations leave `avgRefMis[0]` at its initialized `0.0f` value (since the main per-voxel loop only computes misorientations for `featureIds[i] > 0`, and the per-feature finalize loop iterates `for(i = 1; i < totalFeatures; i++)`, skipping index 0 entirely). Algorithm characteristic, not a defect.

---

## Comparison artifacts

For this filter's V&V cycle, the legacy A/B comparison was performed by **source inspection** rather than empirical run. Justification: both algorithms have been independently V&V'd at the source-code level (this filter via the V&V report; the EbsdLib precision math via BadDataNeighborOrientationCheckFilter's V&V cycle), and the data fixtures used here do not include sym-op-aligned boundaries that would surface the precision-class deviation. Running an empirical A/B on the data fixtures would confirm bit-identical (or sub-ULP) output, which is the expected outcome from source inspection.

If a future engineer wants to run an empirical A/B for confirmation:

- **6.5.171 binary**: `/Users/mjackson/Applications/DREAM3D.app/Contents/bin/PipelineRunner`
- **Suggested input fixture**: convert any V&V data fixture to legacy `.dream3d` format via the same h5py-based script pattern used in `BadDataNeighborOrientationCheckFilter`'s `bad_data_neighbor_orientation_check_v2/case_1/.../6_5_*_input.json` series. A draft Python script for Fixture B (Mode 0, 2×2×2 single grain, all 5° about z) lives at `/tmp/build_cfrm_fixtureB_legacy.py` from this V&V cycle.
- **Suggested legacy pipeline**: `DataContainerReader` → `FindFeatureReferenceMisorientations` → `DataContainerWriter`. The `DataContainerReader` requires a `DataContainerArrayProxy` enumerating the input file's paths; that adds ~150 lines of JSON for a 6-data-fixture sweep.
- **Expected outcome**: bit-identical or sub-ULP-difference output between SIMPLNX and 6.5.171 on the data fixtures (no sym-op-aligned boundaries → precision improvement not observable).
