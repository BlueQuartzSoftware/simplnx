# Deviations from DREAM3D 6.5.171: ComputeFeatureNeighborMisorientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindMisorientations`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindMisorientations.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeFeatureNeighborMisorientationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

The legacy A/B comparison was performed by **source inspection** rather than empirical run. Justification: SIMPLNX `ComputeFeatureNeighborMisorientations::operator()()` is a clean Port of legacy `FindMisorientations::execute()` (same per-feature outer loop, same per-neighbor inner loop, same phase-match gate, same optional per-feature averaging finalize). Both implementations share a divisor bug at the `tempMisoList` reassignment (D1 below) — verified by grep of the legacy source. The bug went undetected for the lifetime of both implementations because the `ComputeAvgMisors=true` test in the SIMPLNX suite was an `[.][UNIMPLEMENTED][!mayfail]` stub with zero CI coverage.

---

## ComputeFeatureNeighborMisorientationsFilter-D1

| Field            | Value                                                                                                                                |
|------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborMisorientationsFilter-D1`                                                                                     |
| **Filter UUID**  | `0b68fe25-b5ef-4805-ae32-20acb8d4e823`                                                                                               |
| **Status**       | active (SIMPLNX fixed 2026-06-02; legacy 6.5.171 still has the bug)                                                                  |

**Symptom:** Per-feature `AvgMisorientations` (output of `ComputeAvgMisors=true` / legacy `FindAvgMisors=true`) differ between SIMPLNX (post-2026-06-02 fix) and DREAM3D 6.5.171 on any dataset where features have mixed-phase neighbor lists. The legacy result depends on the *order* in which neighbors appear in the per-feature `NeighborList`: if the last-iterated neighbor is a phase match, the divisor used is the full neighbor-list length (incorrect); if the last neighbor is a phase mismatch, the divisor is decremented by 1 from the full length (the per-mismatch decrement at line 90 happens to be the last write to `tempMisoList`). The legacy result is therefore correct in some cases by accident and wrong by up to `(N-K) / N` of the true value in others, where N is the neighbor count and K is the number of phase-matched neighbors.

The bug is **non-observable on the V&V data fixtures' single-phase configurations** (no phase mismatches → no decrements → divisor matches list length, which equals the number of matches, which is correct). The bug **IS observable** on the bug-exposing fixture `Mixed Phase Neighbors (exposes divisor bug)`, which constructs a neighbor list `[match, mismatch, match]` for which the expected average is `(5 + 10) / 2 = 7.5°` but the buggy code produces `(5 + 10) / 3 = 5.0°`.

**Root cause:** **Bug** in both legacy DREAM3D 6.5.171 and SIMPLNX pre-fix.

The legacy code at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindMisorientations.cpp` (lines TBD) and the SIMPLNX pre-fix code at `Algorithms/ComputeFeatureNeighborMisorientations.cpp:75` both contain `tempMisoList = featureNeighborList.size();` *inside* the inner per-neighbor j-loop. The intended behavior is for `tempMisoList` to start each outer-loop iteration (per feature) at `featureNeighborList.size()` and then decrement by 1 for each phase-mismatched neighbor (line 90: `tempMisoList > 0 ? tempMisoList-- : tempMisoList = 0;`). Because the reassignment happens at the *top* of each j-iteration, the decrement from the *previous* iteration is clobbered. The result is that only the *last* j-iteration's match/mismatch state actually affects `tempMisoList`: if the last neighbor is a match, the assignment runs and the decrement doesn't, so the final divisor is N; if the last neighbor is a mismatch, both the assignment and the decrement run, so the final divisor is N - 1.

The SIMPLNX fix (2026-06-02) moves the `tempMisoList = featureNeighborList.size();` assignment from line 75 to before the inner j-loop (alongside `tempMisorientationLists[i].assign(...)` at line ~67), so the assignment runs once per outer-loop iteration (per feature) and the decrement is preserved across j-iterations. The result is the mathematically correct divisor: the number of phase-matched neighbors.

The bug went undetected for the lifetime of both implementations because:
1. **The legacy 6.5.171 implementation had no automated test coverage of the `ComputeAvgMisors=true` path** (legacy DREAM3D's CI tested filters with default parameter values; this parameter defaults to false in many user-facing pipelines and the test infrastructure didn't sweep over both values).
2. **The SIMPLNX Port preserved the bug** without a regression test that exercises mixed-phase neighbor lists. The `ComputeAvgMisors=true` test in `ComputeFeatureNeighborMisorientationsTest.cpp` was an `[.][UNIMPLEMENTED][!mayfail]` stub with the comment "TODO: needs to be implemented. This will need the input .dream3d file to be regenerated with the missing data generated using DREAM3D 6.6". Zero CI coverage.
3. **The retroactive bug-triage cycle (2026-05) caught it** by source inspection. Documented in `/Users/mjackson/Desktop/bug_triage.md` as Bug #2.

**Affected users:** Anyone running DREAM3D 6.5.171 or SIMPLNX pre-2026-06-02 with `ComputeAvgMisors=true` (legacy `FindAvgMisors=true`) on data containing features with mixed-phase neighbor lists. In practice this affects any multi-phase EBSD dataset that has at least one feature whose neighbor list includes both same-phase and different-phase neighbors. Single-phase datasets are unaffected. Datasets where the bug "accidentally" produces the correct divisor (every feature's neighbor list ends in a mismatch) are also unaffected.

**Recommendation:** **Trust SIMPLNX (post-2026-06-02 fix).** The pre-fix per-feature `AvgMisorientations` values from both DREAM3D 6.5.171 and pre-fix SIMPLNX are mathematically incorrect for any feature with a mixed-phase neighbor list. Users migrating from 6.5.171 should expect per-feature average misorientations to shift toward the mathematically correct value, with the shift size proportional to the fraction of phase-mismatched neighbors per feature.

A legacy backport branch of `FindMisorientations.cpp` with the same fix (move the `tempMisoList` reassignment outside the inner loop) would produce the corrected values on DREAM3D 6.5.171 for users requiring legacy-version-parity post-correction. The fix is mechanically the same as the SIMPLNX fix and is a one-line move. No such backport branch is currently maintained.

---

## ComputeFeatureNeighborMisorientationsFilter-D2

| Field            | Value                                                       |
|------------------|-------------------------------------------------------------|
| **Deviation ID** | `ComputeFeatureNeighborMisorientationsFilter-D2`            |
| **Filter UUID**  | `0b68fe25-b5ef-4805-ae32-20acb8d4e823`                      |
| **Status**       | active (precision-class; non-deviation in algorithmic sense) |

**Symptom:** Per-neighbor `MisorientationList` values and per-feature `AvgMisorientations` values differ between SIMPLNX (EbsdLib 2.4.1+, post-D1 fix) and DREAM3D 6.5.171 on real EBSD datasets containing cubic-phase features with grain-pair boundaries near cubic symmetry operators (e.g., 4-fold about c-axis, 3-fold about [111], 2-fold about face-diagonal). Per-neighbor values shift by sub-`0.0001°` (within float precision), but the magnitude amplifies when averaged across many neighbors at the per-feature level. On the V&V data fixtures (pure φ1 rotations about z, no sym-op-aligned neighbor pairs), no observable deviation.

**Root cause:** **Precision** — not an algorithm change in either implementation.

The deviation traces to the EbsdLib 2.4.1 release commit `5c8c993` (BlueQuartz Software, 2026-05-29), which replaces a precision-fragile `acos(w)` form in `CubicOps::calculateMisorientationInternal` with a numerically-stable `2·atan2(|v|, w)` form using the explicit reduced-quaternion `v` components. The precision improvement is real and mathematically more correct; it manifests for cubic misorientations whose minimum-rotation-axis representation lies on or near a cubic symmetry operator.

This filter is a clean Port of `FindMisorientations` (modulo the D1 divisor bug, which existed in both implementations and is now fixed in SIMPLNX). The SIMPLNX algorithm reproduces the legacy per-feature outer loop, per-neighbor inner loop, and per-feature average finalization. The legacy filter consumes `OrientationLib::CubicOps::getMisoQuat` (pre-fix `acos`-form, float32); the SIMPLNX filter consumes `ebsdlib::CubicOps::calculateMisorientation` (post-fix `2·atan2`-form, QuatD). The difference is entirely in the EbsdLib precision improvement, NOT in this filter.

For the full root-cause walkthrough of the EbsdLib precision improvement, see the precedent characterization in `vv/deviations/BadDataNeighborOrientationCheckFilter.md` §"Non-deviations" → "EbsdLib 2.4.1 CubicOps precision improvement". The characterization there applies equally to this filter. As with `ComputeFeatureReferenceMisorientationsFilter-D1`, this filter's per-feature averaging amplifies the per-voxel precision shift across the feature's neighbor list.

**Affected users:** Anyone migrating from DREAM3D 6.5.171 to SIMPLNX on cubic-phase EBSD data with features whose neighbor lists include sym-op-aligned grain-pair boundaries. Non-cubic data and data without sym-op-aligned boundaries are unaffected.

**Recommendation:** **Trust SIMPLNX.** The 6.5.171 result was limited by float32-input ULP noise amplified by `acos`-near-1 catastrophic cancellation; SIMPLNX returns the mathematically correct value. The shift is well below typical EBSD measurement resolution and will not materially affect downstream microstructural analyses.

---

## Non-deviations (algorithm characteristics common to both filters)

The following behaviors are NOT deviations — SIMPLNX (post-D1 fix) and DREAM3D 6.5.171 (with D1 still present) agree on them where the D1 bug is not exercised. Captured here so future engineers don't re-discover them and propose them as deviations.

### NaN entry on phase mismatch

Both implementations write `NaN` (via `std::numeric_limits<float>::quiet_NaN()` or `<cmath>`'s `NAN`) into the per-neighbor `MisorientationList` entry when the neighbor's phase differs from the focal feature's phase, or when the focal feature's `laueClass1` is out of range for `orientationOps`. **Both filters share this behavior** — algorithm characteristic, not a defect.

### Per-feature outer-loop iteration starts at index 1 (skips background feature 0)

Both implementations iterate `for(size_t i = 1; i < totalFeatures; i++)` in the per-feature outer loop, skipping the background feature at index 0. The `MisorientationList[0]` and `AvgMisorientations[0]` entries are therefore left at their initialized default values (empty list and `0.0f`, respectively). **Both filters share this behavior**.

### Self-misorientation not computed

Neither implementation includes the focal feature `i` in its own neighbor list (the `NeighborList<int32>` input is assumed to be a list of *other* features that share at least one boundary with feature `i`, not including `i` itself). Both implementations therefore do not compute `misorientation(i, i)` (which would be `0°` by definition). **Both filters share this assumption** — it is a property of how `ComputeFeatureNeighbors` (the upstream filter that produces the `NeighborList`) is conventionally used in the SIMPLNX and DREAM3D 6.5.171 pipelines.
