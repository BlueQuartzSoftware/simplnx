# Retroactive V&V: ComputeFeaturePhasesFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` |
| SIMPLNX ClassName | `ComputeFeaturePhasesFilter` |
| SIMPLNX Human Name | Compute Feature Phases |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | `FindFeaturePhases` *(historical legacy name; confirm in legacy SIMPL repo)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeaturePhasesFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeaturePhases.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ComputeFeaturePhasesFilterTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ComputeFeaturePhasesFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/ComputeFeaturePhasesFilter.json`
- `src/Plugins/SimplnxCore/docs/ComputeFeaturePhasesFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter appears to be a direct translation of the legacy SIMPL `FindFeaturePhases` filter; the human-name change ("Find" → "Compute") is cosmetic and the algorithm is the same one-pass voxel scan that copies a per-voxel phase into the corresponding feature slot.
- **Evidence:** No rewrite signal in PR history. The algorithm body is small (~50 lines) and well-known. PR #1455 was a warning-emission cleanup (no behavior change). PR #1544 was a mechanical Algorithm-class extraction (no behavior change). PR #1588 was a SIMPL-conversion test addition (no behavior change). The implementation has been stable since before the 2025-10-01 audit window.
- **Action required:** Confirm by reading the corresponding SIMPL `FindFeaturePhases` source and running `compare-legacy-dream3d` step (e) against a shared toy dataset.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1535 preflight cleanup, #1521 unused-algorithm-code move, #1538 zlib extraction, #1476 backwards-compat scaffold, #1301 algorithm-class scaffolding) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1455 — *"BUG: Compute Feature Phases Warnings Consolidated"* — merged 2025-10-07

- **Files in this filter:** filter (.cpp) only
- **Diff size:** 1 file, +13 / -13 lines
- **Change nature:** **User-facing warning consolidation.** Replaced an internal `std::map<int32,int32>` (feature → warning-count) with a `std::set<int32>` (the set of features that had inconsistent per-voxel phase IDs), and emitted a single comma-separated warning listing the affected feature IDs instead of one warning per feature. The PR description (referencing issue #1262) notes the per-feature counts were not actionable for the user since the offending cells were not reported, and the set form is also lower-memory and faster.
- **V&V content:** **Low-but-not-zero.** No algorithmic change to the computed `FeaturePhases` output. The change is observable only in the message-handler stream (number / shape of warning messages). It does, however, document an explicit invariant: *"if voxels in feature `i` carry inconsistent phase IDs, the LAST phase encountered during the linear scan wins."* That last-write-wins behavior is now embedded in both the source comment and the user-visible warning.

### PR #1543 — *"DOC: Update pipeline references in each of the documentation files"* — merged 2026-02-24

- **Files in this filter:** docs (.md), +2 / -2 lines
- **Change nature:** Documentation hygiene — refreshed the `## Example Pipelines` block to point at the current pipeline filenames (`(02) Small IN100 Full Reconstruction`, `INL Export`).
- **V&V content:** None algorithmic. Doc currency only.

### PR #1544 — *"ENH: Move Filter executeImpl() logic to Algorithm classes"* — merged 2026-02-26

- **Files in this filter:** filter (.cpp), algorithm (.cpp NEW, .hpp NEW)
- **Diff size:** 3 files, +147 / -57 lines (algorithm gained 80+59 lines; filter shrank by ~57 lines)
- **Change nature:** **Mechanical refactor (Issue #1284).** Extracted the executeImpl() body into a new `ComputeFeaturePhases` algorithm class under `Algorithms/`, with a `ComputeFeaturePhasesInputValues` struct as the parameter-passing vehicle. The filter's executeImpl now does nothing but populate the struct and invoke the algorithm functor. The algorithm body itself (the for-loop that builds `featureMap` and assigns into `featurePhases`) is unchanged from PR #1455.
- **V&V content:** None algorithmic — the goal was separation-of-concerns, not behavior change. PR description explicitly states the pattern is "moving execute logic from Filter files into dedicated Algorithm classes" with no functional differences. Risk: any silent behavior change introduced during the move would be invisible because the unit test exemplar comparison uses the same input data file (`6_6_stats_test_v2.dream3d`) before and after.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +46 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeFeaturePhasesFilter.json` (766 bytes)
  - `test/simpl_conversion/6_5/ComputeFeaturePhasesFilter.json` (823 bytes)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::ComputeFeaturePhasesFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values (CellPhasesArrayPath, CellFeatureIdsArrayPath, CellFeaturesAttributeMatrixPath, FeaturePhasesArrayName). It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1301 | Add missing algorithm classes to some filters | Scaffold-only; placed an empty `not_used/ComputeFeaturePhases.cpp` stub that was later replaced by PR #1544 |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1476 | Fix Backwards Pipeline Compatibility and Add Testing | Centralized backwards-compat infrastructure later replaced by PR #1588 |
| #1521 | Move unused algorithm codes to internal directory | File-organization refactor (the empty `not_used/` stub) |
| #1535 | Remove redundant preflight checks | Cleanup |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure |

## Test coverage detected

`ComputeFeaturePhasesFilterTest.cpp` contains 2 `TEST_CASE`s:

1. `SimplnxCore::ComputeFeaturePhasesFilter(Valid Parameters)` — runs the filter against the `6_6_stats_test_v2.dream3d` exemplar data, computes a new `Computed_Phases` array under the Cell Feature Data attribute matrix, and tuple-by-tuple compares it against the in-archive exemplar `Phases` array.
2. `SimplnxCore::ComputeFeaturePhasesFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

Test 1 exercises the algorithm against a real EBSD-derived dataset. Test 2 is conversion-only.

There is **no** small toy-data test, **no** test for the multi-phase-per-feature warning path (PR #1455's consolidated warning string), and **no** test that verifies the documented invariant on a hand-derivable input.

## Exemplar archive

- **Archive name:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt` (line 233)
- **Provenance:** *(TBD — engineer must inspect the archive.)* The `6_6_` prefix indicates this is a legacy DREAM3D 6.6 carry-over dataset, not a freshly generated NX exemplar. The archive is shared by many `*Filter` tests in `SimplnxCore` (it is the standard "stats" dataset), so the exemplar `Phases` array under `Cell Feature Data` was produced by the legacy DREAM3D 6.x `FindFeaturePhases` run that built the file in the first place.
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the `.dream3d` file's HDF5 history group (which may carry the legacy SIMPL pipeline that produced it), and provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy. Note that because the exemplar was produced by legacy DREAM3D, the existing test 1 is implicitly an "agreement-with-legacy" test — which means a `compare-legacy-dream3d` run should reproduce the same agreement.

## Oracle classification (tentative)

- **Recommended class:** **Class 1 (Analytical) + Class 4 (Invariant)**.
- **Rationale:**
  - **Class 1 (Analytical).** The filter's specification is mathematically trivial. Given per-voxel `CellPhases[v]` and per-voxel `FeatureIds[v]`, for each feature `i` the output is `FeaturePhases[i] = CellPhases[v_i]` for any voxel `v_i` with `FeatureIds[v_i] == i`. By the documented assumption that all voxels in a given feature share a phase, the choice of `v_i` does not matter. A 3-voxel toy dataset (e.g. `FeatureIds=[1,1,2]`, `CellPhases=[1,1,2]` ⇒ `FeaturePhases=[_,1,2]` indexed from 1) yields a closed-form expected output that can be hand-typed into the test.
  - **Class 4 (Invariant) — companion.** The defining invariant is *"for every voxel `v`, `FeaturePhases[FeatureIds[v]] == CellPhases[v]`"* (when the input is consistent). When the input is *inconsistent*, the documented behavior is *"the LAST phase encountered during the linear voxel scan wins, and a warning is emitted listing the affected feature IDs"* — that warning behavior is exactly what PR #1455 nailed down. Both the consistent and inconsistent cases are testable with a toy dataset.
- **Action required:** Developer to defend or replace this dual recommendation. If a paper reference exists in the legacy DREAM3D filter docs, Class 3 could be added. None is expected — this filter is plumbing, not science.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. The algorithm is short (~50 lines) and the review should be quick. |
| Code path coverage (algorithmic) | Partial | Only the "consistent input" path is tested. The inconsistent-phase warning path (PR #1455) and the early-cancel path are not exercised. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `6_6_stats_test_v2.tar.gz` referenced in test/CMakeLists.txt. Shared with many other SimplnxCore tests. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The `6_6_` prefix indicates legacy DREAM3D 6.6 origin, so provenance probably lives in the .dream3d file's pipeline history group. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | No | No script or hand-derivation on file. The filter is trivially amenable to one — a 3-voxel test would close this gap in minutes. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run. The exemplar is implicitly legacy-derived, so the existing test 1 is a partial substitute. |
| Deviation entries (`ComputeFeaturePhases-D<N>`) | None | Not yet written. PR #1455's warning-format change is a candidate Deviation entry vs. SIMPL 6.5.172 if legacy still emits one warning per feature. |
| Documentation currency | Probably current | Updated by PR #1543 (pipeline references). The auto-generated parameter table is in place. Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 + Class 4 is the recommended starting pair; no paper expected.
2. **Add a toy-data test.** A 3-to-10-voxel hand-built `DataStructure` (build `CellPhases` and `FeatureIds` in code, run the filter, assert the resulting `FeaturePhases` array element-by-element against hand-typed expected values) is trivial here and would convert the verification from "matches a legacy-derived exemplar" to "matches a closed-form analytical answer." This is the single highest-value V&V improvement for this filter.
3. **Add an inconsistent-input test.** Build a toy dataset where two voxels of feature `i` carry different `CellPhases` values, run the filter, and assert (a) the warning text contains feature `i`'s ID and (b) the resulting `FeaturePhases[i]` equals the *last* `CellPhases` value encountered during the linear scan. This locks in the documented behavior of PR #1455.
4. **Inspect `6_6_stats_test_v2.tar.gz` and document provenance.** The archive is shared with many tests; the ReadMe should record once which legacy pipeline produced it.
5. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same toy data. The expected outcome is full numeric agreement on `FeaturePhases`. The only candidate Deviation is cosmetic: the warning-message format changed in PR #1455 (from N warnings to 1 warning).
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of SIMPL `FindFeaturePhases`; warning-message format consolidated in PR #1455; executeImpl() body extracted into an Algorithm class in PR #1544."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeFeaturePhases-D1`
> **Filter UUID:** `da5bb20e-4a8e-49d9-9434-fbab7bc434fc`
> **Symptom:** SIMPLNX emits a single comma-separated warning listing all features that contained inconsistent per-voxel phase IDs (e.g. *"Elements from some features did not all have the same phase ID. The last phase ID copied into each feature will be used. Effected Phase Features: 3, 17, 42"*). SIMPL 6.5.172 emits one summary warning followed by one per-feature warning of the form *"Phase Feature N created M warnings."*
> **Root cause:** Intentional UX change in SIMPLNX — see PR #1455 (issue #1262). The per-feature counts were not actionable for users (the specific offending cells were not reported), and the set-based representation is lower-memory and faster.
> **Affected users:** Anyone whose downstream tooling parses warning streams from this filter; anyone counting warnings as a quality metric.
> **Recommendation:** Trust SIMPLNX. The numeric output (`FeaturePhases`) is identical between versions — only the warning text/count differs. No legacy patch needed unless a user has automation that parses the legacy per-feature warning lines.
> **Status:** Proposed — pending verification that 6.5.172 actually emits the per-feature warnings (run the comparison).

> **Deviation ID:** `ComputeFeaturePhases-D2` *(speculative — list only if legacy disagrees)*
> **Filter UUID:** `da5bb20e-4a8e-49d9-9434-fbab7bc434fc`
> **Symptom:** *(none expected — placeholder.)* If `compare-legacy-dream3d` finds any numeric difference in `FeaturePhases` between SIMPLNX and 6.5.172 on the same input, file it here. None is expected because the algorithm is a one-pass voxel scan with no floating-point math.
> **Status:** Placeholder — delete if legacy comparison shows full numeric agreement.
