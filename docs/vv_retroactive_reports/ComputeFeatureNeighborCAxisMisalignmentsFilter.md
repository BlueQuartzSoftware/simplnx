# Retroactive V&V: ComputeFeatureNeighborCAxisMisalignmentsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

> **TOP-PRIORITY ALERT — REPLICATED BUG PATTERN:** The c-axis sibling has the same divisor-bug pattern that the audit flagged in `ComputeFeatureNeighborMisorientationsFilter`. In `ComputeFeatureNeighborCAxisMisalignments::operator()()`, line 111 reassigns `hexNeighborListSize = currentNeighborList.size();` on **every** iteration of the inner j-loop, so the `hexNeighborListSize--;` decrement on line 150 (taken when a neighbor's phase does not match a hex pair) is overwritten by the next j-iteration. After the j-loop the divisor on line 162 (`avgCAxisMisalignmentPtr / hexNeighborListSize`) reflects only the LAST iteration's state — `currentNeighborList.size()` if the last neighbor was a hex match, or `currentNeighborList.size() - 1` if the last neighbor was a non-hex skip. This produces a wrong "Feature Average C-Axis Misalignment" whenever the feature has any non-hex neighbors that are not exclusively at the very end of the list. **See "Recommended Deviation entries" → `ComputeFeatureNeighborCAxisMisalignments-D1` for details.** This is a SUSPECTED REAL BUG that is **production-relevant** because the prebuilt `EBSD_Hexagonal_Data_Analysis.d3dpipeline` uses `find_avg_misals: true`.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| SIMPLNX ClassName | `ComputeFeatureNeighborCAxisMisalignmentsFilter` |
| SIMPLNX Human Name | Compute Feature Neighbor C-Axis Misalignments |
| SIMPL UUID (legacy) | `cdd50b83-ea09-5499-b008-4b253cf4c246` (preserved in `// LEGACY UUID` comment in .hpp) |
| SIMPL ClassName | *(TBD — confirm in legacy SIMPL repo; almost certainly `FindFeatureNeighborCAxisMisalignments`)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureNeighborCAxisMisalignmentsFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeFeatureNeighborCAxisMisalignmentsTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeFeatureNeighborCAxisMisalignmentsFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeFeatureNeighborCAxisMisalignmentsFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`
- `src/Plugins/OrientationAnalysis/pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` (production caller)

## Algorithm Relationship

- **Tentative classification:** **Port (with reviewed cleanup)** — the SIMPLNX UUID is new (`636ee030-…`) but the legacy SIMPL UUID is preserved in the header comment, indicating a deliberate port from the SIMPL `FindFeatureNeighborCAxisMisalignments` filter. PR #1467 explicitly says "reviewed and signed off by OEMs" and PR #1438 carries multiple correctness adjustments to the same file as part of the Microtexture cleanup pass.
- **Evidence:** Algorithm class file already lives outside `not_used/`. UUID-preserving comment present in .hpp. PR #1438 + PR #1467 are explicit code-review events. PR #1467 also rewrote the unit test from scratch.
- **Action required:** Confirm by opening the corresponding SIMPL filter source (likely `FindFeatureNeighborCAxisMisalignments.cpp`) and running `compare-legacy-dream3d` (Step 0 e) against a hex-phase toy dataset. Specifically check whether the divisor bug described in the alert above is also present in legacy DREAM.3D 6.5.171 — if it is, this becomes a "both versions wrong, NX cleaner refactor preserved the bug" case. The doc currently says *"Results from this filter can differ from its original version in DREAM.3D 6.5.171 by around 0.0001"* which suggests the developer did at least one informal numerical comparison but did not flag the divisor problem.

## PRs inspected (since 2025-10-01)

> Note: a SIMPL Backwards Compatibility Test (#1588) and a one-line doc subgroup typo fix (#1547) are listed individually; pure-style/repo-wide PRs (#1457 static-inline, #1582 cancel-check sweep, #1538 zlib extraction, #1474 MSVC warning, #1439 NeighborList multidim) are summarized in the Pruned table.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25 (broad refactor, exception flagged because it carried a real bug fix and behavior changes for THIS filter)

- **Files in this filter:** algorithm (.cpp +/- ~58 lines), algorithm (.hpp -2), filter (.cpp +/- ~14 lines)
- **Diff size (this filter only):** 3 files, +36 / -38
- **Change nature:** Multiple substantive changes bundled into the broad Microtexture pass:
  1. **Real bug fix:** *"Fixes ComputeFeatureNeighborCAxisAlignments crash if 'Find Avg Misalignments' was not enabled."* The fix replaced an unconditional `getDataRefAs<Float32Array>(AvgCAxisMisalignmentsArrayName)` with a conditional `getDataAs<Float32Array>` guarded by `if(m_InputValues->FindAvgMisals)`. Before this PR, calling the filter with `FindAvgMisals == false` would throw because the avg array was never created in preflight.
  2. **Variable renames:** `phase1/phase2` → `xtalPhase1/xtalPhase2`, `i` → `featureIdx`. Cosmetic but pervasive — affects readability of any subsequent diff.
  3. **Error/warning text rewritten:** The `-1562` and `-1563` messages were shortened. Behavior unchanged but log output for users differs.
  4. **Preflight UX:** moved a hex-symmetry warning from `resultOutputActions.warnings()` (a real warning) to `preflightUpdatedValues` (an info banner shown next to the parameter UI). User-facing behavior change — silent in pipeline mode now.
  5. **Parameter labels and default array names changed:** `"C-Axis Misalignment List"` → `"Feature C-Axis Misalignment NeighborList"`, default name `AvgCAxisMisalignments` → `AvgNeighborCAxisMisalignments`. **This is a default-value change that downstream consumers may not have anticipated.**
- **V&V content:** **High** — fixes a crash. Also introduces small but real behavioral changes (warning channel, default array name) that should be Deviation candidates vs. SIMPL.

### PR #1467 — *"REV: ComputeFeatureNeighborCAxisMisalignment reviewed and updated"* — merged 2025-11-12

- **Files in this filter:** algorithm (.cpp ~+69 lines, ~-69), test (.cpp +111 / -96 — completely rewritten), docs (.md ~+10 / -10), test/CMakeLists.txt (+1)
- **Diff size:** 5 files, +105 / -96 (this filter only)
- **PR description:** *"Code was reviewed and signed off by OEMs. Unit test was updated with a completely new unit test file."*
- **Change nature:** **Explicit code-review pass + new exemplar test.** This is the closest thing to a per-filter V&V event in the entire history. Concrete changes:
  1. Added comment headers ("Validate any Crystal Structure issues early...", "Get references to all the input data", "Loop over every feature", etc.).
  2. Renamed `nName` → `neighborFeatureId` for clarity.
  3. Changed `misalignmentLists` from `vector<vector<float>>` → `vector<vector<double>>` (later reverted in PR #1474 for MSVC). **Precision-relevant.**
  4. Replaced `(Constants::k_PiD / 2.0)` literal with `Constants::k_PiOver2D` constant. Behaviorally identical.
  5. Restructured how `cAxisMisalignmentList` is populated: previously `setLists(misalignmentLists)` was called once after the outer loop; now `setList(featureIdx, ...)` is called per feature inside the loop. The semantic should be the same but the timing changes (per-feature vs. bulk write).
  6. Added a registration of new exemplar archive `compute_feature_neighbor_caxis_misalignments.tar.gz` to `test/CMakeLists.txt`.
  7. **The new test** uses a single Hex exemplar dataset (`7_5_simplnx_test_file_25x50_Hex.dream3d`) and compares both the per-feature `CAxisMisalignmentList` and the per-feature `AvgCAxisMisalignments` against `(7_5)`-suffixed exemplars. This is the FIRST test that exercises `find_avg_misals: true`, and it does so against a hex-phase-only dataset — meaning **the divisor bug described in the alert at the top of this report is NOT exercised** because every neighbor pair is a hex match (so the `else` branch where `hexNeighborListSize--` is reached never runs).
- **V&V content:** **Medium-high process value, but the test is a coverage trap.** The "OEM signed off" claim covers the file as it stood after this PR, yet the divisor-reset bug from the legacy implementation was preserved verbatim. The new test cannot fail on it because the exemplar dataset has no mixed-phase neighbors.
- **Deviation candidate:** Whatever PR #1467 changed about per-vs-bulk `setList(s)` calls and `float`→`double` precision should be cross-checked against legacy 6.5.171 output via `compare-legacy-dream3d`.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 (broad refactor, exception flagged because it changed math-relevant code paths even if API-equivalent)

- **Files in this filter:** algorithm (.cpp ~+12 / -11)
- **Change nature:** Replaced `OrientationTransformation::qu2om<QuatD, OrientationD>(...)` with `ebsdlib::QuaternionDType(...).toOrientationMatrix()`, replaced `OrientationMatrixToGMatrixTranspose(oMatrix)` with `oMatrix.transpose()`, swapped include style from `"EbsdLib/..."` to `<EbsdLib/...>`, and changed `EbsdLib::CrystalStructure::...` → `ebsdlib::CrystalStructure::...`.
- **V&V content:** API rename on its face, but **two of the renames touch the math path:**
  - The Q→OM conversion route changed callees. Numerical equivalence with the prior `qu2om` template needs explicit confirmation.
  - The g-matrix transpose now uses `Eigen::Matrix::transpose()` directly. The prior `OrientationMatrixToGMatrixTranspose(oMatrix)` was a free function that may have done extra reshape work — needs confirmation that it was a pure transpose and nothing else.
- **Deviation candidate:** Possible numerical drift vs. legacy. The doc note about "differ from 6.5.171 by around 0.0001" predates this PR — the drift may have grown after PR #1472 and not been re-measured.

### PR #1547 — *"DOC: Fix filter documentation and documentation related code bugs"* — merged 2026-03-10

- **Files in this filter:** docs (.md), 1 line changed
- **Diff size:** +1 / -1
- **Change nature:** `Statistics (Crystallographic)` → `Statistics (Crystallography)` in the Group/Subgroup line. Cosmetic.
- **V&V content:** None.

### PR #1582 — *"ENH: Add missing cancel checks to lots of filters"* — merged 2026-04-08 (broad refactor, exception flagged because the cancel-check insertion is correctness-relevant for long-running pipelines)

- **Files in this filter:** algorithm (.cpp +5 lines)
- **Change nature:** Added `if(m_ShouldCancel) { return {}; }` at the top of the outer feature loop.
- **V&V content:** Pure cancel-handling. The early-return uses `return {};` which yields a `Result<>::valid()` with no error — pipeline downstream filters will continue to execute against partially-computed output. That's the project convention. Not a bug, but worth noting for completeness.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp +51 lines), plus two new fixture files
  - `test/simpl_conversion/6_5/ComputeFeatureNeighborCAxisMisalignmentsFilter.json` (~36 lines)
  - `test/simpl_conversion/6_4/ComputeFeatureNeighborCAxisMisalignmentsFilter.json` (~35 lines)
- **Change nature:** **Test addition.** Per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both the SIMPL 6.5 (UUID-mapped) and SIMPL 6.4 (Filter_Name fallback) pipeline conversion paths via `DYNAMIC_SECTION`. Asserts that all seven parameters (including `k_FindAvgMisals_Key == true`) survive the conversion.
- **V&V content:** **Pipeline-conversion correctness only.** Does not exercise `executeImpl` against legacy data. The fixture for 6.4 only checks JSON-key mapping, not data-path resolution.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API/include change in filter .cpp; one-line edit, no behavior change for this filter |
| #1457 | Clean up 'static inline' from filter headers | Pure header style |
| #1474 | Fix MSVC template warnings | Reverted PR #1467's `vector<vector<double>>` back to `vector<vector<float>>` and added `static_cast<float32>` on the degree assignment. **Precision regression vs. PR #1467** but consistent with original SIMPL behavior, so likely benign for legacy parity. Worth noting in summary. |

## Test coverage detected

`ComputeFeatureNeighborCAxisMisalignmentsTest.cpp` contains 2 `TEST_CASE`s:

1. `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: Valid Filter Execution` — Loads `7_5_simplnx_test_file_25x50_Hex.dream3d` exemplar, runs the filter with `find_avg_misals = true`, compares both `CAxisMisalignmentList` and `AvgCAxisMisalignments` against `(7_5)`-suffixed exemplars using `CompareNeighborListFloatArraysWithNans` and `CompareFloatArraysWithNans`. *(Added/rewritten by PR #1467)*
2. `OrientationAnalysis::ComputeFeatureNeighborCAxisMisalignmentsFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`. *(Added by PR #1588)*

**Coverage gaps detected:**
- **Mixed-phase coverage MISSING.** The exemplar is hex-phase-only ("25x50_Hex"), so the `else` branch (`hexNeighborListSize--`, NaN-write) is never executed. The divisor bug described in the alert is therefore invisible to the test suite.
- **`find_avg_misals = false` path NOT tested.** Only the `true` branch has a test. The crash that PR #1438 fixed (false-branch null-deref) would not be caught by regression.
- **No-hex-phase error path (`-1562`) NOT tested.** No test exercises the early bail-out.
- **Mixed hex/cubic warning path (`-1563`) NOT tested.** No test exercises the warning issuance and NaN-fill behavior.

## Exemplar archive

- **Archive name:** `compute_feature_neighbor_caxis_misalignments.tar.gz`
- **SHA512:** `955cd35b7ae24579ef9c533df34e1118012a8e5e2a71f8613117c714fc220c5dfa78d91a2964b41752e70684b79d4aa790e488e9a7be4c9dcf7b642ee2897ceb`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` line 141 *(added by PR #1467)*
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any inner ReadMe.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. The exemplars are suffixed `(7_5)` which suggests they were generated from a SIMPL 6.5.171 pipeline run — confirm. Promote contents into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class:** **Class 4 (Invariant-based)** + opportunistic **Class 1 (Analytical)** spot checks. Class 3 (Paper-based) is *not* recommended unless a paper reference can be located — the source files contain no DOI or paper citation, and the docs only reference the EBSD_Hexagonal_Data_Analysis pipeline.
- **Rationale (Class 4 — Invariants):**
  1. All output values in `CAxisMisalignmentList` and `AvgCAxisMisalignments` for hex-matching neighbor pairs must lie in `[0°, 90°]` (because the algorithm folds via `if(w > pi/2) w = pi - w;`).
  2. `misalign(i, j) == misalign(j, i)` to within float epsilon — the per-feature NeighborList output is an undirected adjacency, so symmetry across the two corresponding entries should hold.
  3. For two features with parallel c-axes, `misalign == 0°`.
  4. Non-hex neighbor entries must be NaN.
  5. When `find_avg_misals == true`, the average for each feature should equal the arithmetic mean of the non-NaN entries in that feature's neighbor list. **This invariant is the test that would expose the divisor bug.**
- **Rationale (Class 1 — Analytical spot checks):**
  - Identity-c-axis pair: feature with `q = (1, 0, 0, 0)` and a neighbor with the same quaternion → `misalign == 0°`.
  - Orthogonal c-axes: feature with c-axis along [001] and neighbor with c-axis along [100] → `misalign == 90°`.
  - Antipodal c-axes: feature with c-axis along [001] and neighbor with c-axis along [00-1] → `misalign == 0°` (because the algorithm uses `|c1·c2|` semantics via the `pi - w` fold).
- **Action required:** Developer to confirm whether a paper reference exists in the algorithm header or legacy DREAM3D docs (none was found by `grep`). Defend or replace.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | **Yes — PR #1467, but incomplete** | OEMs signed off on a version that retained the divisor-reset bug. The review focused on naming, comments, and structure — not on the inner-loop divisor invariant. |
| Code path coverage (algorithmic) | **Partial** | One exemplar test exercises `find_avg_misals = true` against an all-hex dataset. Mixed-phase, false-branch, and error/warning paths are untested. |
| Code path coverage (SIMPL conversion) | **Yes** | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `compute_feature_neighbor_caxis_misalignments.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | **Unknown** | TBD by inspecting archive contents. The `(7_5)` suffix on exemplar arrays suggests SIMPL-generated reference. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | **No** | No script or hand-derivation on file. The four Class-1 analytical spot checks above are recommended. |
| Legacy comparison report (Step 0 e) | **Implicit only** | The doc says "differ from 6.5.171 by around 0.0001" which implies an informal comparison was done but no `compare-legacy-dream3d` artifact is on file. |
| Deviation entries (`ComputeFeatureNeighborCAxisMisalignments-D<N>`) | **None** | Not yet written. **Multiple strong candidates below.** |
| Documentation currency | **Probably current** | Updated by PR #1438 (parameter renames) and PR #1547 (subgroup typo). The "Notes" section about `Hexagonal_High` is incomplete — the algorithm also accepts `Hexagonal_Low`. |
| Verification archive (OneDrive) | **No** | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **(a) Confirm the oracle.** Class 4 (invariant-based) is the recommended starting point with Class 1 spot checks. Defend or replace.
2. **(b) Promote tests to oracle assertions.** Add explicit Class-4 invariant assertions to the existing test:
   - For every per-feature average, recompute from the NeighborList and `REQUIRE` it equals the stored average. This single check would fail today on the divisor bug — assuming a mixed-phase dataset is used.
   - Add `REQUIRE` that all non-NaN entries lie in `[0°, 90°]`.
   - Add a NaN-count check based on phase-mismatch count.
3. **(c) Build a mixed-phase toy dataset.** Construct (or load) a small synthetic case with 3 features: one hex, one hex, one cubic — connected as a triangle in the neighbor list. Hand-derive the expected per-feature avg. This is the smallest dataset that will demonstrate the divisor bug.
4. **(d) Add tests for the missing code paths.** `find_avg_misals = false`, no-hex-phase error path (`-1562`), mixed hex/non-hex warning path (`-1563`).
5. **(e) Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.171 on (i) the existing all-hex exemplar and (ii) the new mixed-phase toy. Expected outcomes:
   - The all-hex case should reproduce the doc's "0.0001" drift claim and confirm whether PR #1472 (EbsdLib 2.0 API rename) increased that drift.
   - The mixed-phase case will tell us whether legacy 6.5.171 has the same divisor bug or not — which determines whether `D1` is "trust SIMPLNX" or "both wrong, fix both."
6. **Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `FindFeatureNeighborCAxisMisalignments` filter (legacy UUID `cdd50b83-…` preserved). Reviewed in PR #1467. PR #1438 fixed a `find_avg_misals = false` crash and renamed default output array names. PR #1472 swapped the EbsdLib quat-to-orientation-matrix call. The legacy divisor-reset bug appears to have been preserved through all of these."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D1`
> **Filter UUID:** `636ee030-9f07-4f16-a4f3-592eff8ef1ee`
> **Severity:** **HIGH — suspected real bug; production-relevant.**
> **Symptom:** When `find_avg_misals == true` AND a feature has neighbors that are NOT hex/hex pairs, the per-feature `AvgCAxisMisalignments[featureIdx]` is computed as `(sum of hex-pair angles) / divisor`, where `divisor` reflects only the LAST inner-loop iteration's value of `hexNeighborListSize` rather than the running decremented count. Specifically, `divisor == currentNeighborList.size()` if the last neighbor was a hex match, or `divisor == currentNeighborList.size() - 1` if the last neighbor was a non-hex skip. The numerator only accumulated hex-match contributions, so the average is biased low (divisor too large) in most cases.
> **Root cause:** `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp` line 111 reassigns `hexNeighborListSize = currentNeighborList.size();` on EVERY iteration of the inner j-loop, overwriting the `hexNeighborListSize--;` decrement on line 150. The reassignment should be moved to BEFORE the j-loop so the decrement is preserved across iterations.
> **Replication of sibling bug:** This is structurally identical to the suspected bug in `ComputeFeatureNeighborMisorientations.cpp` line 75 (`tempMisoList = featureNeighborList.size();`). Both filters carry the bug in the same place, the same way; the c-axis version was reviewed by OEMs in PR #1467 and the bug was not caught.
> **Why the existing test does not catch it:** The exemplar dataset (`7_5_simplnx_test_file_25x50_Hex.dream3d`) is hex-only, so the `else` branch (`hexNeighborListSize--`) is never reached.
> **Affected users:** Anyone running `EBSD_Hexagonal_Data_Analysis.d3dpipeline` (which sets `find_avg_misals: true`) on data with mixed phases. Anyone whose downstream microtexture analysis consumes `AvgCAxisMisalignments` on multi-phase samples.
> **Recommendation:**
>   1. **Fix the algorithm:** Move `hexNeighborListSize = currentNeighborList.size();` to be set ONCE before the j-loop, not inside it. Apply the equivalent fix to `ComputeFeatureNeighborMisorientations.cpp`.
>   2. Add a mixed-phase test exemplar that exercises the decrement.
>   3. Add an invariant assertion in the test: recompute the per-feature average from the NeighborList and `REQUIRE` agreement with the stored average.
>   4. Run `compare-legacy-dream3d` against SIMPL 6.5.171 to determine whether legacy has the same bug. If yes, this is a "both wrong, fix both" Deviation. If no, legacy was right and the SIMPLNX port introduced a regression.
> **Status:** Proposed — pending fix and pending legacy comparison.

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D2`
> **Filter UUID:** `636ee030-9f07-4f16-a4f3-592eff8ef1ee`
> **Severity:** Medium — possible correctness issue separate from D1.
> **Symptom:** The "Feature Average C-Axis Misalignments" output array is created in preflight via `CreateArrayAction(DataType::float32, ..., {1}, ...)` with no `fillValue` argument. Inside the algorithm, the FIRST hex-match write per feature does `avgCAxisMisalignmentPtr->getValue(featureIdx) + currentMisalignmentList[j]`, which reads the array's pre-write value. If `CreateArrayAction` does not zero-initialize when `fillValue` is empty (the underlying DataStore default depends on the IOCollection's `createDataStoreWithType` implementation), the accumulator starts from undefined or `mudflap` state.
> **Root cause:** `ComputeFeatureNeighborCAxisMisalignmentsFilter.cpp` line 125-127 does not pass an explicit fill value. `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp` line 142-143 assumes the array starts at zero.
> **Recommendation:** Pass `"0"` as the fillValue argument to `CreateArrayAction`, OR explicitly zero the array at the top of `operator()()` when `FindAvgMisals` is true. Confirm by inspection of `DataStoreUtilities::CreateDataStore`'s default-init behavior whether this is currently a real bug or a latent one.
> **Status:** Proposed — pending confirmation of DataStore default-init semantics.

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D3`
> **Filter UUID:** `636ee030-9f07-4f16-a4f3-592eff8ef1ee`
> **Severity:** Low — user-facing change vs. SIMPL.
> **Symptom:** Default output array names changed in PR #1438:
>   - `"AvgCAxisMisalignments"` → `"AvgNeighborCAxisMisalignments"`
>   - Parameter labels reworded from `"C-Axis Misalignment List"` / `"Average C-Axis Misalignments"` → `"Feature C-Axis Misalignment NeighborList"` / `"Feature Average C-Axis Misalignments"`
> **Root cause:** Intentional rename in PR #1438.
> **Affected users:** Anyone whose downstream pipeline references the old default name. Existing pipeline files in the repo were re-saved with the new name; user-saved pipelines still reference the old name and will produce arrays with the new name.
> **Recommendation:** Document the rename in release notes / migration guide. The SIMPL backwards-compatibility test (PR #1588) covers the conversion path but uses generic `"TestName"` placeholders, so it does not enforce that the OLD default name still works.
> **Status:** Proposed — confirm legacy default name in SIMPL repo.

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D4`
> **Filter UUID:** `636ee030-9f07-4f16-a4f3-592eff8ef1ee`
> **Severity:** Low — possible numerical drift.
> **Symptom:** PR #1472 swapped two pieces of orientation math:
>   - Quaternion → orientation matrix: `OrientationTransformation::qu2om<QuatD, OrientationD>` → `ebsdlib::QuaternionDType(...).toOrientationMatrix()`
>   - G-matrix transpose: `OrientationMatrixToGMatrixTranspose(oMatrix)` → `oMatrix.transpose()`
> **Recommendation:** Numerically diff per-feature avg c-axis misalignment values against pre-#1472 commit on the existing exemplar. The doc's "0.0001" drift note predates this PR; verify drift has not grown. Likely benign but should be confirmed.
> **Status:** Proposed — pending bisection.

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D5`
> **Filter UUID:** `636ee030-9f07-4f16-a4f3-592eff8ef1ee`
> **Severity:** Low — UX change.
> **Symptom:** PR #1438 moved the hex-symmetry crystal-structure warning from `resultOutputActions.warnings()` (a real warning that surfaces in pipeline logs / CLI output) to `preflightUpdatedValues` (an info banner that displays only in the GUI parameter panel). Pipeline-mode users no longer see this warning at all.
> **Recommendation:** Restore the `warnings()` push or document the deliberate UX choice. Either is acceptable; the audit only flags the silent change.
> **Status:** Proposed — defer to UX policy.
