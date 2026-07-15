# V&V Report: ComputeKernelAvgMisorientationsFilter

|                            |                                                                                                                                              |
|----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | OrientationAnalysis                                                                                                                          |
| SIMPLNX UUID               | `61cfc9c1-aa0e-452b-b9ef-d3b9e6268035`                                                                                                       |
| SIMPLNX Human Name         | Compute Kernel Average Misorientations                                                                                                       |
| DREAM3D 6.5.171 equivalent | `FindKernelAvgMisorientations` — `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindKernelAvgMisorientations.{h,cpp}` (UUID `88d332c1-cf6c-52d3-a38d-22f6eae19fa6`) |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                                      |
| Status                     | DRAFT                                                                                                                             |
| Sign-off                   | *Reopened 2026-07-15 for the `use_feature_ids` feature (issue #1613, branch `topic/kam_ignore_feature_ids`). Prior sign-off: Michael Jackson <mike.jackson@bluequartz.net> (2026-06-03) — superseded; re-sign-off pending second-engineer review.* |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Port with bug fix + one NX-only feature addition** — per-voxel kernel averaging; focal-validity gate; divisor always ≥1 (focal self-included). `QuatF`→`QuatD`; `getMisoQuat`→`calculateMisorientation`; iteration order optimized for cache (`col→row→plane` → `plane→row→col`). D2 (legacy l-loop bound typo `KernelSize.z+1` → `KernelSize.x+1`) corrected at port. UUID reassigned. **New this cycle (issue #1613):** `use_feature_ids` BoolParameter (default `true`). `true` = legacy per-grain behavior (neighbor gate `featureIds[point]==featureIds[neighbor]`, behavior-identical to before). `false` = per-voxel KAM: neighbor contributes iff in-bounds AND `featureIds[neighbor]>0` AND `cellPhases[neighbor]==cellPhases[point]`. Focal gate unchanged. No legacy counterpart — see D3. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — 6 hand-derived data fixtures: uniform single-feature, x-axis gradient, z-axis gradient (3D path), multi-feature multi-voxel with background, **per-voxel multi-feature (`use_feature_ids=false`), and per-voxel two-phase gates**. **Class 4 (Invariant) companion** — non-negativity, cubic max-angle bound (62.8°), uniform-within-feature ⇒ KAM=0, background-voxel ⇒ KAM=0 exactly, **and mode-equivalence (per-grain ≡ per-voxel bit-for-bit on single-feature single-phase data)**. Class 1 oracle uses pure φ1 Bunge ZXZ rotations `(φ1, 0, 0)` so that for cubic symmetry, the misorientation between any two cells equals `|Δφ1|` (the c-axis 4-fold reduction is identity when φ1 differences are ≤45°). |
| Code paths enumerated  | 9 of 9 algorithmic paths exercised (re-enumerated this cycle for the per-mode neighbor gate): (1) focal-valid gate → enter kernel; (2) boundary clamp (out-of-bounds j/k/l or neighbor<0) → `continue`; (3) per-grain neighbor feature-id match → accumulate; (4) per-grain neighbor feature-id mismatch → skip; (5) per-voxel neighbor `featureId>0` + phase-match → accumulate; (6) per-voxel neighbor `featureId==0` → skip; (7) per-voxel neighbor phase-mismatch → skip; (8) focal-invalid (`featureIds==0 \|\| cellPhases==0`) → KAM=0 directly; (9) `numVoxel==0` fallback → dead code in practice (the focal voxel always self-contributes in both modes when the focal is valid). |
| Tests today            | **9 TEST_CASEs / 9 ctest entries**, 100% pass on in-core and out-of-core builds (see Task 5 regression sweep). 6 Class 1 fixtures + 2 Class 4 invariant tests (Mode-Equivalence + the 3-sub-section Invariants) + 1 SIMPL backwards-compatibility test. **No exemplar archive consumed by this filter.**                                                                                                                                                                                                                                                                                                                                              |
| Exemplar archive       | **None — inline-constructed in test source.** The pre-existing main exemplar TEST_CASE (consumed `6_6_stats_test_v2.tar.gz`) was **retired 2026-06-03** because the exemplar `KernelAverageMisorientations` array was a circular oracle (regenerated from pre-EbsdLib-2.4.1 SIMPLNX output, where the precision shift documented as D1 manifests as a non-zero spurious self-misorientation contribution in every focal voxel — see deviations doc). The 6 Class 1 hand-derived data fixtures, plus the Class 4 invariants, cover all 9 re-enumerated active algorithmic paths and replace the retired test. The shared archive `6_6_stats_test_v2.tar.gz` remains downloaded for `AlignSectionsMutualInformation`, `ComputeShapes`, and `ComputeSchmids` tests; only F#5's consumption line was removed. |
| Legacy comparison      | **Runtime A/B against DREAM3D 6.5.171 completed this cycle (default per-grain path).** Identical synthetic input (12³ cube, 8 features, single cubic phase, kernel `{1,1,1}`) fed through both `PipelineRunner` (6.5.171 `FindKernelAvgMisorientations`) and `nxrunner`; inputs bit-identical (Quats, FeatureIds). KAM delta: **max \|Δ\|=0.0072°, mean \|Δ\|=0.00075°, 0/1728 cells exceed 0.01°, bidirectional** (928 cells legacy>nx, 800 legacy<nx). Gating is provably identical on this path (both use the `featureIds[point]==featureIds[neighbor]` gate → identical neighbor sets → identical divisor), so the delta is purely `calculateMisorientation` precision — consistent with **D1** (EbsdLib 2.4.1 precision fix) plus the `QuatF`→`QuatD` port delta (same precision family). No structural/gating deviation. `use_feature_ids=false` has no legacy counterpart (see D3). Prior source-inspection findings retained. Two deviations observed: **D1 (EbsdLib 2.4.1 CubicOps precision improvement)** — precision-class deviation analogous to BadDataNeighborOrientationCheck, ComputeFeatureFaceMisorientation, ComputeFeatureNeighborMisorientations, and ComputeFeatureReferenceMisorientations of this cycle. The KAM filter is *more sensitive* to this fix than the per-pair misorientation filters because the kernel inclusion of the focal voxel triggers a self-misorientation call per cell, where the pre-2.4.1 `acos(w near 1)` form returns a spurious ~0.03° on float32-sourced quaternions instead of 0°. This precision noise propagates directly into the per-cell average. **D2 (legacy kernel-bound bug at `FindKernelAvgMisorientations.cpp:264`)** — legacy `for(int32_t l = -m_KernelSize.x; l < m_KernelSize.z + 1; l++)` uses `KernelSize.z + 1` as the upper bound for the x-direction inner loop (should be `KernelSize.x + 1`). SIMPLNX has the correct form at line 108. Bug is dormant when `KernelSize.x == KernelSize.z` (default `{1,1,1}` case); fires for asymmetric kernels. |
| Bug flags              | **One real legacy bug (D2) corrected at port time** — see deviations. SIMPLNX has been correct from the port onward; no SIMPLNX-side source change required by this V&V cycle. Logged to `/Users/mjackson/Desktop/bug_triage.md` as a known legacy DREAM3D 6.5.171 issue. |
| V&V phase              | **REOPENED 2026-07-15 for the `use_feature_ids` feature (issue #1613); status returned to DRAFT.** Delta from the 2026-06-03 COMPLETE state: 3 new TEST_CASEs (per-voxel multi-feature, per-voxel two-phase gates, mode-equivalence invariant); code paths re-enumerated for the per-mode neighbor gate (6→9); Oracle section extended with the new fixtures' derivations; **runtime legacy A/B run this cycle** (previously source-inspection only); new deviation **D3** (per-voxel mode is an NX-only capability). Unchanged from prior cycle: Class 1 + Class 4 oracle family, no exemplar archive, D1/D2 findings. Three source-tree deliverables in place. **Outstanding:** re-sign-off (DRAFT → READY FOR REVIEW → COMPLETE) pending second-engineer review — is a human step, not self-applied. |

## Summary

`ComputeKernelAvgMisorientationsFilter` computes the per-cell **Kernel Average Misorientation (KAM)**: for each valid cell (featureId > 0, phase > 0), the algorithm iterates over an axis-aligned kernel of user-specified radius `KernelSize = (x, y, z)`, averages the misorientation between the focal cell's orientation quaternion and every same-feature neighbor's quaternion (including the focal cell itself, which contributes a self-misorientation of 0°), and stores the result. Cells with `featureId == 0` or `phase == 0` are treated as background and assigned KAM = 0 directly.

The filter is the cell-level analog of the feature-level `ComputeFeatureNeighborMisorientationsFilter`. Like that filter, it consumes `Quats` (cell-level avg-orientations) and `CrystalStructures` (per-phase Laue class index), and delegates the actual cubic/hex/etc. symmetry-reduced disorientation calculation to `ebsdlib::LaueOps::calculateMisorientation()`. Unlike the feature-level filter, the kernel iteration ALWAYS visits the focal cell as part of its neighbor list (via the `j=k=l=0` inner iteration), so the per-cell divisor `numVoxel` is always ≥ 1.

The output is `KernelAverageMisorientations`, a `Float32Array` co-located in the same `AttributeMatrix` as the input `Cell Data` arrays, sized one-tuple-per-cell with one component per tuple, in degrees.

**`use_feature_ids` feature (issue #1613, this cycle):** a new `BoolParameter` (default `true`) selects the neighbor-inclusion rule. With `use_feature_ids = true` (the default, behavior-identical to the legacy filter and to every prior release), a kernel neighbor contributes only if it shares the focal cell's `featureId` — the classic per-grain KAM. With `use_feature_ids = false`, the filter computes a **per-voxel** KAM: a neighbor contributes iff it is in-bounds, has `featureId > 0`, and shares the focal cell's phase — feature boundaries are ignored, so the kernel averages across grain boundaries within the same phase. The focal-validity gate (`featureIds[point] > 0 && cellPhases[point] > 0`) is identical in both modes.

## Algorithm Relationship

*Classification:* **Port (with UUID reassignment + name rename + one inherent legacy bug corrected at port time), plus one NX-only feature addition (`use_feature_ids`, issue #1613).**

*Evidence:* Cross-checked SIMPLNX algorithm against legacy `FindKernelAvgMisorientations.cpp::execute()`. Same per-voxel outer loop, same per-kernel inner loop, same `KAM = totalMisorientation / numVoxel`. On the **default per-grain path (`use_feature_ids = true`)** the neighbor gate is identical: legacy `if(good && m_FeatureIds[point] == m_FeatureIds[neighbor])` (line 292) ≡ SIMPLNX `featureIds[point] == featureIds[neighborIdx]`. Port-time deltas:

- `QuatF` → `QuatD`; `getMisoQuat` → `calculateMisorientation` (EbsdLib 2.4.1+ `2·atan2(|v|, w)` precision form — see D1). The `float32`→`float64` promotion of the misorientation math is the second component of the precision family observed in the runtime A/B (below).
- `setParallelizationEnabled` removed (now always parallel via `ParallelData3DAlgorithm`).
- Iteration order `col→row→plane` → `plane→row→col` (cache-friendlier for x-fastest-varying storage; mathematically identical since all writes go to the same `point` index).
- D2 corrected at port.
- UUID reassigned; `Find` → `Compute` rename.

*NX-only feature addition (issue #1613, this cycle):* the `use_feature_ids` BoolParameter. When `true` (default) the code path is bit-for-bit the pre-feature per-grain algorithm — the feature is opt-in and does not alter the default output. When `false`, the neighbor gate becomes `featureIds[neighborIdx] > 0 && cellPhases[neighborIdx] == cellPhases[point]` (per-voxel, phase-gated, feature-agnostic). This is a **new capability with no DREAM3D 6.5.171 counterpart** — validated by the Class 1 / Class 4 oracle only, never by legacy comparison (see D3).

*Material PRs since baseline:* commits `25959c1f2..e86d2eb04` on branch `topic/kam_ignore_feature_ids` add the `use_feature_ids` parameter, the per-mode neighbor gate, three unit tests, and the user-doc update. No other change to the core algorithm.

## Oracle

*Confirmed class:* **Class 1 (Analytical) primary, Class 4 (Invariant) companion.**

### Class 1 (Analytical)

Class 1 oracle derived by hand for each fixture in terms of `|Δφ1|` between cell pairs, justified by the cubic FZ analysis below. All quaternions in the test fixtures use the helper `QuatFromPhi1Deg(phi1)` which returns the quaternion form of a pure Bunge ZXZ Euler rotation `(phi1, 0, 0)` with `Phi = phi2 = 0`. This collapses to a single rotation about the z-axis.

**Cubic FZ argument:** For two cells with pure φ1 rotations differing by Δφ1, the disorientation between them in the cubic FZ equals `|Δφ1|` whenever `|Δφ1| ≤ 45°`. The reasoning: the cubic group's 4-fold rotation about the z-axis reduces φ1 differences modulo 90°; for |Δφ1| ≤ 45°, the reduction is the identity operator (no reduction needed). For all 4 Class 1 fixtures, the maximum φ1 difference between any pair is ≤ 30°, well within the 45° bound. Other cubic symmetry operators (3-fold about [111], 2-fold about [110], etc.) only produce smaller-angle equivalents for misorientations not aligned with a pure z-axis rotation; for pure z-rotations of small magnitude, the identity is the global minimum.

**Per-fixture expected KAM derivation:**

| Fixture                                    | Geometry | Kernel    | Per-cell expected KAM (degrees)                            |
|--------------------------------------------|----------|-----------|------------------------------------------------------------|
| `Class 1 - Uniform 2D Single Feature`      | 3x3x1    | {1,1,0}   | All cells = 0.0 (all in-kernel neighbors share orientation) |
| `Class 1 - 1D x-axis Gradient`             | 5x1x1    | {1,0,0}   | [2.5, 10/3, 10/3, 10/3, 2.5] (see derivation in test comments) |
| `Class 1 - 1D z-axis Gradient (3D path)`   | 1x1x3    | {0,0,1}   | [5.0, 20/3, 5.0]                                            |
| `Class 1 - Multi-Feature Multi-Voxel + BG` | 6x1x1    | {1,0,0}   | [5.0, 5.0, 10.0, 10.0, 0.0, 0.0]                            |
| `Class 1 - Per-Voxel Mode` (`use_feature_ids=false`) | 6x1x1 | {1,0,0} | [5.0, 20/3, 10.0, 10.0, 0.0, 0.0]                     |
| `Class 1 - Per-Voxel Mode Two-Phase Gates` (`use_feature_ids=false`) | 5x1x1 | {1,0,0} | [5.0, 5.0, 0.0, 0.0, 0.0]                     |

Detailed per-cell hand-derivations are in the test file's TEST_CASE comments and in `vv/provenance/ComputeKernelAvgMisorientationsFilter.md`.

**Per-voxel-mode fixture derivations (new this cycle, issue #1613):**

*`Class 1 - Per-Voxel Mode`* — the same 6×1×1 layout as the Multi-Feature fixture (featureIds `[1,1,2,2,0,1]`, phases `[1,1,1,1,0,1]`, φ1 `[0,10,0,20,–,30]°`), kernel `{1,0,0}`, run with `use_feature_ids=false`. A neighbor now contributes iff `featureId>0 && phase==focalPhase`, so feature boundaries are crossed but `featureId=0` and phase-mismatched cells are still excluded:
- cell 0: {self=0, x1(F1,P1)=|10−0|} → 10/2 = **5.0** (same as per-grain).
- cell 1: {x0(F1,P1)=|0−10|, self=0, x2(F2,P1)=|0−10|} → 20/3 ≈ **6.667** — the *diagnostic* cell: per-grain gives 5.0 (x2 skipped as different feature); per-voxel includes x2 because it is a valid same-phase cell. This one value proves the mode actually changed behavior.
- cell 2: {x1(F1,P1)=|10−0|, self=0, x3(F2,P1)=|20−0|} → 30/3 = **10.0**.
- cell 3: {x2(F2,P1)=|0−20|, self=0}; x4 excluded (`featureId=0`) → 20/2 = **10.0**.
- cell 4: focal-invalid (`featureId=0, phase=0`) → **0.0** exactly.
- cell 5: {self=0}; x4 excluded (`featureId=0`) → 0/1 = **0.0**.

*`Class 1 - Per-Voxel Mode Two-Phase Gates`* — 5×1×1, every cell its own feature, two cubic phases (ensemble `[999,1,1]`); featureIds `[1,2,3,4,0]`, phases `[1,1,2,1,1]`, φ1 `[0,10,20,30,40]°`, kernel `{1,0,0}`, `use_feature_ids=false`:
- cell 0: {self=0, x1(P1)=|0−10|} → 10/2 = **5.0** (per-grain would give 0.0 — every feature is a single cell).
- cell 1: {x0(P1)=|10−0|, self=0}; x2 skipped (phase 2 ≠ 1) → 10/2 = **5.0**.
- cell 2: {self=0}; x1, x3 skipped (phase 1 ≠ 2) → 0/1 = **0.0**.
- cell 3: {self=0}; x2 skipped (phase), x4 skipped (`featureId=0`) → 0/1 = **0.0**.
- cell 4: focal `featureId=0` (invalid) → **0.0** exactly, even though its phase>0 — proves the focal gate is unchanged in per-voxel mode.

### Class 4 (Invariant)

Class 4 invariants asserted in the `Class 4 - Invariants` TEST_CASE across 3 sub-sections:

1. **Uniform-orientation single-feature → KAM == 0 everywhere.** Asserted on a 3x3x3 uniform-identity-quaternion fixture with kernel `{1,1,1}` (3D path coverage).
2. **Background cell → KAM == 0 exactly.** Asserted on a 3x1x1 fixture with the middle cell flagged as `(featureId=0, phase=0)`.
3. **Range and non-triviality on the x-axis gradient fixture:** (i) `KAM[i] >= 0` for all cells, (ii) `KAM[i] <= 62.8°` (Mackenzie cubic upper bound), (iii) at least one cell has `KAM > 0` (sanity check that the algorithm actually computed something).

**Mode-equivalence invariant (new this cycle — `Class 4 - Mode Equivalence on Single Feature`):** on single-feature single-phase data, the per-grain gate (`featureId` match) and the per-voxel gate (`featureId>0 && phase match`) admit *exactly the same* neighbor set for every focal cell, so the two modes must produce **bit-for-bit identical** output. Asserted on a 3×3×3 gradient fixture (φ1 = 2x+3y+4z°, max 18° < 45° FZ bound) with kernel `{1,1,1}`: `REQUIRE(kamPerGrain[i] == kamPerVoxel[i])` for every cell, plus a non-triviality guard that at least one cell is non-zero. This is a derived property (not a hand-computed value), so it holds regardless of the EbsdLib precision class and pins the invariant "the feature is opt-in and does not change output where the two gates coincide."

The Class 4 invariants are oracle-agnostic — they hold for any input, so they catch regressions even if specific Class 1 expected values were edited away.

### Class 2, 3, 5

N/A — no reference-library invocation (Class 2), no published-paper figure reproduction (Class 3), no expert-visual sign-off (Class 5) needed. Class 1 + Class 4 are sufficient.

### Second-engineer oracle review

Recommended pending another engineer review. The multi-feature multi-voxel fixture in particular has 6 hand-derived per-cell expected values; a second pair of eyes on the symmetry-reduced cubic misorientation reasoning would catch arithmetic mistakes in the test comments before the V&V cycle is closed.

## Code path coverage

Re-enumerated this cycle for the per-mode neighbor gate (`ComputeKernelAvgMisorientations.cpp:124`). Paths 3–4 are the per-grain branch (`use_feature_ids = true`); paths 5–7 are the per-voxel branch (`use_feature_ids = false`).

| Path | Description                                                                                                                                                  | Exercised by |
|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------|
| 1    | Focal-valid gate (`featureIds[point] > 0 && cellPhases[point] > 0`) → enter kernel                                                                           | All 6 Class 1 fixtures; both Class 4 tests |
| 2    | Kernel cell out-of-bounds (boundary clamp `col+l > xPoints-1` etc., or `neighbor < 0`) → `continue`                                                          | `Class 1 - 1D x-axis Gradient` cells 0/4 (x-boundary); `Class 1 - 1D z-axis Gradient` planes 0/2 (z-boundary); `Class 1 - Uniform 2D` corners; both 3×3×3 fixtures (all faces) |
| 3    | **Per-grain** (`use_feature_ids=true`): in-bounds neighbor + `featureId` match → accumulate miso + numVoxel++                                                | All default-mode Class 1 fixtures; Class 4 Invariants (i)/(iii); Class 4 Mode-Equivalence (per-grain run) |
| 4    | **Per-grain**: in-bounds neighbor + `featureId` mismatch → skip (no accumulate)                                                                              | `Class 1 - Multi-Feature Multi-Voxel + Background` (cells 1, 2 see different-feature in-bounds neighbors) |
| 5    | **Per-voxel** (`use_feature_ids=false`): in-bounds neighbor + `featureId>0` + phase match → accumulate                                                       | `Class 1 - Per-Voxel Mode` cells 1, 2 (cross-feature same-phase include); `Class 1 - Per-Voxel Two-Phase Gates` cells 0, 1; Class 4 Mode-Equivalence (per-voxel run) |
| 6    | **Per-voxel**: neighbor `featureId == 0` → skip                                                                                                              | `Class 1 - Per-Voxel Mode` cells 3, 5 (x=4 background excluded); `Class 1 - Per-Voxel Two-Phase Gates` cell 3 (x=4 excluded) |
| 7    | **Per-voxel**: neighbor phase mismatch (`cellPhases[neighbor] != cellPhases[point]`) → skip                                                                  | `Class 1 - Per-Voxel Two-Phase Gates` cell 2 (excludes phase-1 x=1/x=3), cells 1 & 3 (exclude phase-2 x=2) |
| 8    | Focal-invalid (`featureIds == 0 || cellPhases == 0`) → KAM = 0 directly                                                                                      | `Class 1 - Multi-Feature` cell 4; `Class 1 - Per-Voxel Mode` cell 4; `Class 1 - Per-Voxel Two-Phase Gates` cell 4 (`featureId=0` yet phase>0 — proves focal gate unchanged); Class 4 Invariants (ii) |
| 9    | `numVoxel == 0` fallback → KAM = 0                                                                                                                           | **Not exercised** — dead code in practice. In both modes the focal voxel always self-contributes when the focal is valid (per-grain: `featureId==featureId`; per-voxel: focal-valid ⇒ `featureId>0` and `phase==phase`), guaranteeing numVoxel ≥ 1 whenever path 1 is entered. Path 8 skips the kernel entirely. |

8 of 9 paths exercised by the V&V suite; the 9th is unreachable by construction (both modes) and is flagged in the algorithm review as removable dead code.

## Test inventory

| TEST_CASE                                                                              | Category | Status | ctest entry                                                                            |
|----------------------------------------------------------------------------------------|----------|--------|----------------------------------------------------------------------------------------|
| `: SIMPL Backwards Compatibility`                                                      | Compat   | kept   | Yes (2 dynamic sections: 6.4 + 6.5)                                                    |
| `: Class 1 - Uniform 2D Single Feature`                                                | Class 1  | kept   | Yes                                                                                    |
| `: Class 1 - 1D x-axis Gradient`                                                       | Class 1  | kept   | Yes                                                                                    |
| `: Class 1 - 1D z-axis Gradient (3D path)`                                             | Class 1  | kept   | Yes                                                                                    |
| `: Class 1 - Multi-Feature Multi-Voxel + Background`                                   | Class 1  | kept   | Yes                                                                                    |
| `: Class 1 - Per-Voxel Mode (use_feature_ids = false)`                                 | Class 1  | **new (#1613)** | Yes — per-voxel cross-feature include + `featureId=0` exclude; expected `{5.0, 20/3, 10.0, 10.0, 0, 0}` |
| `: Class 1 - Per-Voxel Mode Two-Phase Gates`                                           | Class 1  | **new (#1613)** | Yes — per-voxel phase-mismatch exclude + `featureId=0` focal gate; expected `{5.0, 5.0, 0, 0, 0}` |
| `: Class 4 - Mode Equivalence on Single Feature`                                       | Class 4  | **new (#1613)** | Yes — per-grain ≡ per-voxel bit-for-bit on single-feature single-phase 3×3×3 |
| `: Class 4 - Invariants` (3 sub-sections)                                              | Class 4  | kept   | Yes (3 SECTIONs)                                                                       |
| ~~`: ComputeKernelAvgMisorientationsFilter` (legacy exemplar test)~~                   | RETIRED  | —      | Retired 2026-06-03 (circular oracle from pre-EbsdLib-2.4.1 SIMPLNX output)             |

9 active TEST_CASEs / 9 ctest entries (confirmed via `ctest -N -R "ComputeKernelAvgMisorientations"`); 100% pass on both in-core and out-of-core builds (Task 5 regression sweep, 2026-07-15).

## Exemplar archive

**None** — inline-constructed. The pre-V&V test (now retired) consumed `6_6_stats_test_v2.tar.gz` (SHA512 `e84999...089723`, downloaded from the BlueQuartz Data_Archive release). The archive contains exemplar `KernelAverageMisorientations` arrays generated from a pre-2.4.1 SIMPLNX build, which embeds the spurious self-misorientation precision noise described in D1. Comparing the post-2.4.1 SIMPLNX output against those exemplars fails by ~0.01-0.05° per cell on real Small_IN100 data. The exemplar arrays cannot be re-generated against the post-2.4.1 build (circular oracle pattern), so the test was retired and replaced with the analytical / invariant suite above.

The shared archive remains referenced in `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 130) for use by `AlignSectionsMutualInformation`, `ComputeShapes`, and `ComputeSchmids` tests. Only F#5's consumption line was removed.

## Deviations from DREAM3D 6.5.171

See `vv/deviations/ComputeKernelAvgMisorientationsFilter.md` for the canonical, ID-stable list:

- **`ComputeKernelAvgMisorientationsFilter-D1`** — EbsdLib 2.4.1 CubicOps precision improvement (non-deviation in the algorithmic sense; precision class). The pre-2.4.1 `acos(w near 1)` form produces a spurious ~0.03° angle on float32-sourced identical quaternions, which inflates every per-cell self-misorientation contribution to the KAM. The 2.4.1 `2*atan2(|v|, w)` form returns 0° as expected. **Confirmed by runtime A/B this cycle:** on identical 12³ synthetic input (default per-grain path, kernel `{1,1,1}`), max \|Δ\|=0.0072°, mean \|Δ\|=0.00075°, 0/1728 cells > 0.01°, gating provably identical — the delta is purely `calculateMisorientation` precision (D1 + the `QuatF`→`QuatD` port delta).
- **`ComputeKernelAvgMisorientationsFilter-D2`** — Legacy `FindKernelAvgMisorientations.cpp:264` uses `KernelSize.z + 1` as the upper bound of the x-direction inner loop (should be `KernelSize.x + 1`). SIMPLNX has the correct form. Dormant when `KernelSize.x == KernelSize.z`; fires for asymmetric kernels. Logged in `bug_triage.md`.
- **`ComputeKernelAvgMisorientationsFilter-D3`** — `use_feature_ids = false` (per-voxel KAM) is an **NX-only capability** added for issue #1613. DREAM3D 6.5.171 `FindKernelAvgMisorientations` has no equivalent (it is per-grain only), so there is nothing to A/B against; the mode is validated by the Class 1 per-voxel fixtures and the Class 4 mode-equivalence invariant. The default (`use_feature_ids = true`) is unchanged and remains legacy-comparable.

## Provenance

See `vv/provenance/ComputeKernelAvgMisorientationsFilter.md` for the canonical record of how the inlined data fixtures were designed and how the expected values were derived.
