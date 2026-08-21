# Deviations from DREAM3D 6.5.171: ComputeKernelAvgMisorientationsFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent (`FindKernelAvgMisorientations`, source at `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindKernelAvgMisorientations.{h,cpp}` in DREAM3D 6.5.171).

Entries are referenced by stable ID (`ComputeKernelAvgMisorientationsFilter-D<N>`) from the V&V report and from public migration guidance. The ID is stable across renames; the Filter UUID field is the permanent cross-reference anchor.

## Comparison summary

The legacy A/B comparison was performed by source inspection (2026-06-03) **and by an empirical runtime A/B on the default per-grain path** (2026-07-15, branch `topic/kam_ignore_feature_ids`). SIMPLNX `ComputeKernelAvgMisorientations` is a clean Port of legacy `FindKernelAvgMisorientations::execute()` (same per-voxel outer triple loop; same per-kernel inner triple loop; same focal-validity gate; on the default path the same same-feature gate inside the kernel — legacy line 292 ≡ SIMPLNX line 124 `true`-branch; same per-voxel average with the focal voxel always included in the divisor). The port-time deltas are documented in the V&V report's Algorithm Relationship section. Three deviations are recorded: a precision-class non-deviation (D1) traceable to the EbsdLib 2.4.1 release **and now empirically quantified** (see D1's runtime-A/B block); a legacy bug (D2) at the inner x-loop bound corrected at port time; and an NX-only capability (D3), the `use_feature_ids=false` per-voxel mode added for issue #1613, which has no legacy counterpart and is therefore validated by oracle only, not by comparison.

**Runtime A/B setup.** A synthetic legacy-format input (`.superpowers/sdd/task-6-ab/kam_ab_input.dream3d`, authored with the `compare-legacy-dream3d` writer helper) — a 12×12×12 image, 8 features (2×2×2 octant blocks), single cubic phase, per-cell orientations = per-feature base rotation ⊗ ≤3° intra-grain perturbation — was fed **unchanged** through both binaries: `PipelineRunner` (6.5.171 `FindKernelAvgMisorientations`) via `legacy_kam.json`, and `nxrunner` (`ComputeKernelAvgMisorientations`, `use_feature_ids=true`) via `nx_kam.d3dpipeline` (nxrunner imports the legacy v7 file directly). Kernel `{1,1,1}` (default; D2 dormant). The round-tripped `Quats` and `FeatureIds` were verified bit-identical in both outputs, so any KAM difference is algorithmic/numeric, not an input artifact.

---

## ComputeKernelAvgMisorientationsFilter-D1

| Field            | Value                                                                            |
|------------------|----------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D1`                                       |
| **Filter UUID**  | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035`                                           |
| **Status**       | active (precision-class; non-deviation in algorithmic sense)                     |

**Symptom:** Per-cell `KernelAverageMisorientations` values differ between SIMPLNX built against fixed EbsdLib (≥ v2.4.1, commit `5c8c993`) and DREAM3D 6.5.171 (and, equivalently, between fixed-EbsdLib and pre-fix-EbsdLib SIMPLNX builds — see the dependency note below). The shift is **quaternion-specific, not a uniform per-cell offset**: it is *exactly* 0 for focal cells whose symmetry-reduced self-misorientation lands on the trivial `wmin` candidate (the identity quaternion, and small rotations about a high-symmetry axis), and ~`0.03°` only for focal cells whose self-misorientation is reduced through a non-trivial cubic sym-op candidate (4-fold / 3-fold / 2-fold) that lands at `1 − ε`. Across a real dataset (Small_IN100 and similar) this averages to a per-cell shift of ~`0.005–0.05°`, depending on the focal-cell orientation distribution and the kernel size; it amplifies for asymmetric (e.g. `{2,2,1}`) or single-voxel kernels because the focal-cell self-misorientation term then carries more weight in the average.

**Dependency:** the fix lives in EbsdLib commit `5c8c993`, contained in the `v2.4.1` tag. As of this PR the SIMPLNX `vcpkg.json` pins `ebsdlib version>=2.4.1`, so the **standard vcpkg build now links the fixed EbsdLib** and the artifact no longer appears in any supported configuration: both the standard build (`NX-Com-Qt69-Vtk95-Rel`) and the local-source build (`NX-Com-Qt69-Vtk95-Rel-EbsdLib`, `SIMPLNX_USE_LOCAL_EBSD_LIB=ON`) produce the correct, self-miso-free result. The V&V data-fixture unit tests assert the exact analytical oracle (margin `1e-3`) and pass in both configurations. The artifact reappears only if EbsdLib is pinned below `2.4.1` (e.g. an older vcpkg baseline); the *Empirical confirmation* below was captured against the pre-fix `2.4.0` — the version that shipped before this PR — to characterize the symptom and verify the fix.

**Root cause:** **Precision** — not an algorithm change in either implementation.

The deviation traces to the EbsdLib 2.4.1 release commit `5c8c993` (BlueQuartz Software, 2026-05-29), which replaces a precision-fragile `acos(w)` form in `CubicOps::calculateMisorientationInternal` with a numerically-stable `2·atan2(|v|, w)` form using the explicit reduced-quaternion `v` components. The precision improvement is real and mathematically more correct; for `ComputeKernelAvgMisorientationsFilter` specifically it manifests *more strongly than for the per-pair misorientation filters in this cycle* because the kernel inclusion of the focal voxel triggers a per-cell self-misorientation call. For the pre-fix `acos(w)`-form:

- `q_self_miso = q_focal * q_focal.conjugate() = (0, 0, 0, 1)` mathematically (identity quaternion), so the *true* self-misorientation is exactly 0°.
- The error is **not** introduced by the raw `q * q.conjugate()` product (its `w` component is `|q|² ≈ 1`); it is introduced by the **symmetry reduction**. `calculateMisorientationInternal` maximizes `wmin` over the 24 cubic sym-op candidates, evaluating three candidate forms per sym op: `qco.w()`, `(qco.z() + qco.w())/√2` (the 4-fold-about-c form), and `(qco.x()+qco.y()+qco.z()+qco.w())/2`. For the identity misorientation, several candidates equal 1.0 mathematically — but on float32-sourced quaternions a non-trivial candidate such as `(qco.z() + qco.w())/√2` evaluates to `1 − ε` (with `ε ≈ 1.7e-8`) and can be selected as the maximum. **Which focal quaternions trigger this is candidate-dependent: most reduce on the trivial `qco.w() == 1.0` branch and yield exactly 0; only those whose maximizing candidate is a non-trivial sym-op form land at `1 − ε`.**
- `acos(1 − ε)` near 1 is precision-fragile: the derivative of `acos` at 1 is `-1/√(1-x²) → -∞`, so a `1-ULP` error in `wmin` propagates to a `√(2ε)`-scale error in the angle (then doubled by the `2 * acos(wmin)` step). For `ε` of order `1e-8` this puts the spurious self-miso in the `~0.02–0.03°` range — the fix commit message cites `~0.02°`; the value **measured empirically on this branch is `0.0326°`** (see below). The exact constant depends on the winning sym-op candidate and the platform's float32 quantization, so treat the magnitude as order-of-`0.03°`, not a fixed number.
- The post-fix `2 * atan2(|v|, w)` form, using the **explicit** reduced-quaternion vector components, is numerically stable: components like `(qco.z() - qco.w())` evaluate to *exactly* 0 in IEEE-754 when `qco.z() == qco.w()` regardless of upstream float32 truncation, so `|v| = 0` and the result is exactly 0 for every identity self-misorientation.

The KAM filter is *more sensitive* than `ComputeFeatureNeighborMisorientations` and `BadDataNeighborOrientationCheck` to this precision improvement because:

1. **Self-misorientation contribution.** The KAM kernel includes the focal cell (via the `j=k=l=0` inner iteration). For each focal cell, the algorithm therefore makes one call to `calculateMisorientation` with `q1 == q2`. With pre-fix EbsdLib this call returns a spurious ~0.03° **for the subset of focal orientations whose symmetry reduction lands on a non-trivial sym-op candidate** (exactly 0 for the rest), which gets added to `totalMisorientation` and shifts that cell's average up by `(spurious_self / numVoxel)`. With fixed EbsdLib the call returns 0° for *every* focal orientation and contributes nothing. This is the *entire* KAM-specific deviation — see point 3.

2. **Same-feature large-N averaging.** For a cell in the middle of a large grain with kernel `{1,1,1}`, numVoxel = 27 (all same-feature). The cumulative effect of 27 small precision noises averages out somewhat, but the systematic self-miso contribution is always present.

3. **The deviation is the self-miso term, essentially nothing else.** For *distinct*-orientation pairs the two EbsdLib forms agree to well below `0.0001°` — empirically confirmed: the sibling `ComputeFeatureNeighborMisorientations` data fixtures assert distinct-pair misorientations of `5.0°` and `10.0°` at margin `1e-3` and **pass against both vcpkg `2.4.0` and the fixed EbsdLib**. That filter excludes the focal feature from its neighbor list, so it never makes a `q1 == q2` call and shows no shift. KAM's per-cell shift is therefore attributable *entirely* to the focal-cell self-misorientation term, not to any per-pair precision noise — which is why KAM is the most observable filter in this cycle for the EbsdLib precision fix.

**Affected users:** Anyone migrating from DREAM3D 6.5.171 to SIMPLNX on cubic-phase EBSD data with this filter, *or* anyone running a SIMPLNX build pinned to EbsdLib `< 2.4.1` (the standard vcpkg build now pins `≥ 2.4.1`, so this affects only builds on an older baseline). The shift is per-cell, systematic in sign (always slightly above the true KAM), and proportional to the inverse of the kernel volume (1 / numVoxel) — but only for focal cells whose orientation triggers the artifact (see *Symptom*); unaffected focal cells shift by 0. The figures below are the **upper bound for an affected focal cell**: for `KernelSize = {1,1,1}` on a grain interior the affected-cell shift is `~0.03°/27 ≈ 0.001°`; for `KernelSize = {0,0,0}` (single-voxel kernel — just the focal cell) it is the full `~0.03°` because the divisor is 1.

**Recommendation:** **Trust SIMPLNX (EbsdLib 2.4.1+).** The 6.5.171 result was limited by the well-understood `acos(w near 1)` precision pathology amplified by float32-sourced quaternion inputs; SIMPLNX returns the mathematically correct value. The shift is well below typical EBSD measurement resolution and will not materially affect downstream microstructural analyses, but the cumulative effect on KAM-based maps will be visibly smoother in the post-2.4.1 output. Users requiring exact 6.5.171 reproduction can compile against EbsdLib < 2.4.1 (not recommended).

For the full root-cause walkthrough of the EbsdLib precision improvement, see the precedent characterization in `vv/deviations/BadDataNeighborOrientationCheckFilter.md` §"Non-deviations" → "EbsdLib 2.4.1 CubicOps precision improvement". The characterization there applies equally to this filter, with the additional amplification factor described above.

**Empirical confirmation (V&V cycle, branch `topic/vv/ComputeFeatureNeighborMisorientationsFilter`, 2026-06-04):** The Class 1 / Class 4 data fixtures were run on Apple Silicon against both EbsdLib builds, with per-pair `calculateMisorientation` results instrumented:

- **Distinct-orientation pairs are exact on both builds.** Pairs of `5°`, `10°`, and `15°` apart returned `4.99991°`, `4.99988°`, etc. (`< 0.0002°` from the analytical value) on both vcpkg `2.4.0` and the fixed local EbsdLib. This rules out per-pair precision noise as a contributor and confirms point 3 above.
- **Self-misorientations are 0 for most focal orientations even pre-fix.** `q1 == q2` for the identity and for the `5°`, `10°`, `15°`-about-c focal cells returned *exactly* `0.0°` on vcpkg `2.4.0`.
- **Only the `20°`-about-c focal cells triggered the artifact pre-fix.** Their self-misorientation returned `0.0325663°` on vcpkg `2.4.0` (matching the `~0.033°` derived above), inflating those cells' KAM by `0.0326°/numVoxel`. Example: the 1D x-axis gradient fixture's last cell (`numVoxel = 2`) read `2.51628°` against an analytical `2.5°` — exceeding the test's `1e-3` margin.
- **The fixed EbsdLib zeroes every self-misorientation.** Rebuilding the same fixtures with the `NX-Com-Qt69-Vtk95-Rel-EbsdLib` preset (local EbsdLib at `5c8c993`), all three misorientation suites pass exactly: KAM `134/134` assertions, `ComputeFeatureNeighborMisorientations` `56/56`, `ComputeFeatureReferenceMisorientations` `238/238`.

The data-fixture unit tests assert the analytical oracle directly (margin `1e-3`, no tolerance for the pre-fix artifact). With EbsdLib pinned `≥ 2.4.1` in `vcpkg.json` this is the correct, regression-sensitive choice: it holds in every supported build and would immediately flag any future regression of the EbsdLib precision fix, rather than silently absorbing it under a loose tolerance.

**Runtime A/B confirmation on general 3D orientations (V&V cycle, branch `topic/kam_ignore_feature_ids`, 2026-07-15):** the setup described in *Comparison summary* above (identical 12³ / 8-feature / single-cubic-phase input through both `PipelineRunner` 6.5.171 and `nxrunner`, default per-grain path, kernel `{1,1,1}`) produced the following per-cell `KernelAverageMisorientations` deltas over all 1728 cells:

**Input recipe and seed (for reproducibility):** the input was generated by `.superpowers/sdd/task-6-ab/make_input.py` from a single seeded RNG, `np.random.default_rng(1613)` (seed = issue #1613). The 12×12×12 grid is partitioned into 8 features as 2×2×2 octant blocks (single cubic phase, `CrystalStructures = [999, 1]`); each feature is assigned a random-axis base rotation with angle drawn uniformly from 5–25°, and every cell within that feature then receives an independent small intra-grain perturbation (≤3°, about its own random axis) composed onto the feature's base rotation. This is not raw uniform-random unit quaternions across the volume — it is a per-grain-base-plus-scatter construction, chosen so that a spread of focal-cell self-misorientations exercises the D1 precision path. The script is the source of truth for the exact construction.

| Metric | Value |
|---|---|
| legacy KAM range (min/mean/max) | 0.952 / 2.142 / 3.551° |
| nx KAM range (min/mean/max) | 0.952 / 2.141 / 3.549° |
| \|Δ\| min / mean / max | 2.4e-7 / 7.5e-4 / **7.2e-3°** |
| cells \|Δ\| > 0.001° | 461 / 1728 |
| cells \|Δ\| > 0.01° | **0** / 1728 |
| signed (legacy − nx): cells legacy>nx / legacy<nx | 928 / 800 (bidirectional; sums to 1728) |

Interpretation: the delta is entirely precision-class and is fully explained by D1's family. **Gating is provably identical** on this path — legacy line 292 (`m_FeatureIds[point] == m_FeatureIds[neighbor]`) and SIMPLNX's `use_feature_ids=true` branch admit the same neighbor set for every focal cell, and both include the focal self, so `numVoxel` (the divisor) is identical per cell in both builds. The remaining difference is therefore purely in the per-pair `calculateMisorientation` values, from two combined precision effects: (a) the EbsdLib 2.4.1 symmetry-reduction fix on the focal self-misorientation term (the effect characterized above), and (b) the `QuatF`→`QuatD` port delta — legacy does the misorientation math in `float32`, SIMPLNX in `float64`. Effect (b) is why the delta is **bidirectional** here whereas the earlier pure-φ1 empirical confirmation (2026-06-04) saw legacy ≥ nx: those fixtures used high-symmetry pure-z-axis rotations for which distinct-pair misorientations happen to agree between the two forms to `<1e-4°`, isolating the one-directional self-miso term; on **general 3D orientations** the `float32`-vs-`float64` distinct-pair difference surfaces at the `~1e-3°` scale and takes either sign. Both effects are precision, not algorithmic — no cell exceeds `0.01°` (well below EBSD angular resolution) and there is no structural/gating pattern (a gating difference would show as `O(degrees)` jumps on specific cells, not uniform sub-`0.01°` noise). **Recommendation stands: trust SIMPLNX.**

Scratch artifacts for this A/B (input generator, both pipelines, both output `.dream3d` files, diff script) live under `.superpowers/sdd/task-6-ab/` — outside the source tree, gitignored, and ephemeral; not part of the deliverable. Because the input is fully determined by the recipe and seed above, the 928/800 signed-cell split and the max-\|Δ\|=0.0072° figure are regenerable at any time by re-running `make_input.py` and the two pipelines, independent of whether the scratch directory itself persists.

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

The SIMPLNX algorithm at `Algorithms/ComputeKernelAvgMisorientations.cpp:111` is correct:

```cpp
for(int32_t l = -kernelSize[0]; l < kernelSize[0] + 1; l++)
```

where `kernelSize[0]` is X. The port from legacy to SIMPLNX silently corrected the bug — most likely the porter manually wrote the loop bound instead of mechanically copy-pasting the legacy line, and used `kernelSize[0]` consistently for both the lower and upper bounds.

**Why this bug went undetected in 6.5.171:** Default and most shipping pipelines use symmetric kernels (`{1,1,1}` is the parameter default; the Small_IN100 reference pipelines all use `{1,1,1}`). The bug is dormant for any symmetric kernel and produces correct output. Asymmetric kernels are uncommon in published DREAM3D workflows but are a real use case for non-isotropic-voxel-spacing serial-section EBSD data.

**Affected users:** DREAM3D 6.5.171 users who ran `FindKernelAvgMisorientations` with an asymmetric `KernelSize`. The output is silently wrong: cells near the upper x-boundary may also see different in-kernel neighbor counts than expected due to the boundary clamp now interacting with the wider-than-requested x-iteration range.

**Recommendation:** **Trust SIMPLNX.** The bug was fixed at port time and SIMPLNX has produced the correct kernel shape for all kernel parameters since the OrientationAnalysis plugin was first ported. Users migrating from DREAM3D 6.5.171 with asymmetric kernels should expect KAM values to change toward the mathematically correct (intended-kernel) value.

A legacy backport branch of `FindKernelAvgMisorientations.cpp` with `m_KernelSize.z + 1` changed to `m_KernelSize.x + 1` would produce the corrected values on DREAM3D 6.5.171 for users requiring legacy-version-parity post-correction. The fix is a one-character edit. No such backport branch is currently maintained.

This bug is documented in `/Users/mjackson/Desktop/bug_triage.md` (Bug #9) as a known legacy DREAM3D 6.5.171 issue with no SIMPLNX-side action required.

---

## ComputeKernelAvgMisorientationsFilter-D3

| Field            | Value                                                                              |
|------------------|------------------------------------------------------------------------------------|
| **Deviation ID** | `ComputeKernelAvgMisorientationsFilter-D3`                                         |
| **Filter UUID**  | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035`                                             |
| **Status**       | active (NX-only capability; no legacy counterpart)                                 |

**Symptom:** SIMPLNX exposes a `use_feature_ids` boolean parameter (default `true`) that DREAM3D 6.5.171 `FindKernelAvgMisorientations` does not have. With `use_feature_ids = false`, SIMPLNX computes a **per-voxel** Kernel Average Misorientation in which a kernel neighbor contributes whenever it is in-bounds, has `featureId > 0`, and shares the focal cell's phase — regardless of whether it belongs to the same feature. There is no way to produce this output with DREAM3D 6.5.171, which only ever computes the per-grain KAM (neighbor must share the focal cell's `featureId`).

**Root cause:** **Algorithmic choice** — a deliberate feature addition (issue #1613), not a bug, precision effect, or library difference. SIMPLNX adds a second neighbor-inclusion mode; the legacy filter has only the per-grain mode.

The two modes differ only in the neighbor gate at `Algorithms/ComputeKernelAvgMisorientations.cpp:124`:

```cpp
const bool neighborContributes = useFeatureIds
    ? (featureIds[point] == featureIds[neighborIdx])                                  // per-grain (legacy-equivalent)
    : (featureIds[neighborIdx] > 0 && cellPhases[neighborIdx] == cellPhases[point]);  // per-voxel (NX-only, #1613)
```

The focal-validity gate (`featureIds[point] > 0 && cellPhases[point] > 0`), the boundary clamps, the divisor semantics (focal self always included), and the background short-circuit are all identical between the two modes and unchanged from the legacy behavior.

**Relationship to the default path:** `use_feature_ids = true` is the default and is behavior-identical to every prior SIMPLNX release and to DREAM3D 6.5.171 (up to the D1/D2 precision/bug notes). The per-voxel mode is strictly opt-in; enabling it cannot change the default output. On single-feature single-phase data the two modes are provably equivalent (see the Class 4 mode-equivalence invariant in the V&V report), because every neighbor that passes the per-grain gate also passes the per-voxel gate and vice-versa.

**Affected users:** none in the migration sense — this is additive. Users who want per-voxel KAM (e.g. to visualize sub-grain orientation gradients without feature segmentation, or to include grain-boundary-adjacent lattice curvature) now have it in SIMPLNX with no DREAM3D 6.5.171 equivalent. Users reproducing legacy pipelines leave the parameter at its `true` default and see no change.

**Validation:** Class 1 analytical fixtures (`Class 1 - Per-Voxel Mode`, expected `{5.0, 20/3, 10.0, 10.0, 0, 0}`; `Class 1 - Per-Voxel Mode Two-Phase Gates`, expected `{5.0, 5.0, 0, 0, 0}`) plus the Class 4 `Mode Equivalence on Single Feature` invariant. Because there is no legacy counterpart, this mode is **never** validated by an A/B numeric comparison — the oracle is the sole authority, per V&V policy.

**Recommendation:** **Trust SIMPLNX.** This is a new, oracle-validated capability. No legacy-parity concern applies.

---

## Non-deviations (algorithm characteristics common to both filters)

The following behaviors are NOT deviations — SIMPLNX (post-EbsdLib 2.4.1) and DREAM3D 6.5.171 (with D2 dormant on symmetric kernels) agree on them where D1 precision noise is below the user's tolerance. Captured here so future engineers don't re-discover them and propose them as deviations.

### Focal voxel always included in the kernel sum

Both implementations have the focal cell as a same-feature neighbor of itself (the `j=k=l=0` inner iteration produces `neighbor = point`). The focal cell's self-misorientation contributes 0° to `totalMisorientation` and 1 to `numVoxel`. This is intentional algorithm characteristic — it provides a non-zero divisor for cells with no in-kernel same-feature neighbors (a single-voxel isolated grain). **Both filters share this behavior** — algorithm characteristic, not a defect.

### Background cell short-circuit to KAM = 0

Both implementations short-circuit background cells to `KAM = 0` at the *end* of the per-cell processing. Legacy explicitly checks `featureIds[point] == 0 || cellPhases[point] == 0` (line 311). **Since the review-driven cleanup (commit `7f9cddc7d`, 2026-07-16), SIMPLNX expresses the identical condition as an `else` branch** (`.cpp:144-146`) on the preceding `if(featureIds[point] > 0 && cellPhases[point] > 0)` gate (`.cpp:86`) rather than a second explicit `if` (previously at the pre-cleanup line 146) — logically identical by De Morgan's law for these `int32` arrays whose valid domain is non-negative (the only way to fail `> 0` is to equal `0`). For hypothetical out-of-domain negative ids the pre-cleanup code left the output cell holding uninitialized memory (the `DataStore` allocates without a fill value); the `else` now writes a deterministic `0.0f` there — a strict improvement over undefined behavior, not a preserved-behavior change. The logic is AFTER the kernel loop but only fires when the focal validity check at the top failed (so the kernel loop didn't run). **Both filters share this behavior**; the SIMPLNX code shape changed, the observable output did not (confirmed by the unchanged 9-test suite).

### `numVoxel == 0` fallback (dead code — removed from SIMPLNX 2026-07-16)

Legacy `FindKernelAvgMisorientations` still includes an `if(numVoxel == 0) { KAM[point] = 0; }` guard immediately after the `KAM[point] = totalMiso / numVoxel` divide. Prior to the review-driven cleanup, SIMPLNX carried the identical dead-code guard (the former path 9 in the V&V report's code path coverage). In practice the focal voxel always self-matches (former path 6), so `numVoxel >= 1` whenever the focal-validity gate passed — the fallback was provably unreachable in both implementations. **Commit `7f9cddc7d` (2026-07-16) removed the SIMPLNX-side guard as pure dead code** (behavior-preserving — confirmed by the unchanged 9-test suite passing on both in-core and out-of-core builds); legacy retains its copy unchanged. This is now a code-shape difference only, not a behavioral one: both implementations compute the identical KAM for every reachable input.

### Multi-threading model

Both implementations parallelize over the outer cell loop. Legacy uses `tbb::parallel_for` over a single dimension after marshalling; SIMPLNX uses `ParallelData3DAlgorithm` with a 3D `Range3D`. Both make concurrent reads of the shared input `DataArray`s (FeatureIds, CellPhases, Quats, CrystalStructures). Per the SIMPLNX project policy (`CLAUDE.md`), DataArray subscript access is not formally thread-safe for concurrent reads, but in practice this works for read-only access on contiguous in-memory DataStores. The algorithm has been stable under parallel execution on shipping pipelines; no thread-safety issue surfaced during the V&V cycle's test suite. Out-of-core (OOC) DataStore variants would need explicit testing, but this filter explicitly calls `parallelAlgorithm.requireArraysInMemory(algArrays)` at line 205 (pre-cleanup: line 196), which disables parallelization when any input array is out-of-core (the algorithm then runs serially rather than concurrently accessing the not-thread-safe OOC stores). OOC inputs are still processed correctly — just single-threaded. **Since the review-driven cleanup (commit `7f9cddc7d`), `FindKernelAvgMisorientationsImpl` also builds its `LaueOps` list once in its constructor (a worker member, `.cpp:29,166`) rather than once per `convert()` call — the object is constructed once per `parallelAlgorithm.execute(...)` call and its `m_OrientationOps` member is read-only for the remainder of execution, then shared by const-reference across all parallel `convert()` invocations. This follows the same constructor-built/shared-across-parallel-ranges precedent as `ComputeFeatureFaceMisorientationPerTriangleImpl`, and `calculateMisorientation` remains a const, stateless call on the shared `LaueOps::Pointer` objects — no new thread-safety exposure. Confirmed by the unchanged 9-test suite passing on the out-of-core build (gate 2).**
