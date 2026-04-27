# Retroactive V&V: ComputeFeatureNeighborMisorientationsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `0b68fe25-b5ef-4805-ae32-20acb8d4e823` |
| SIMPLNX ClassName | `ComputeFeatureNeighborMisorientationsFilter` |
| SIMPLNX Human Name | Compute Feature Neighbor Misorientations |
| SIMPL UUID | `{286dd493-4fea-54f4-b59e-459dd13bbe57}` (per `simpl_conversion/6_5/...json`) |
| SIMPL ClassName | `FindMisorientations` (per `simpl_conversion/6_4/...json` Filter_Name) |
| SIMPL Human Name | Find Feature Neighbor Misorientations *(typical SIMPL label — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureNeighborMisorientationsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeFeatureNeighborMisorientationsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeFeatureNeighborMisorientationsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeFeatureNeighborMisorientationsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureNeighborMisorientationsFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** — direct translation of the SIMPL `FindMisorientations` filter. The algorithm structure (loop over features starting at index 1, fetch quaternion, walk the per-feature neighbor list, dispatch to `LaueOps::calculateMisorientation`, optionally accumulate average) mirrors the legacy implementation. SIMPL UUID is preserved through the SIMPLConversion path; new SIMPLNX UUID was assigned.
- **Evidence:** No rewrite signal in PR history. All material edits since 2025-10-01 are surgical (EbsdLib API rename, cancel-check insertion, error message improvement). The algorithm body is ~120 lines and delegates the math entirely to EbsdLib.
- **Action required:** Confirm by reading the corresponding SIMPL `FindMisorientations.cpp` and running `compare-legacy-dream3d` (Step 0 e) against a shared toy dataset.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1516 Application error handling, #1547 doc subgroup label fix, #1576 error-message wording, #1538 zlib extraction) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1439 — *"ENH/API: Multi-Dimensional Tuple Support for StringArray and NeighborList"* — merged 2025-10-03

- **Files in this filter:** filter (.cpp), test (.cpp). Only single-line touches in this filter; broader API rework lives in NeighborList itself.
- **Diff size:** Trivial in this filter (call-site adaptations to the new NeighborList API).
- **Change nature:** **API plumbing.** This filter consumes a `NeighborList<int32>` and produces a `NeighborList<float32>`, so it picks up signature/shape changes free. No algorithmic change in this filter, but the *NeighborList* container's tuple-shape semantics changed underneath it.
- **V&V content:** None at the filter level. The risk surface is in NeighborList itself, not here.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25

- **Files in this filter:** algorithm (.cpp) only — 2 lines (include syntax change `"EbsdLib/..."` → `<EbsdLib/...>`).
- **Diff size:** Trivial in this filter. (The PR is large overall — it carries the C-axis bug fixes, ComputeAvgOrientations correction, and `REV: Compute Feature Neighbor Code Review` notation in the commit message. The "Compute Feature Neighbor" review note **may** refer to this filter or to `ComputeFeatureNeighborCAxisMisalignments` — engineer to confirm which file the REV note actually targets.)
- **Change nature:** Include-syntax hygiene in this filter; broader microtexture correctness pass elsewhere.
- **V&V content:** **Possibly relevant** — if the REV note in the commit message refers to *this* filter, then this PR represents the only line-by-line algorithm review in the entire history. Worth confirming with the developer. The mechanical diff to this file is trivial.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 (broad refactor, exception flagged because this filter is an EbsdLib-delegating filter and the rename touches the math call site directly)

- **Files in this filter:** algorithm (.cpp), 4 lines changed.
- **Diff size:** +4 / -4 lines.
- **Change nature:** **API rename only.** Substitutions: `LaueOps` → `ebsdlib::LaueOps`, `QuatF` → `ebsdlib::QuatD` (note: float→double type change for the quaternion local), `OrientationD` → `ebsdlib::AxisAngleDType`. The downstream conversion `axisAngle[3] * k_180OverPiF` is unchanged.
- **V&V content:** **Behavioral risk flagged.** The local quaternion type changed from `QuatF` (single precision) to `QuatD` (double precision), but the input array is still `Float32Array` and the result is still cast back to `float`. The ingest into the double-precision `QuatD` is a widening conversion, then `calculateMisorientation` runs in double, then the angle is narrowed back to float. Net effect should be slightly *more* accurate than legacy SIMPL. Whether legacy's single-precision path gives bit-identical results is the open question — if it does not, this is a candidate Deviation entry vs. SIMPL.

### PR #1582 — *"ENH: Add missing cancel checks to lots of filters"* — merged 2026-04-08

- **Files in this filter:** algorithm (.cpp), +5 lines.
- **Change nature:** **Responsiveness fix.** Adds an `if(m_ShouldCancel) return {};` at the top of the outer feature loop. No algorithmic change.
- **V&V content:** Improves cancel responsiveness; does not change output for non-cancelled runs.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +49 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeFeatureNeighborMisorientationsFilter.json` (1148 bytes)
  - `test/simpl_conversion/6_5/ComputeFeatureNeighborMisorientationsFilter.json` (1091 bytes)
- **Change nature:** **Test addition.** Per-filter SIMPL→SIMPLNX backwards-compatibility test exercising both SIMPL 6.4 (Filter_Name `FindMisorientations`) and 6.5 (UUID `{286dd493-4fea-54f4-b59e-459dd13bbe57}`) conversion paths via `DYNAMIC_SECTION`. Test name: `"OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only.** Verifies that the seven SIMPL parameters (`FindAvgMisors`, `NeighborListArrayPath`, `AvgQuatsArrayPath`, `FeaturePhasesArrayPath`, `CrystalStructuresArrayPath`, `MisorientationListArrayName`, `AvgMisorientationsArrayName`) round-trip into the corresponding SIMPLNX argument keys. Does **not** verify filter output matches legacy.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1516 | Implement error handling for simplnx::Application | Application-level, no filter-behavior change |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure |
| #1547 | Fix filter documentation and documentation related code bugs | Doc subgroup label change `Crystallographic` → `Crystallography` |
| #1576 | Improve error messages across the codebase | Reworded the `-34500` preflight error to include path and component count; no behavioral change |

## Test coverage detected

`ComputeFeatureNeighborMisorientationsTest.cpp` contains **3 `TEST_CASE`s**:

1. `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter` — exercises the **default path** (`ComputeAvgMisors = false`) on the Small IN100 dataset (`6_6_stats_test_v2.dream3d`), then compares the produced `CalculatedMisorientationList` against the exemplar `MisorientationList` via `UnitTest::CompareNeighborLists<float>`. **Note:** does NOT exercise the `ComputeAvgMisors = true` branch — the average-misorientation accumulation logic is untested.
2. `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Misorientation Per Feature` — **stub** marked `[.][UNIMPLEMENTED][!mayfail]` with body comment *"TODO: needs to be implemented. This will need the input .dream3d file to be regenerated with the missing data generated using DREAM3D 6.6."* This is the test that *would* cover the average-misorientations branch — it has been left unimplemented since at least 2025-10-01.
3. `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

**Coverage gap:** The `ComputeAvgMisors = true` branch — including the per-mismatched-phase decrement of `tempMisoList`, the divide-by-non-skipped-count, and the `NaN` fallback when *all* neighbors had a mixed phase — has **no test exercise** in CI. This is the most error-prone control flow in the algorithm body.

## Exemplar archive

- **Archive name:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 130)
- **Inner exemplar file:** `6_6_stats_test_v2.dream3d`, shared with multiple OrientationAnalysis stats tests.
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplars were generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. The `6_6_` prefix suggests these exemplars were generated by DREAM3D 6.6 — confirm. Promote into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class:** **Class 3 (Paper-based)** — Rowenhorst et al., *"Consistent representations of and conversions between 3D rotations"*, Modelling Simul. Mater. Sci. Eng. 23 (2015) 083501, plus standard texture-textbook treatment of crystallographic disorientation (e.g., Randle & Engler). The math is delegated to EbsdLib's `LaueOps::calculateMisorientation`, which returns the minimum-angle disorientation under the relevant Laue group's symmetry operators.
- **Class 4 (Invariant) companion:** Defensible invariants the test should assert:
  1. All misorientation angles lie in `[0°, max-fundamental-zone-angle-for-Laue-group]` (e.g., `≤ 62.8°` for cubic-m3m, `≤ 93.8°` for hexagonal-6/mmm).
  2. Symmetry: `misor(i, j) == misor(j, i)`. The current loop computes both; the test could verify they agree.
  3. Mixed-phase pairs produce `NaN`, never a finite value.
  4. List length per feature equals neighbor-list length per feature.
- **Class 1 (Analytical) companion:** Two synthetic features with identical `AvgQuats` produce `misor = 0` (within float tolerance). Two features whose quaternions differ by exactly one symmetry operator of the Laue group produce `misor = 0`. These are easy hand-derivable spot-checks that cost little to add.
- **Defense of Class 3:** Both Rowenhorst and the standard texture textbooks give well-defined formulas for disorientation under cubic and hexagonal symmetry. EbsdLib implements those formulas. The Class-3 recommendation is appropriate; Class 4 invariants are encoded as supplementary assertions, and Class 1 spot-checks pin specific known answers.
- **Action required:** Developer to confirm Class 3 recommendation and pick the canonical paper citation for the deviation report.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Possibly performed | PR #1438 commit message contains `REV: Compute Feature Neighbor Code Review` — engineer to confirm whether this targeted *this* filter or `ComputeFeatureNeighborCAxisMisalignments`. No structured review document on file. |
| Code path coverage (algorithmic) | **Partial** | Only the `ComputeAvgMisors = false` path is tested. The avg-misor branch has an unimplemented stub since at least 2025-10-01. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `6_6_stats_test_v2.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The `6_6_` prefix suggests legacy DREAM3D 6.6 generated them. |
| Oracle class recorded | **No** | This document is the first to propose one (Class 3 + Class 4 + Class 1). |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. |
| Deviation entries (`ComputeFeatureNeighborMisorientations-D<N>`) | None | Not yet written. Candidates: float→double quaternion type change in PR #1472; mixed-phase NaN handling correctness; `tempMisoList` decrement logic when *all* neighbors are mixed-phase. |
| Documentation currency | Probably current | Brief description, includes the mixed-phase NaN note. Updated by PR #1547 (subgroup label). |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 3 (Rowenhorst 2015) is the recommended starting point, with Class 4 invariant assertions as a supplement. Developer to confirm and pin the exact citation.
2. **Implement the unimplemented test.** `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Misorientation Per Feature` is a stub. It needs to exercise `ComputeAvgMisors = true` against a regenerated exemplar that contains a per-feature `AvgMisorientations` array. This closes the largest coverage hole.
3. **Add Class-1 hand-derived spot-checks.** Construct a tiny 2-feature DataStructure where `AvgQuats[0] == AvgQuats[1]` and verify `misor == 0`. Construct a 2-feature DataStructure where the two quaternions differ by a known cubic symmetry operator and verify `misor == 0`. These are fast and don't depend on any tar.gz archive.
4. **Add Class-4 invariant assertions** to the existing exemplar test: assert `0 ≤ angle ≤ k_FZMaxAngleForLaueGroup` for every angle in the produced `NeighborList<float>`, and assert `NaN` never appears when all phases match.
5. **Inspect `6_6_stats_test_v2.tar.gz` and document provenance.** Determine how the exemplars were generated (DREAM3D 6.6 pipeline, hand-built, etc.) and write an Oracle Provenance block for the archive ReadMe.
6. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same toy data. Pay particular attention to (a) bit-identicality after the QuatF→QuatD change in PR #1472 and (b) the `tempMisoList` mixed-phase decrement bug suspected below.
7. **Resolve the REV note.** Confirm whether PR #1438's `REV: Compute Feature Neighbor Code Review` referred to this filter; if so, locate any review notes and incorporate them.
8. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of SIMPL `FindMisorientations` (UUID `{286dd493-4fea-54f4-b59e-459dd13bbe57}`); EbsdLib API renames in PR #1472 introduced a single→double precision change in the local quaternion variable that may shift least-significant bits relative to legacy."*
9. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeFeatureNeighborMisorientations-D1`
> **Filter UUID:** `0b68fe25-b5ef-4805-ae32-20acb8d4e823`
> **Symptom:** Possible least-significant-bit differences in `MisorientationList` values between SIMPLNX and SIMPL 6.5.172 due to PR #1472's promotion of the local quaternion variable from `QuatF` (single precision) to `QuatD` (double precision). The input array is still `Float32Array`, but the math now runs in double precision before being narrowed back to float for storage.
> **Root cause:** Improvement in SIMPLNX. Legacy SIMPL ran the disorientation calculation in single precision throughout. SIMPLNX widens to double for the calculation.
> **Affected users:** Anyone byte-comparing SIMPLNX output against SIMPL 6.5.172 on the same input. Differences should be at the float-precision noise floor (~1e-6 degrees) and are not material for downstream analysis.
> **Recommendation:** Trust SIMPLNX. Document the precision-promotion in the deviation report so reviewers do not flag near-zero numeric differences as bugs.
> **Status:** **Proposed** — pending verification that legacy actually exhibits these LSB differences (run the comparison).

> **Deviation ID:** `ComputeFeatureNeighborMisorientations-D2`
> **Filter UUID:** `0b68fe25-b5ef-4805-ae32-20acb8d4e823`
> **Symptom (suspected):** When `ComputeAvgMisors = true` and at least one neighbor of a feature has a different phase / Laue class, the per-neighbor count used to divide the accumulator is decremented by `tempMisoList--` *inside the j-loop on every iteration where the phase mismatches* — but `tempMisoList` is *re-initialized to `featureNeighborList.size()` on every iteration of the j-loop* (line 75 of `ComputeFeatureNeighborMisorientations.cpp`: `tempMisoList = featureNeighborList.size();`). The result is that after the j-loop, `tempMisoList` reflects only the **last** neighbor's mismatch state (size, or size-1), not a true count of matched-phase neighbors. The divisor used at line 99 (`(*avgMisorientations)[i] /= static_cast<float>(tempMisoList);`) is therefore wrong whenever any phase mismatch occurs.
> **Root cause:** Likely a port-fidelity bug. The `tempMisoList = featureNeighborList.size()` assignment looks like it was meant to be *outside* the j-loop (initialize once per feature), but is currently inside it. The legacy SIMPL filter may have the same bug — check.
> **Affected users:** Any pipeline that enables `ComputeAvgMisors = true` with mixed-phase ensembles. Single-phase data is unaffected (no mismatches → divisor equals neighbor count, which is correct).
> **Recommendation:** **DO NOT FIX YET.** First run the legacy comparison. If SIMPL 6.5.172 has the same bug, document as a shared legacy bug and propose a coordinated fix on both sides. If only SIMPLNX has it, this is a regression to fix.
> **Status:** **Suspected — engineer to verify by reading the source carefully and tracing on a 2-phase toy dataset.** This is exactly the kind of bug the unimplemented `Misorientation Per Feature` test (gap #2 above) would catch.

> **Deviation ID:** `ComputeFeatureNeighborMisorientations-D3`
> **Filter UUID:** `0b68fe25-b5ef-4805-ae32-20acb8d4e823`
> **Symptom:** Feature index 0 (the unassigned/background feature) is skipped (loop starts at `i = 1`), but the `MisorientationList` and `AvgMisorientations` arrays are sized to `totalFeatures` and the index-0 entry is left at default-initialized (empty list / 0.0f). Downstream consumers iterating over `[0, totalFeatures)` see a zero/empty entry for feature 0.
> **Root cause:** Convention — feature ID 0 is the "unassigned" sentinel in DREAM3D / SIMPLNX feature-ID arrays. This matches legacy SIMPL behavior and is not a bug per se, but it is an invariant the user / reviewer should be made aware of.
> **Affected users:** Anyone iterating starting at index 0 instead of index 1.
> **Recommendation:** Document in the filter markdown ("Feature 0 is the unassigned background and has an empty misorientation list / zero average").
> **Status:** **Documentation gap, not a deviation from legacy.** Listed here so it is captured during the audit.
