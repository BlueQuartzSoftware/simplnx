# Deviations from DREAM3D 6.5.171: ComputeKernelAvgMisorientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindKernelAvgMisorientations`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindKernelAvgMisorientations.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeKernelAvgMisorientationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

The legacy A/B comparison was performed by **source inspection** rather than empirical run. Justification: SIMPLNX `ComputeKernelAvgMisorientations` is a clean Port of legacy `FindKernelAvgMisorientations::execute()` (same per-voxel outer triple loop; same per-kernel inner triple loop; same focal-validity gate; same same-feature gate inside the kernel; same per-voxel average with the focal voxel always included in the divisor). The port-time deltas are documented in the V&V report's Algorithm Relationship section. Two deviations were identified: a precision-class non-deviation (D1) traceable to the EbsdLib 2.4.1 release, and a legacy bug (D2) at the inner x-loop bound that was corrected at port time.

---

## ComputeKernelAvgMisorientationsFilter-D1

| Field            | Value                                                                            |
|------------------|----------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D1`                                       |
| **Filter UUID**  | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035`                                           |
| **Status**       | active (precision-class; non-deviation in algorithmic sense)                     |

**Symptom:** Per-cell `KernelAverageMisorientations` values differ between SIMPLNX built against fixed EbsdLib (≥ v2.4.1, commit `5c8c993`) and DREAM3D 6.5.171 (and, equivalently, between fixed-EbsdLib and pre-fix-EbsdLib SIMPLNX builds — see the dependency note below). The shift is **quaternion-specific, not a uniform per-cell offset**: it is *exactly* 0 for focal cells whose symmetry-reduced self-misorientation lands on the trivial `wmin` candidate (the identity quaternion, and small rotations about a high-symmetry axis), and ~`0.03°` only for focal cells whose self-misorientation is reduced through a non-trivial cubic sym-op candidate (4-fold / 3-fold / 2-fold) that lands at `1 − ε`. Across a real dataset (Small_IN100 and similar) this averages to a per-cell shift of ~`0.005–0.05°`, depending on the focal-cell orientation distribution and the kernel size; it amplifies for asymmetric (e.g. `{2,2,1}`) or single-voxel kernels because the focal-cell self-misorientation term then carries more weight in the average.

**Dependency (resolved in this PR):** the fix lives in EbsdLib commit `5c8c993`, contained in the `v2.4.1` tag. As of this PR the SIMPLNX `vcpkg.json` pins `ebsdlib version>=2.4.1`, so the **standard vcpkg build now links the fixed EbsdLib** and the artifact no longer appears in any supported configuration: both the standard build (`NX-Com-Qt69-Vtk95-Rel`) and the local-source build (`NX-Com-Qt69-Vtk95-Rel-EbsdLib`, `SIMPLNX_USE_LOCAL_EBSD_LIB=ON`) produce the correct, self-miso-free result. The V&V toy-fixture unit tests assert the exact analytical oracle (margin `1e-3`) and pass in both configurations. The artifact reappears only if EbsdLib is pinned below `2.4.1` (e.g. an older vcpkg baseline); the *Empirical confirmation* below was captured against the pre-fix `2.4.0` — the version that shipped before this PR — to characterize the symptom and verify the fix.

**Root cause:** **Precision** — not an algorithm change in either implementation.

The deviation traces to the EbsdLib 2.4.1 release commit `5c8c993` (BlueQuartz Software, 2026-05-29), which replaces a precision-fragile `acos(w)` form in `CubicOps::calculateMisorientationInternal` with a numerically-stable `2·atan2(|v|, w)` form using the explicit reduced-quaternion `v` components. The precision improvement is real and mathematically more correct; for `ComputeKernelAvgMisorientationsFilter` specifically it manifests *more strongly than for the per-pair misorientation filters in this cycle* because the kernel inclusion of the focal voxel triggers a per-cell self-misorientation call. For the pre-fix `acos(w)`-form:

- `q_self_miso = q_focal * q_focal.conjugate() = (0, 0, 0, 1)` mathematically (identity quaternion), so the *true* self-misorientation is exactly 0°.
- The error is **not** introduced by the raw `q * q.conjugate()` product (its `w` component is `|q|² ≈ 1`); it is introduced by the **symmetry reduction**. `calculateMisorientationInternal` maximizes `wmin` over the 24 cubic sym-op candidates, evaluating three candidate forms per sym op: `qco.w()`, `(qco.z() + qco.w())/√2` (the 4-fold-about-c form), and `(qco.x()+qco.y()+qco.z()+qco.w())/2`. For the identity misorientation, several candidates equal 1.0 mathematically — but on float32-sourced quaternions a non-trivial candidate such as `(qco.z() + qco.w())/√2` evaluates to `1 − ε` (with `ε ≈ 1.7e-8`) and can be selected as the maximum. **Which focal quaternions trigger this is candidate-dependent: most reduce on the trivial `qco.w() == 1.0` branch and yield exactly 0; only those whose maximizing candidate is a non-trivial sym-op form land at `1 − ε`.**
- `acos(1 − ε)` near 1 is precision-fragile: the derivative of `acos` at 1 is `-1/√(1-x²) → -∞`, so a `1-ULP` error in `wmin` propagates to a `√(2ε)`-scale error in the angle (then doubled by the `2 * acos(wmin)` step). For `ε` of order `1e-8` this puts the spurious self-miso in the `~0.02–0.03°` range — the fix commit message cites `~0.02°`; the value **measured empirically on this branch is `0.0326°`** (see below). The exact constant depends on the winning sym-op candidate and the platform's float32 quantization, so treat the magnitude as order-of-`0.03°`, not a fixed number.
- The post-fix `2 * atan2(|v|, w)` form, using the **explicit** reduced-quaternion vector components, is numerically stable: components like `(qco.z() - qco.w())` evaluate to *exactly* 0 in IEEE-754 when `qco.z() == qco.w()` regardless of upstream float32 truncation, so `|v| = 0` and the result is exactly 0 for every identity self-misorientation.

The KAM filter is *more sensitive* than `ComputeFeatureNeighborMisorientations` and `BadDataNeighborOrientationCheck` to this precision improvement because:

1. **Self-misorientation contribution.** The KAM kernel includes the focal cell (via the `j=k=l=0` inner iteration). For each focal cell, the algorithm therefore makes one call to `calculateMisorientation` with `q1 == q2`. With pre-fix EbsdLib this call returns a spurious ~0.03° **for the subset of focal orientations whose symmetry reduction lands on a non-trivial sym-op candidate** (exactly 0 for the rest), which gets added to `totalMisorientation` and shifts that cell's average up by `(spurious_self / numVoxel)`. With fixed EbsdLib the call returns 0° for *every* focal orientation and contributes nothing. This is the *entire* KAM-specific deviation — see point 3.

2. **Same-feature large-N averaging.** For a cell in the middle of a large grain with kernel `{1,1,1}`, numVoxel = 27 (all same-feature). The cumulative effect of 27 small precision noises averages out somewhat, but the systematic self-miso contribution is always present.

3. **The deviation is the self-miso term, essentially nothing else.** For *distinct*-orientation pairs the two EbsdLib forms agree to well below `0.0001°` — empirically confirmed: the sibling `ComputeFeatureNeighborMisorientations` toy fixtures assert distinct-pair misorientations of `5.0°` and `10.0°` at margin `1e-3` and **pass against both vcpkg `2.4.0` and the fixed EbsdLib**. That filter excludes the focal feature from its neighbor list, so it never makes a `q1 == q2` call and shows no shift. KAM's per-cell shift is therefore attributable *entirely* to the focal-cell self-misorientation term, not to any per-pair precision noise — which is why KAM is the most observable filter in this cycle for the EbsdLib precision fix.

**Affected users:** Anyone migrating from DREAM3D 6.5.171 to SIMPLNX on cubic-phase EBSD data with this filter, *or* anyone running a SIMPLNX build pinned to EbsdLib `< 2.4.1` (the standard vcpkg build now pins `≥ 2.4.1`, so this affects only builds on an older baseline). The shift is per-cell, systematic in sign (always slightly above the true KAM), and proportional to the inverse of the kernel volume (1 / numVoxel) — but only for focal cells whose orientation triggers the artifact (see *Symptom*); unaffected focal cells shift by 0. The figures below are the **upper bound for an affected focal cell**: for `KernelSize = {1,1,1}` on a grain interior the affected-cell shift is `~0.03°/27 ≈ 0.001°`; for `KernelSize = {0,0,0}` (single-voxel kernel — just the focal cell) it is the full `~0.03°` because the divisor is 1.

**Recommendation:** **Trust SIMPLNX (EbsdLib 2.4.1+).** The 6.5.171 result was limited by the well-understood `acos(w near 1)` precision pathology amplified by float32-sourced quaternion inputs; SIMPLNX returns the mathematically correct value. The shift is well below typical EBSD measurement resolution and will not materially affect downstream microstructural analyses, but the cumulative effect on KAM-based maps will be visibly smoother in the post-2.4.1 output. Users requiring exact 6.5.171 reproduction can compile against EbsdLib < 2.4.1 (not recommended).

For the full root-cause walkthrough of the EbsdLib precision improvement, see the precedent characterization in `vv/deviations/BadDataNeighborOrientationCheckFilter.md` §"Non-deviations" → "EbsdLib 2.4.1 CubicOps precision improvement". The characterization there applies equally to this filter, with the additional amplification factor described above.

**Empirical confirmation (V&V cycle, branch `topic/vv/ComputeFeatureNeighborMisorientationsFilter`, 2026-06-04):** The Class 1 / Class 4 toy fixtures were run on Apple Silicon against both EbsdLib builds, with per-pair `calculateMisorientation` results instrumented:

- **Distinct-orientation pairs are exact on both builds.** Pairs of `5°`, `10°`, and `15°` apart returned `4.99991°`, `4.99988°`, etc. (`< 0.0002°` from the analytical value) on both vcpkg `2.4.0` and the fixed local EbsdLib. This rules out per-pair precision noise as a contributor and confirms point 3 above.
- **Self-misorientations are 0 for most focal orientations even pre-fix.** `q1 == q2` for the identity and for the `5°`, `10°`, `15°`-about-c focal cells returned *exactly* `0.0°` on vcpkg `2.4.0`.
- **Only the `20°`-about-c focal cells triggered the artifact pre-fix.** Their self-misorientation returned `0.0325663°` on vcpkg `2.4.0` (matching the `~0.033°` derived above), inflating those cells' KAM by `0.0326°/numVoxel`. Example: the 1D x-axis gradient fixture's last cell (`numVoxel = 2`) read `2.51628°` against an analytical `2.5°` — exceeding the test's `1e-3` margin.
- **The fixed EbsdLib zeroes every self-misorientation.** Rebuilding the same fixtures with the `NX-Com-Qt69-Vtk95-Rel-EbsdLib` preset (local EbsdLib at `5c8c993`), all three misorientation suites pass exactly: KAM `134/134` assertions, `ComputeFeatureNeighborMisorientations` `56/56`, `ComputeFeatureReferenceMisorientations` `238/238`.

The toy-fixture unit tests assert the analytical oracle directly (margin `1e-3`, no tolerance for the pre-fix artifact). With EbsdLib pinned `≥ 2.4.1` in `vcpkg.json` this is the correct, regression-sensitive choice: it holds in every supported build and would immediately flag any future regression of the EbsdLib precision fix, rather than silently absorbing it under a loose tolerance.

---

## ComputeKernelAvgMisorientationsFilter-D2

| Field            | Value                                                                              |
|------------------|------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D2`                                         |
| **Filter UUID**  | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035`                                             |
| **Status**       | active (SIMPLNX correct since port; legacy 6.5.171 still has the bug)              |

**Symptom:** Per-cell `KernelAverageMisorientations` values differ between SIMPLNX and DREAM3D 6.5.171 whenever the user-supplied `KernelSize` has `KernelSize.x != KernelSize.z`. For symmetric kernels (`{1,1,1}`, `{2,2,2}`, etc. — the default and the most common use), the deviation is **dormant**. For asymmetric kernels (e.g., `{1, 1, 2}` — common when the user is processing serial-section data with non-isotropic voxel spacing), the legacy code iterates the x-direction inner loop with the WRONG bound, producing a kernel of incorrect shape and an incorrect KAM.

Concrete example: with `KernelSize = {1, 1, 2}` on a `30x30x30` voxel grid, legacy `FindKernelAvgMisorientations` iterates the inner-most `l` loop from `l = -1` to `l = 2` (5 iterations: `-1, 0, 1, 2`) instead of the correct `l = -1` to `l = 1` (3 iterations). For each focal cell, legacy adds the cells at `x+2` (out of the user's intended kernel) to the average while still excluding cells at `x = focal - 2`. The kernel becomes asymmetric in a way the user did not request.

**Root cause:** **Bug** in legacy DREAM3D 6.5.171 only.

The legacy code at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindKernelAvgMisorientations.cpp:264` is:

```cpp
for(int32_t l = -m_KernelSize.x; l < m_KernelSize.z + 1; l++)
//                                            ^ should be .x
```

The two surrounding outer loops use the correct axis: `m_KernelSize.z` for `j` (line 258) and `m_KernelSize.y` for `k` (line 261). Line 264 is a copy-paste typo where the upper bound `m_KernelSize.z + 1` was carried over from the z-loop instead of being changed to `m_KernelSize.x + 1`.

The SIMPLNX algorithm at `Algorithms/ComputeKernelAvgMisorientations.cpp:108` is correct:

```cpp
for(int32_t l = -kernelSize[0]; l < kernelSize[0] + 1; l++)
```

where `kernelSize[0]` is X. The port from legacy to SIMPLNX silently corrected the bug — most likely the porter manually wrote the loop bound instead of mechanically copy-pasting the legacy line, and used `kernelSize[0]` consistently for both the lower and upper bounds.

**Why this bug went undetected in 6.5.171:** Default and most shipping pipelines use symmetric kernels (`{1,1,1}` is the parameter default; the Small_IN100 reference pipelines all use `{1,1,1}`). The bug is dormant for any symmetric kernel and produces correct output. Asymmetric kernels are uncommon in published DREAM3D workflows but are a real use case for non-isotropic-voxel-spacing serial-section EBSD data.

**Affected users:** DREAM3D 6.5.171 users who ran `FindKernelAvgMisorientations` with an asymmetric `KernelSize`. The output is silently wrong: cells near the upper x-boundary may also see different in-kernel neighbor counts than expected due to the boundary clamp now interacting with the wider-than-requested x-iteration range.

**Recommendation:** **Trust SIMPLNX.** The bug was fixed at port time and SIMPLNX has produced the correct kernel shape for all kernel parameters since the OrientationAnalysis plugin was first ported. Users migrating from DREAM3D 6.5.171 with asymmetric kernels should expect KAM values to change toward the mathematically correct (intended-kernel) value.

A legacy backport branch of `FindKernelAvgMisorientations.cpp` with `m_KernelSize.z + 1` changed to `m_KernelSize.x + 1` would produce the corrected values on DREAM3D 6.5.171 for users requiring legacy-version-parity post-correction. The fix is a one-character edit. No such backport branch is currently maintained.

This bug is documented in `/Users/mjackson/Desktop/bug_triage.md` (Bug #9, added during this V&V cycle) as a known legacy DREAM3D 6.5.171 issue with no SIMPLNX-side action required.

---

## Non-deviations (algorithm characteristics common to both filters)

The following behaviors are NOT deviations — SIMPLNX (post-EbsdLib 2.4.1) and DREAM3D 6.5.171 (with D2 dormant on symmetric kernels) agree on them where D1 precision noise is below the user's tolerance. Captured here so future engineers don't re-discover them and propose them as deviations.

### Focal voxel always included in the kernel sum

Both implementations have the focal cell as a same-feature neighbor of itself (the `j=k=l=0` inner iteration produces `neighbor = point`). The focal cell's self-misorientation contributes 0° to `totalMisorientation` and 1 to `numVoxel`. This is intentional algorithm characteristic — it provides a non-zero divisor for cells with no in-kernel same-feature neighbors (a single-voxel isolated grain). **Both filters share this behavior** — algorithm characteristic, not a defect.

### Background cell short-circuit to KAM = 0

Both implementations branch on `featureIds[point] == 0 || cellPhases[point] == 0` at the *end* of the per-cell processing (legacy line 311, SIMPLNX line 136) and unconditionally set `KAM = 0` for these cells. The logic is logically AFTER the kernel loop but only fires when the focal validity check at the top failed (so the kernel loop didn't run). **Both filters share this behavior**.

### `numVoxel == 0` fallback (dead code in practice)

Both implementations include an `if(numVoxel == 0) { KAM[point] = 0; }` guard immediately after the `KAM[point] = totalMiso / numVoxel` divide. In practice the focal voxel always self-matches (path 6 in the V&V report's code path coverage), so `numVoxel >= 1` whenever the focal validity check at the top of the cell processing was passed. The fallback is dead code. **Both filters share this dead code** — could be removed in both, but no functional issue.

### Multi-threading model

Both implementations parallelize over the outer cell loop. Legacy uses `tbb::parallel_for` over a single dimension after marshalling; SIMPLNX uses `ParallelData3DAlgorithm` with a 3D `Range3D`. Both make concurrent reads of the shared input `DataArray`s (FeatureIds, CellPhases, Quats, CrystalStructures). Per the SIMPLNX project policy (`CLAUDE.md`), DataArray subscript access is not formally thread-safe for concurrent reads, but in practice this works for read-only access on contiguous in-memory DataStores. The algorithm has been stable under parallel execution on shipping pipelines; no thread-safety issue surfaced during the V&V cycle's 6-test suite. Out-of-core (OOC) DataStore variants would need explicit testing, but this filter explicitly calls `parallelAlgorithm.requireArraysInMemory(algArrays)` at line 196 to refuse OOC inputs.
