# V&V Working Document: ComputeFeatureNeighborCAxisMisalignmentsFilter

*Status:* **DRAFT — Phase 1 (Discovery) complete; Phases 2–13 are work orders for the engineer to execute.**

> **🚨 Production-relevant bug pre-confirmed.** This filter contains a divisor-bug (Phase 1 §"Algorithm summary" → "Pre-confirmed bug") that ships in `EBSD_Hexagonal_Data_Analysis.d3dpipeline` with `find_avg_misals: true`. The bug is a copy-paste from sibling `ComputeFeatureNeighborMisorientations` and was triaged in `docs/vv_bug_triage_reports/bug_triage.md` (Bug #3). Fix lands in **Phase 8** of this V&V; **bundle the PR with the sibling fix** per Phase 13.

This document is the engineer's working V&V doc. **Retroactive promotion pass** — the audit's tentative report at [`docs/vv_retroactive_reports/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`](../../../../docs/vv_retroactive_reports/ComputeFeatureNeighborCAxisMisalignmentsFilter.md) supplied most of the content below.

**How to use this document:**
- Each phase has a **Goal**, **Tasks** (checkboxes), and **Exit criteria**. Phase 1 is the canonical source for discovery context; later phases reference Phase 1 rather than restate.
- Where a "starting point" is provided, you can confirm it or replace it.
- Update this document as you work. Phase 13 updates the Status line at the top to "COMPLETE"; the file stays at this location.

**Source-of-truth references** (paths relative to this doc at `src/Plugins/OrientationAnalysis/vv/`):
- Policy: [`docs/vv_templates/mtr_filter_verification_validation.md`](../../../../docs/vv_templates/mtr_filter_verification_validation.md)
- Oracle quick-reference: [`docs/vv_templates/oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md)
- Audit cross-cutting findings: [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md) "Cross-cutting findings"
- Bug triage detail: [`docs/vv_bug_triage_reports/bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md) "Bug 3"
- Retroactive report: [`docs/vv_retroactive_reports/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`](../../../../docs/vv_retroactive_reports/ComputeFeatureNeighborCAxisMisalignmentsFilter.md)
- Skill: `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`

---

## Phase 1 — Discovery *(complete; canonical source for all later phases)*

**Goal**: locate every file related to this filter and identify the legacy DREAM3D equivalent.

### Tasks (verify findings)

- [ ] Confirm metadata, source files, exemplar archive, PR list, test inventory, algorithm summary, and pre-confirmed bug below are accurate
- [ ] Re-run `git log --since=2025-10-01 --oneline -- <Phase-1 files>` to check no new PRs landed since this doc was generated

### Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `636ee030-9f07-4f16-a4f3-592eff8ef1ee` |
| SIMPLNX ClassName | `ComputeFeatureNeighborCAxisMisalignmentsFilter` |
| Human Name | Compute Feature Neighbor C-Axis Misalignments |
| SIMPL UUID | `cdd50b83-ea09-5499-b008-4b253cf4c246` (preserved as `// LEGACY UUID` comment in `.hpp`) |
| SIMPL ClassName | `FindFeatureNeighborCAxisMisalignments` *(confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |
| Tier | Tier-1 (MTR SBIR list) |
| Mode | Retroactive promotion (retroactive report exists) |
| Sibling | `ComputeFeatureNeighborMisorientationsFilter` — same divisor-bug pattern; **bundle fixes** |

### Source files

| Path (relative to `src/Plugins/OrientationAnalysis/`) | Notes |
|---|---|
| `src/OrientationAnalysis/Filters/ComputeFeatureNeighborCAxisMisalignmentsFilter.{hpp,cpp}` | Filter |
| `src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.{hpp,cpp}` | Algorithm |
| `test/ComputeFeatureNeighborCAxisMisalignmentsTest.cpp` | 2 TEST_CASEs |
| `test/simpl_conversion/6_4/ComputeFeatureNeighborCAxisMisalignmentsFilter.json` | 6.4 conversion fixture |
| `test/simpl_conversion/6_5/ComputeFeatureNeighborCAxisMisalignmentsFilter.json` | 6.5 conversion fixture |
| `docs/ComputeFeatureNeighborCAxisMisalignmentsFilter.md` | User doc |
| `pipelines/EBSD_File_Processing/EBSD_Hexagonal_Data_Analysis.d3dpipeline` | **Production caller** — sets `find_avg_misals: true` |

### Test exemplar archive

- **Archive**: `compute_feature_neighbor_caxis_misalignments.tar.gz`
- **SHA512**: `955cd35b7ae24579ef9c533df34e1118012a8e5e2a71f8613117c714fc220c5dfa78d91a2964b41752e70684b79d4aa790e488e9a7be4c9dcf7b642ee2897ceb`
- **Referenced in**: `test/CMakeLists.txt` line ~141 (added by PR #1467)
- **Source file used**: `7_5_simplnx_test_file_25x50_Hex.dream3d` — **hex-phase-only**. Exemplar arrays for `CAxisMisalignmentList` and `AvgCAxisMisalignments` are stored with `(7_5)` suffix suggesting SIMPL 6.5.171-generated reference.
- **Provenance**: not yet documented. Phase 2 investigates.
- **Coverage gap**: hex-only exemplar means Test 1 cannot exercise the divisor bug or the mixed-phase NaN path.

### Material PRs since 2025-10-01

8 material PRs. The filter has been heavily worked on but the divisor bug survived all of them — including PR #1467's explicit OEM code review.

| PR | Date | Title | Effect on this filter |
|---|---|---|---|
| #1438 | 2025-10-25 | ENH: Microtexture related filter cleanup | **Fixed a crash** in the `find_avg_misals=false` branch (unconditional `getDataRefAs` → conditional `getDataAs`). Renamed default array `AvgCAxisMisalignments` → `AvgNeighborCAxisMisalignments` (D3). Moved hex-symmetry warning from `warnings()` to `preflightUpdatedValues` info banner (D5). Variable renames `phase1`→`xtalPhase1`, `i`→`featureIdx`. |
| #1467 | 2025-11-12 | REV: ComputeFeatureNeighborCAxisMisalignment reviewed and updated | **Explicit code review by OEMs.** Test rewritten from scratch using hex-only exemplar. Restructured `setLists` (bulk) → `setList` per-feature inside loop. Added comment headers. **Did NOT catch the divisor bug** — review focused on naming/structure. |
| #1472 | 2025-11-24 | ENH: Update to EbsdLib 2.0.0 API | **Promoted from prune list** because this filter delegates math to EbsdLib. Two math-relevant call swaps: `OrientationTransformation::qu2om` → `Quaternion::toOrientationMatrix()`; `OrientationMatrixToGMatrixTranspose` → `Matrix::transpose()`. May cause numerical drift (D4). |
| #1474 | 2026-01-08 | COMP: Fix MSVC template warnings | **Reverted PR #1467's `vector<vector<double>>` back to `<float>`** with `static_cast<float32>` on the degree assignment. Precision regression vs. #1467 but consistent with original SIMPL behavior. |
| #1547 | 2026-03-10 | DOC: Fix filter documentation … | One-line subgroup typo (`Crystallographic` → `Crystallography`). |
| #1582 | 2026-04-08 | ENH: Add missing cancel checks to lots of filters | Added `if(m_ShouldCancel) { return {}; }` at top of outer feature loop. |
| #1588 | 2026-04-22 | ENH: SIMPL Backwards Compatibility Test Redesign | Added per-filter SIMPL conversion test + 6.4 + 6.5 fixture JSONs. Tests parameter mapping, not output. |
| #1457, #1439 | various | (style cleanup, NeighborList multidim API) | Pruned — pure mechanical edits |

### TEST_CASE inventory

`ComputeFeatureNeighborCAxisMisalignmentsTest.cpp` contains 2 `TEST_CASE`s:

| # | Name | Coverage | Encodes oracle? |
|---|---|---|---|
| 1 | `Valid Filter Execution` | Exemplar comparison on hex-only dataset (`find_avg_misals=true` only) | Class 5 (legacy-agreement) |
| 2 | `SIMPL Backwards Compatibility` | 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` | Conversion-only |

⚠️ **Coverage gaps that matter for this V&V**:
- **Mixed-phase coverage MISSING** — divisor bug invisible to current tests
- **`find_avg_misals = false` branch UNTESTED** — the crash PR #1438 fixed has no regression pin
- **Error path `-1562` (no hex phase) UNTESTED**
- **Warning path `-1563` (mixed hex/non-hex) UNTESTED**

### Algorithm summary

For each feature, walks its `NeighborList`. For each (feature, neighbor) pair where both phases are `Hexagonal_High` or `Hexagonal_Low`: rotates the c-axis [0,0,1] by each feature's quaternion (via EbsdLib `LaueOps::calculateMisorientation` → axis-angle), computes the angle between the two c-axes, folds to `[0°, 90°]` via `if(w > pi/2) w = pi - w;`. For non-hex pairs: writes NaN to the per-feature NeighborList<float32> and decrements a "hex neighbor count" used as the average's divisor.

- **Output arrays**: per-feature `NeighborList<float32>` of misalignment angles (always); per-feature `Float32Array` of averages (only if `FindAvgMisals = true`)
- **Restrictions**: only `Hexagonal_High` / `Hexagonal_Low` Laue groups; mixed-phase neighbor → NaN
- **Threading**: single-threaded; cancel check at top of outer feature loop (PR #1582); `ThrottledMessenger` not used (no progress messaging — gap)
- **EbsdLib delegation**: `LaueOps::calculateMisorientation`. PR #1472 reformulated the qu→om path through a different EbsdLib API; possible numerical drift (D4)

#### Pre-confirmed bug — divisor-reset (D1, HIGH severity)

- **File**: `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp`
- **Bug**: line 111 — `hexNeighborListSize = currentNeighborList.size();` is reassigned every iteration of the inner j-loop, **clobbering** the `hexNeighborListSize--;` decrement on line 150 (taken when a neighbor is non-hex)
- **Effect**: divisor on line 162 reflects only the last j-iteration: `currentNeighborList.size()` if last neighbor was a hex match, else `currentNeighborList.size() - 1`. Earlier mismatches silently ignored. Average is biased low whenever any non-hex neighbor exists.
- **Production impact**: ships in `EBSD_Hexagonal_Data_Analysis.d3dpipeline` with `find_avg_misals: true`. Hex-only exemplar means current tests cannot trigger.
- **Sibling**: `ComputeFeatureNeighborMisorientations.cpp` line 75 — same shape, same fix
- **Triage**: see [`docs/vv_bug_triage_reports/bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md) Bug 3 for failing test code + unified-diff fix

### Pre-flagged deviation candidates (full detail in Phase 9)

| ID | Severity | One-line |
|---|---|---|
| D1 | **HIGH** | Divisor reset bug — see "Pre-confirmed bug" above |
| D2 | Medium | `AvgCAxisMisalignments` array possibly not zero-initialized before accumulator-style writes |
| D3 | Low | Default array name change (`AvgCAxisMisalignments` → `AvgNeighborCAxisMisalignments`) per PR #1438 |
| D4 | Low | Possible numerical drift from EbsdLib 2.0 quat→om math swap (PR #1472) |
| D5 | Low | Hex-symmetry warning silenced in pipeline mode (moved to GUI-only banner) per PR #1438 |

### Exit criteria

- ✅ Verification checkboxes above checked
- ✅ Any new findings or corrections noted in this section
- ✅ `git log` confirms no new PRs landed since this doc was generated

---

## Phase 2 — Promote existing work product

**Goal**: investigate provenance and decide what to promote, augment, or replace.

### Tasks

- [ ] **Investigate provenance of `compute_feature_neighbor_caxis_misalignments.tar.gz`** (Phase 1 §"Test exemplar archive"):
  - [ ] When and by whom (`git log --all --follow -- src/Plugins/OrientationAnalysis/test/CMakeLists.txt | grep -B2 compute_feature_neighbor_caxis`); audit notes the archive registration was added by PR #1467
  - [ ] Download archive locally; inspect for inner `ReadMe.md`, source pipeline `.d3dpipeline`, input `.dream3d`
  - [ ] **Confirm the `(7_5)`-suffix interpretation**: are the exemplar arrays generated by SIMPL 6.5.171? If yes, this is the legacy oracle and provenance ReadMe is "Class 5 legacy expert".
- [ ] **Search source for paper references**: `grep -E "@(reference|cite)|10\.\d{4}/" src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/{Algorithms/,}ComputeFeatureNeighborCAxisMisalignments*`. Audit found none; confirm.
  > *Found:*
- [ ] **Check OneDrive/Slack/email** for prior `ComputeFeatureNeighborCAxisMisalignments*` or `FindFeatureNeighborCAxisMisalignments*` notes. Note especially any "0.0001" drift report referenced by the user doc.
  > *Found:*
- [ ] **Decide promote/augment/replace** for each pre-existing artifact:

| Artifact | Candidate role | Decision |
|---|---|---|
| `compute_feature_neighbor_caxis_misalignments.tar.gz` (hex-only) | Class 5 (if SIMPL-generated) — but inadequate alone, must be **augmented** with a mixed-phase fixture per Phase 5 | _______________ |
| User doc claim "differ from 6.5.171 by around 0.0001" | Implicit informal Phase 9 result; **promote** by re-running formally in Phase 9 to confirm the drift hasn't grown post-PR-#1472 | _______________ |
| PR #1467 OEM code-review sign-off | Phase 7 algorithm-review evidence; **augment** with the missed divisor-bug analysis | _______________ |

### Exit criteria

- ✅ Archive provenance determined
- ✅ Promotion table filled in
- ✅ OneDrive/Slack/email checked for prior notes (incl. the doc's "0.0001" drift claim)

---

## Phase 3 — Algorithm Relationship classification

**Goal**: classify how SIMPLNX relates to legacy. Opens the Phase 9 report.

### Tasks

- [ ] **Open SIMPL `FindFeatureNeighborCAxisMisalignments.cpp`** in DREAM3D 6.5.x. Compare the algorithm body to SIMPLNX (Phase 1 §"Algorithm summary").
- [ ] **Critical cross-check**: does legacy 6.5.171 have the **same divisor-reset bug** described in Phase 1 §"Pre-confirmed bug"? This determines whether D1 in Phase 9 reads "trust SIMPLNX after fix" or "both versions wrong, fix both."
- [ ] **Inspect PR #1472's scoped diff** for math-equivalence of the EbsdLib API swap (Phase 1 PR table). Run: `git show 413e6fa46 -- src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp`
- [ ] **Verify `FromSIMPLJson()` parameter mapping** in `ComputeFeatureNeighborCAxisMisalignmentsFilter.cpp` against the 6.4 + 6.5 conversion fixtures. PR #1588 confirmed the test exists; verify all 7 parameters survive correctly.
- [ ] **Pick classification**: ☐ Port  ☐ Minor changes  ☐ Rewrite  ☐ New filter
- [ ] **Write the Phase-9 opening line**:
  > *Suggested* — *Algorithm Relationship: **Port** — direct translation of SIMPL `FindFeatureNeighborCAxisMisalignments` (legacy UUID `cdd50b83-…` preserved). Reviewed in PR #1467. PR #1438 fixed a `find_avg_misals=false` crash and renamed default outputs. PR #1472 swapped the EbsdLib quat→om call. The legacy divisor-reset bug appears to have been preserved through all of these. SIMPLNX fixes it in this V&V.*
  >
  > *Final:*

### Exit criteria

- ✅ SIMPL source compared; divisor-bug presence in legacy determined
- ✅ PR #1472 EbsdLib diff inspected for math equivalence
- ✅ Conversion fixture mapping verified
- ✅ Classification chosen; opening line written

---

## Phase 4 — Oracle classification

**Goal**: pick oracle class(es) defining "correct" independently of legacy DREAM3D.

### Tasks

- [ ] **Confirm or replace the recommended stack** below.
- [ ] **Identify second-engineer reviewer** (policy line 39):
  - [ ] Reviewer + date: ___________
  - [ ] OR skip reason (recorded in Phase 12 archive ReadMe):
    > *Skip reason:*

### Recommended starting point

**Class 4 (Invariant) primary** — multiple natural assertions, all directly testable:
1. All non-NaN output values in `CAxisMisalignmentList` and `AvgCAxisMisalignments` lie in `[0°, 90°]` (algorithm folds via `pi - w`)
2. `misalign(i, j) == misalign(j, i)` to within float epsilon (undirected adjacency symmetry)
3. Two features with parallel c-axes → `misalign == 0°`
4. Antipodal c-axes → `misalign == 0°` (algorithm uses `|c1·c2|` semantics)
5. Non-hex neighbor entries → NaN
6. **`AvgCAxisMisalignments[i] == arithmetic mean of non-NaN entries in NeighborList[i]`** — this invariant is the test that exposes D1

**Class 1 (Analytical) supplemental** — hand-derivable spot checks:
- Identity-c-axis pair → `0°`
- Orthogonal c-axes ([001] vs [100]) → `90°`
- 30° tilt about y from [001] → `30°` (per the failing test in bug triage)

**Class 3 (Paper-based)** — *not* recommended unless a citation can be found. No DOI or paper reference found in source by audit; the doc only references the EBSD_Hexagonal_Data_Analysis pipeline.

### Exit criteria

- ✅ Primary + companion class(es) recorded
- ✅ Class 5 justification recorded if applicable (not expected here)
- ✅ Second-engineer reviewer or skip reason recorded

---

## Phase 5 — Toy data design + independent expected output

**Goal**: build minimum-size synthetic fixtures exercising every code path, with expected output derived from the oracle alone (not from DREAM3D runs).

**Critical**: the existing exemplar (Phase 1 §"Test exemplar archive") is hex-only and **cannot** trigger the divisor bug or the mixed-phase paths. New mixed-phase fixtures are mandatory.

### Tasks

- [ ] **Promote the failing test from bug triage** (Bug 3 in [`bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md)) as Fixture A. The triage already specifies: 4-feature DataStructure with `NeighborList[1] = {4 (cubic), 2 (hex), 3 (hex)}`, expected `AvgCAxisMisalignments[1] = 15.0°`, buggy `develop` returns `10.0°`.
- [ ] **Add the missing-coverage fixtures** (Phase 1 §"TEST_CASE inventory" coverage gaps):
  - [ ] Fixture B — `find_avg_misals = false` happy-path (regression pin for the PR #1438 crash fix)
  - [ ] Fixture C — error path `-1562` (no hex phase present)
  - [ ] Fixture D — warning path `-1563` (mixed hex/non-hex with avg)
- [ ] **Add Class-1 analytical spot-checks** from Phase 4 oracle stack:
  - [ ] Fixture E — identity-c-axis pair (0°)
  - [ ] Fixture F — orthogonal c-axes (90°)
  - [ ] Fixture G — antipodal c-axes (0° by `|c1·c2|`)
- [ ] **Add Class-4 invariant fixture** (Phase 4 invariants 1, 2, 5, 6 are the assertion form):
  - [ ] Fixture H — symmetry: build neighbor pair (i,j), assert `misalign(i,j) == misalign(j,i)`
  - [ ] Fixture I — average == mean-of-non-NaN; this invariant subsumes Fixture A's expected and is the cleanest D1 regression pin
- [ ] **Save oracle artifacts** to `src/Plugins/OrientationAnalysis/vv/ComputeFeatureNeighborCAxisMisalignmentsFilter_artifacts/`:
  - [ ] Hand-derivation notes for Fixtures A, E, F, G
  - [ ] Per-fixture expected-output table

### Exit criteria

- ✅ Fixture A (D1 regression-pin from triage) designed
- ✅ Coverage-gap fixtures B, C, D designed
- ✅ Class-1 spot-checks E, F, G designed
- ✅ Class-4 invariant fixtures H, I designed
- ✅ Working artifact folder created
- ✅ Second-engineer review of oracle artifacts (if scheduled in Phase 4)

---

## Phase 6 — SIMPLNX vs oracle reconciliation

**Goal**: confirm SIMPLNX matches the oracle on every Phase-5 fixture. **The divisor bug WILL surface here on Fixture A and Fixture I** — that's expected.

### Tasks

- [ ] **Run the existing test suite** to confirm baseline:
  ```bash
  cd /Users/mjackson/Workspace2/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel
  ctest -R "ComputeFeatureNeighborCAxisMisalignmentsFilter" --verbose
  ```
- [ ] **Run Fixture A (D1 regression pin)** — should FAIL on `develop` (returns `10.0°`, expected `15.0°`). This is the expected failure mode confirming the bug.
- [ ] **Run Fixtures B, C, D, E, F, G, H, I** as they're implemented — record outcomes in the table below.
- [ ] **Apply the Phase-1 §"Pre-confirmed bug" fix** (also detailed in [`bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md) Bug 3): move `hexNeighborListSize = currentNeighborList.size();` from line 111 (inside j-loop) to right after `currentMisalignmentList.resize(currentNeighborList.size(), -1.0);` (just before the j-loop). Delete the line-111 occurrence.
- [ ] **Re-run all fixtures** — every one must pass after the fix.
- [ ] **Re-run the existing exemplar test** (Phase 1 Test 1) — must still pass since the hex-only exemplar doesn't exercise the bug; the fix should be a no-op for it.

### Reconciliation table

| Fixture ID | Expected (oracle) | SIMPLNX before fix | SIMPLNX after fix | Action |
|---|---|---|---|---|
| A — D1 regression pin | `AvgCAxisMisalignments[1] = 15.0°` | `10.0°` (buggy) | `15.0°` ✓ | Bug fix in algorithm |
| B — `find_avg_misals=false` | no avg array; per-element list correct | _______ | _______ | _______ |
| C — `-1562` no-hex error | preflight INVALID | _______ | _______ | _______ |
| D — `-1563` mixed warning | warning + NaN entries | _______ | _______ | _______ |
| E — identity c-axes | `0°` | _______ | _______ | _______ |
| F — orthogonal | `90°` | _______ | _______ | _______ |
| G — antipodal | `0°` | _______ | _______ | _______ |
| H — symmetry invariant | `m(i,j) == m(j,i)` | _______ | _______ | _______ |
| I — avg == mean-of-non-NaN | per-feature equality | _______ | _______ | _______ |
| Existing Test 1 (hex-only exemplar) | matches `(7_5)` exemplars | passes | passes | sanity check |

### Exit criteria

- ✅ All Phase-5 fixtures run; outcomes recorded
- ✅ D1 fix applied; Fixture A passes after fix
- ✅ Existing exemplar test still passes (fix is a no-op for hex-only data)
- ✅ Failing-test → fix → green-test cycle complete for D1

---

## Phase 7 — Algorithm Review

**Goal**: code quality pass on already-correct code (correctness was Phase 6's job).

### Tasks

- [ ] **Invoke `bluequartz-skills:review-algorithm`** with `ComputeFeatureNeighborCAxisMisalignmentsFilter`.
- [ ] **Note that PR #1467 already conducted an explicit OEM code review** (Phase 1 PR table) but missed the divisor bug. The Phase-7 review here is targeted at remaining quality items, not a re-review.
- [ ] **Pre-observed quality items to confirm** beyond the now-fixed D1:
  - [ ] Single-threaded by choice — verify intent (parents are independent, could parallelize)
  - [ ] No progress messaging via `ThrottledMessenger` — large meshes have no feedback (gap)
  - [ ] `AvgCAxisMisalignments` may not be zero-initialized (D2 — confirm by inspecting `CreateArrayAction` default fill behavior; pass explicit `"0"` fill if needed)
  - [ ] `vector<vector<float>>` per PR #1474 — precision regression vs. PR #1467's `<double>`. Acceptable for legacy parity but worth noting.
  - [ ] `pi - w` fold semantics for c-axis equivalence — confirm correct vs. legacy
- [ ] **Address all Critical/Warning findings** from the review (fix or document deferral).

### Exit criteria

- ✅ Review skill complete
- ✅ Critical/Warning findings fixed or deferred with rationale
- ✅ D2 (uninitialized accumulator) confirmed real or latent

---

## Phase 8 — Unit Test Review & Implementation

**Goal**: encode Phase-5 fixtures as TEST_CASEs, land the D1 fix, and pin the regression.

### Tasks

- [ ] **Implement the D1 failing test** as Fixture A from Phase 5 (full Catch2 code is in [`bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md) Bug 3 → "Failing Catch2 test"). It should fail on `develop` and pass after the Phase-6 fix.
- [ ] **Implement Fixtures B–I** as new TEST_CASEs.
- [ ] **Add Class-4 invariant assertions** to the existing Test 1 (Phase 1 §"TEST_CASE inventory") so that even hex-only data exercises the invariants:
  ```cpp
  // For every per-feature average, recompute from NeighborList and REQUIRE equality
  // (this single assertion would have caught D1 if the exemplar had mixed-phase data)
  ```
- [ ] **Wrap every `getDataRefAs<T>()` call in `REQUIRE_NOTHROW()`** per `.claude/CLAUDE.md`. Audit existing test for any unwrapped calls.
- [ ] **Plan/implement gap coverage** via `bluequartz-skills:plan-filter-tests` and `bluequartz-skills:implement-filter-tests` if Phase 1 + new fixtures still leave any code path uncovered.
- [ ] **Dual-build**: per `bluequartz-skills:dual-build-protocol`, in-core and OOC must both pass.
- [ ] **Coordinate with sibling fix**: the failing test for `ComputeFeatureNeighborMisorientations` Bug #2 from [`bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md) uses a structurally-identical 4-feature mixed-phase fixture. Consider extracting a shared test-helper (`UnitTest::BuildFeatureNeighborMixedPhaseDataStructure(...)`) that both filters' tests consume.

### Exit criteria

- ✅ D1 failing test landed; passes after Phase-6 fix
- ✅ Fixtures B–I implemented as TEST_CASEs
- ✅ Class-4 invariant assertions added to existing Test 1
- ✅ All `getDataRefAs<T>()` calls wrapped
- ✅ Dual-build pass
- ✅ Shared test-helper extracted (or decision recorded to keep separate)

---

## Phase 9 — Legacy DREAM3D Comparison (diff explanation)

**Goal**: diff against DREAM3D 6.5.172 and write up differences as user-facing Deviation entries. **Not a correctness check** — that was Phase 6.

### Tasks

- [ ] **Invoke `bluequartz-skills:compare-legacy-dream3d`**. Use Phase-5 Fixtures A, B, D, E, F, G as comparison inputs (covers the divisor case, the conditional-avg case, the mixed-phase case, and three analytical cases).
- [ ] **Open the report with the Phase-3 Algorithm Relationship line.**
- [ ] **Re-measure the user doc's "0.0001" drift claim** (Phase 2 promotion) on the existing exemplar. Confirm whether PR #1472 grew the drift.
- [ ] **For each pre-flagged candidate deviation** (Phase 1 §"Pre-flagged deviation candidates"), populate or retract:

#### D1 — Divisor reset (HIGH, production-relevant)

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D1`
> **Filter UUID:** `636ee030-9f07-4f16-a4f3-592eff8ef1ee`
> **Symptom:** When `find_avg_misals == true` AND a feature has any non-hex neighbors not at the end of its NeighborList, `AvgCAxisMisalignments[featureIdx]` is biased low (divisor too large).
> **Root cause:** Bug — `Algorithms/ComputeFeatureNeighborCAxisMisalignments.cpp` line 111 reassigns the divisor inside the inner j-loop, clobbering the per-non-hex-neighbor decrement on line 150.
> **Affected users:** Anyone running `EBSD_Hexagonal_Data_Analysis.d3dpipeline` on data with mixed phases. Anyone whose downstream microtexture analysis consumes `AvgCAxisMisalignments` on multi-phase samples.
> **Recommendation:** *(populate after legacy comparison)* — likely "trust SIMPLNX after fix" if legacy 6.5.171 also has the bug; "both versions wrong, fix both" otherwise.
> **Sibling:** Same bug in `ComputeFeatureNeighborMisorientations`. Fix in coordinated PR.

#### D2 — Possibly-uninitialized accumulator (Medium)

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D2`
> **Symptom:** *(if observed)* `AvgCAxisMisalignments` first-write-per-feature reads pre-write value; if `CreateArrayAction` doesn't zero-initialize, accumulator starts undefined.
> **Root cause:** `ComputeFeatureNeighborCAxisMisalignmentsFilter.cpp` does not pass an explicit `fillValue` to `CreateArrayAction`. Algorithm assumes zero.
> **Recommendation:** Pass `"0"` to `CreateArrayAction`, OR explicitly zero the array at top of `operator()()` when `FindAvgMisals=true`.

#### D3 — Default array name change (Low, user-facing)

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D3`
> **Symptom:** Default output array name renamed in PR #1438: `AvgCAxisMisalignments` → `AvgNeighborCAxisMisalignments`. Parameter labels also reworded.
> **Affected users:** User-saved pipelines referencing the old default name will produce arrays under the new name.
> **Recommendation:** Document in release notes / migration guide.

#### D4 — EbsdLib 2.0 math drift (Low)

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D4`
> **Symptom:** *(if observed)* per-feature avg c-axis misalignment values drift vs. pre-PR-#1472 values
> **Root cause:** Precision / Library — PR #1472 replaced `OrientationTransformation::qu2om` with `Quaternion::toOrientationMatrix()` and `OrientationMatrixToGMatrixTranspose` with `Matrix::transpose()`.
> **Recommendation:** Numerically diff vs. pre-#1472 commit on the existing exemplar. Likely benign; confirm.

#### D5 — Silenced hex-symmetry warning (Low, UX)

> **Deviation ID:** `ComputeFeatureNeighborCAxisMisalignments-D5`
> **Symptom:** PR #1438 moved the hex-symmetry warning from `resultOutputActions.warnings()` (visible in pipeline logs) to `preflightUpdatedValues` (GUI-only banner). Pipeline-mode users no longer see it.
> **Recommendation:** Restore `warnings()` push, OR document the deliberate UX choice.

### Exit criteria

- ✅ Comparison run on Phase-5 fixtures
- ✅ User doc's "0.0001" drift claim re-measured; result recorded
- ✅ D1–D5 each populated or retracted with a "no observed difference" note
- ✅ Report opens with Phase-3 line

---

## Phase 10 — Exemplar Validation & Publishing

**Goal**: validate exemplars and add Oracle Provenance to the archive ReadMe.

### Tasks

- [ ] **Invoke `bluequartz-skills:validate-and-publish-exemplars`**.
- [ ] **Decide archive disposition**:
  - [ ] Keep `compute_feature_neighbor_caxis_misalignments.tar.gz` as-is (Class 5 with provenance documenting SIMPL 6.5.171 origin if Phase 2 confirmed)
  - [ ] **Augment with mixed-phase fixture archive** for the new Phase-5 mixed-phase fixtures, OR include them inline in test source (preferred for small fixtures)
  - [ ] Regenerate if Phase 2 found mis-versioning
- [ ] **Add Oracle Provenance block** to archive ReadMe per Phase-4 class:
  - Class 4 + Class 1: no provenance block needed (oracle in test code)
  - Class 5 (existing hex-only exemplar): named source (SIMPL 6.5.171) + date generated + engineer
- [ ] **Re-run dual-build with final exemplar.**

### Exit criteria

- ✅ Archive disposition decided
- ✅ Oracle Provenance block in ReadMe (per Phase-4 class)
- ✅ Dual-build pass

---

## Phase 11 — Documentation Review

**Goal**: bring user docs current with the bug fix, the Phase-9 deviations, and the doc gaps.

### Tasks

- [ ] **Invoke `bluequartz-skills:review-filter-docs`**.
- [ ] **Update the doc's "Notes" section** — currently mentions only `Hexagonal_High`; algorithm also accepts `Hexagonal_Low` (per Phase 1 §"Algorithm summary" restrictions).
- [ ] **Reconcile the doc's "0.0001" drift claim** with the Phase-9 re-measurement. Either confirm the claim, update the number, or remove if no longer accurate.
- [ ] **Add a "Known differences from DREAM3D 6.5" section** linking the public Phase-9 Deviation entries (especially D1's user-facing impact and D3's array rename).
- [ ] **Add a "Bug fixed in DREAM3D-NX" callout** mentioning D1 — users who relied on old (buggy) avg c-axis misalignments need to know.

### Exit criteria

- ✅ Review skill complete
- ✅ Hexagonal_Low support documented
- ✅ Drift claim reconciled
- ✅ Phase-9 Deviations linked from user docs
- ✅ D1 fix called out for users

---

## Phase 12 — Archive

**Goal**: assemble OneDrive archive folder with all data, scripts, and a complete ReadMe.

### Tasks

- [ ] **Invoke `bluequartz-skills:archive-filter-verification`**.
- [ ] **Add V&V-policy-required fields to archive ReadMe**:
  - Algorithm Relationship (Phase 3)
  - Oracle class + rationale (Phase 4)
  - Oracle Provenance block (Phase 10, for Classes 2/3/5)
  - Second-engineer review or skip reason (Phase 4)
  - Promoted-artifacts list (Phase 2)
  - Reproduction instructions
  - **Bug-fix note**: D1 fix landed in Phase 8, with Fixture A as the regression pin
- [ ] **Confirm archive contents**: Phase-5 fixtures, working artifacts, Phase-6 + Phase-9 pipelines (NX and 6.5.172), Phase-9 comparison report, ReadMe.
- [ ] **Upload to OneDrive** as `ComputeFeatureNeighborCAxisMisalignments_VandV/`.

### Exit criteria

- ✅ Archive assembled with all artifacts
- ✅ ReadMe complete (incl. D1 bug-fix note)
- ✅ Uploaded; OneDrive path recorded in Phase 13

---

## Phase 13 — Update tracking artifacts

**Goal**: close the loop and coordinate the bundled fix with the sibling filter.

### Tasks

- [ ] **Update Status line at top** of this doc: `*Status:* **DRAFT — ...**` → `*Status:* **COMPLETE — V&V finished YYYY-MM-DD.**`. File stays at `src/Plugins/OrientationAnalysis/vv/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`.
- [ ] **Record OneDrive archive path**:
  > *OneDrive:*
- [ ] **Update [`docs/vv_retroactive_reports/ComputeFeatureNeighborCAxisMisalignmentsFilter.md`](../../../../docs/vv_retroactive_reports/ComputeFeatureNeighborCAxisMisalignmentsFilter.md)** — DRAFT → confirmed; populate confirmed Algorithm Relationship, Oracle class, V&V status, final Deviation entries (D1–D5).
- [ ] **Update [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md)**:
  - Move filter to confirmed row
  - **Clear D1 bug flag** with commit reference of the bundled fix PR
  - Update at-a-glance metrics (8 → 7 confirmed real bugs, etc.)
- [ ] **Update [`docs/vv_bug_triage_reports/bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md)** — mark Bug 3 as RESOLVED with commit reference.
- [ ] **Coordinate the bundled PR** — per [`bug_triage.md`](../../../../docs/vv_bug_triage_reports/bug_triage.md) "PR landing plan" PR A: bundle the D1 fix here with the analogous fix to `ComputeFeatureNeighborMisorientations.cpp` (Bug #2 in triage). Single PR titled e.g. *"BUG: Fix divisor reset in ComputeFeatureNeighbor{C-Axis,}Misalignments average computation"*.
- [ ] **Update memory** (`mtr_vv_policy.md`) noting the divisor-bug fix landed and the V&V workflow validated end-to-end on a bug-bearing Tier-1 filter.
- [ ] **Note in team V&V deliverable tracker.**

### Exit criteria

- ✅ Status updated; OneDrive recorded
- ✅ Retroactive report DRAFT → confirmed
- ✅ INDEX updated; D1 bug flag cleared
- ✅ Bug triage updated; Bug 3 marked RESOLVED
- ✅ Bundled PR landed (with sibling fix)
- ✅ Memory updated
- ✅ Phase Summary block below filled in

---

## Phase Summary *(fill in at end)*

```markdown
# V&V Complete: ComputeFeatureNeighborCAxisMisalignmentsFilter

## Algorithm Relationship
<Port|Minor|Rewrite|New filter> — <one-line evidence>

## Oracle
Class: <e.g., 4 + 1>
Justification: <one-line>
Second-engineer review: <named, date> | <skipped: reason>

## Bug fixes landed
- D1 (divisor reset) — fixed in commit <sha>; bundled with sibling ComputeFeatureNeighborMisorientations fix per bug_triage.md PR A

## Phase Results (1-line per phase)
| Phase | Status | Notes |
|---|---|---|
| 1–13 | ... | ... |

## Outstanding
<Any deferred / known limitations>
```
