# V&V Working Document: ComputeNeighborhoodsFilter

*Status:* **DRAFT — Phase 1 (Discovery) complete; Phases 2–13 are work orders for the engineer to execute.**

This document is the engineer's working V&V doc for `ComputeNeighborhoodsFilter`. **Fresh V&V pass** — this is a Tier-2 catalog filter (not in the audit's 22-filter Tier-1 list), so no retroactive report exists. Discovery in Phase 1 was performed by audit-style source/git inspection.

> **Notable**: PR #1485 was a recent **bug fix in the binning system** that rewrote the algorithm to use a proper spatial-hash with multi-bin scan (the previous implementation only found centroids in the SAME bin and missed valid neighbors). Mike (MAJ) hand-verified the new tests in that PR. This is a **SIMPLNX-only correctness fix** that almost certainly produces different output than DREAM3D 6.5.171, which still has the old buggy single-bin scan. Phase 9 will need a Deviation entry naming SIMPLNX as the correct version.

**How to use this document:**
- Each phase has a **Goal**, **Tasks** (checkboxes), and **Exit criteria**. Phase 1 is the canonical source for discovery context; later phases reference Phase 1 rather than restate.
- Where a "starting point" is provided, you can confirm it or replace it.
- Update this document as you work. Phase 13 updates the Status line at the top to "COMPLETE"; the file stays at this location.

**Source-of-truth references** (paths relative to this doc at `src/Plugins/SimplnxCore/vv/`):
- Policy: [`docs/vv_templates/mtr_filter_verification_validation.md`](../../../../docs/vv_templates/mtr_filter_verification_validation.md)
- Oracle quick-reference: [`docs/vv_templates/oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md)
- Audit cross-cutting findings: [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md) "Cross-cutting findings"
- Skill: `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`

---

## Phase 1 — Discovery *(complete; canonical source for all later phases)*

**Goal**: locate every file related to this filter and identify the legacy DREAM3D equivalent.

### Tasks (verify findings)

- [ ] Confirm metadata, source files, exemplar archive, PR list, test inventory, algorithm summary, and observed defects below are accurate
- [ ] Re-run `git log --since=2025-10-01 --oneline -- <Phase-1 files>` to check no new PRs landed since this doc was generated

### Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| SIMPLNX ClassName | `ComputeNeighborhoodsFilter` |
| Human Name | Compute Feature Neighborhoods *(per user doc)* |
| SIMPL UUID | `697ed3de-db33-5dd1-a64b-04fb71e7d63e` *(via `SimplnxCoreLegacyUUIDMapping.hpp`)* |
| SIMPL ClassName | `ComputeNeighborhoods` *(per legacy UUID map; legacy file likely `FindNeighborhoods` per DREAM3D `Find*→Compute*` convention — confirm in legacy SIMPL repo)* |
| Plugin | SimplnxCore |
| Tier | Tier-2 catalog filter (not in MTR SBIR 22-filter list) |
| Mode | Fresh V&V pass (no retroactive report) |

### Source files

| Path (relative to `src/Plugins/SimplnxCore/`) | Notes |
|---|---|
| `src/SimplnxCore/Filters/ComputeNeighborhoodsFilter.{hpp,cpp}` | Filter |
| `src/SimplnxCore/Filters/Algorithms/ComputeNeighborhoods.{hpp,cpp}` | Algorithm — spatial-hash implementation post-PR-#1485 |
| `test/ComputeNeighborhoodsTest.cpp` | 3 TEST_CASEs — note `_1`, `_3` naming suggests `_2` was removed at some point |
| `test/simpl_conversion/6_4/ComputeNeighborhoodsFilter.json` | 6.4 conversion fixture |
| `test/simpl_conversion/6_5/ComputeNeighborhoodsFilter.json` | 6.5 conversion fixture |
| `docs/ComputeNeighborhoodsFilter.md` | User doc — has `images/ComputeFeatureNeighborhoods_MultiplesOfAvgDiameter.png`. ⚠️ See "Observed defects" below for doc/code discrepancy. |
| `pipelines/Small_IN100_Processing/(03) Small IN100 Morphological Statistics.d3dpipeline` *(in OrientationAnalysis plugin)* | Production caller |

### Production callers (multi-pipeline blast radius)

The user doc lists 3 example pipelines; this filter is invoked from at least:
- `(03) Small IN100 Morphological Statistics` (confirmed in source via `grep`)
- `InsertTransformationPhase` (per user doc — confirm location during Phase 2)
- `(06) SmallIN100 Synthetic` (per user doc — confirm during Phase 2)

Multi-pipeline impact means any output deviation propagates broadly.

### Test exemplar archive

- **Archive**: `compute_feature_neighborhoods.tar.gz`
- **SHA512**: `dda96546de1b924d8145e8d173d00605bbf7d311c8d3719edd9d698bb8a326009fcfdb885d2fe08204b8e8dcbebe2eb699d3255e25fe34cfc36b9f950a8e07da`
- **Referenced in**: `test/CMakeLists.txt`
- **Provenance**: not yet documented. Phase 2 investigates. Likely SIMPLNX-generated **post-PR-#1485** (i.e., the new spatial-hash algorithm output) given Mike's "hand verified by MAJ" note in PR #1485 — would be a **circular oracle** if so (cross-cutting finding from audit).

### Material PRs since 2025-10-01

| PR | Date | Title | Effect on this filter |
|---|---|---|---|
| #1485 | (date TBD) | **BUG: New implementation for ComputeNeighborhoods filter** | **Major rewrite** (+212/-77 in algorithm.cpp; +83 in test.cpp). Replaced previous binning system that only matched same-bin centroids with proper spatial-hash + multi-bin scan. PR description: *"The previous implementation has issues with the binning system and will only find that a centroid is within a radius if both centroids fall into the same bin. New unit test file was created and hand verified by MAJ."* |
| #1543 | 2026-02-24 | DOC: Update pipeline references in each of the documentation files | Minor doc edit |
| #1547 | 2026-03-10 | DOC: Fix filter documentation and documentation related code bugs | Minor doc edit |
| #1588 | 2026-04-22 | ENH: SIMPL Backwards Compatibility Test Redesign | Added per-filter SIMPL conversion test + 6.4 + 6.5 fixture JSONs |
| #1438 | 2025-10-25 | ENH: Microtexture related filter cleanup | **Pruned for this filter** — only 1 line changed (likely EbsdLib include style). The audit's "always inspect carefully" rule for #1438 doesn't promote here. |
| #1457, #1538 | various | (style cleanup, test infra) | Pruned — pure mechanical edits |

### TEST_CASE inventory

`ComputeNeighborhoodsTest.cpp` contains 3 `TEST_CASE`s:

| # | Name | Coverage | Encodes oracle? |
|---|---|---|---|
| 1 | `ComputeNeighborhoods_1` | (TBD — need to inspect to determine fixture intent) | Probably exemplar comparison |
| 2 | `ComputeNeighborhoods_3` | (TBD — `_3` suffix suggests `_2` was removed; may be the hand-verified test from PR #1485) | (TBD) |
| 3 | `ComputeNeighborhoodsFilter: SIMPL Backwards Compatibility` | 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` | Conversion-only |

⚠️ **Test gap from naming**: `_1`, `_3` naming with no `_2` warrants investigation in Phase 2. The two `_N` tests should be renamed to descriptive names per project convention (e.g. `Valid Filter Execution`, `<scenario name>`).

### Algorithm summary (post-PR-#1485 spatial-hash implementation)

For each feature `i ≥ 1` (skipping background feature 0): count and list other features whose centroid lies within distance `R = avgDiameter * MultiplesOfAverage / 2` of feature `i`'s centroid. Two output arrays:
1. Per-feature `Int32Array Neighborhoods` — neighbor count
2. Per-feature `NeighborList<int32> NeighborhoodList` — neighbor IDs

**Algorithm body**:
1. Compute average ESD `avgDiameter` across **all** non-background features (line 230-236 of algorithm.cpp)
2. Place each feature centroid into a 3D bin keyed by `floor((centroid - origin) / avgDiameter)` (line 241-251)
3. For each feature, scan all bins within ±k = `ceil(MultiplesOfAverage)` in each dimension (line 129-133); for each candidate in each bin, compute distance and add to neighbor list if `distSq <= radiusSq`
4. Emit `m_Neighborhoods[i]++` and `m_LocalNeighborhoodList[i].push_back(j)` via mutex-guarded `updateNeighborHood(i, j)` callback

**Threading**:
- Parallelized via `ParallelDataAlgorithm` over feature index range
- All updates funnel through `updateNeighborHood()` which holds `std::mutex m_Mutex` for both the `Neighborhoods` increment and the `NeighborhoodList` push_back
- Coarse-grained mutex on every neighbor pair found — correctness OK, **may be a perf hotspot** on dense feature grids (Phase 7 algorithm review)

**Output post-processing**: After parallel work, single-threaded loop at line 264-269 copies the per-feature `m_LocalNeighborhoodList[i]` into the output `NeighborList<int32>` via `setList(i, ...)`.

### Observed defects to flag in later phases

⚠️ **D-candidate (doc): Per-phase vs all-features avgDiameter**
- User doc (`docs/ComputeNeighborhoodsFilter.md` line 13) says: *"Compute the average equivalent diameter for all features in a given phase"*.
- Algorithm code (line 231-236) computes avgDiameter across **all** non-background features regardless of phase.
- Either the doc is wrong, or the code missed an intended phase filter. **Phase 11 doc review must reconcile.** Phase 9 may need a Deviation if legacy DREAM3D actually does phase-filter the avg.

⚠️ **D-candidate (correctness vs legacy): SIMPLNX bug fix per PR #1485**
- PR #1485 description confirms a real correctness bug in the previous implementation (single-bin scan only). DREAM3D 6.5.171 likely still has the buggy version.
- If true: SIMPLNX produces MORE neighbors than legacy on the same input (legacy missed cross-bin neighbors). High-confidence Deviation entry; **trust SIMPLNX**.

⚠️ **Defect signal: avgDiameter division by `totalFeatures` includes feature 0**
- Line 236: `avgDiameter /= static_cast<float32>(totalFeatures);`
- The accumulation loop (line 231) starts at `i = 1` (skips feature 0), but the divisor is the full `totalFeatures` count. Off-by-one — divisor should probably be `totalFeatures - 1` to match the actual count of summed features.
- Effect: avgDiameter is biased low by `(N-1)/N` factor. For N = 1000 features, ~0.1% bias; for N = 10 features, ~10% bias. Worth confirming with Mike whether intentional or a real bug.

⚠️ **Defect signal: ⚠️ Test naming `_1`, `_3`** (already noted in TEST_CASE inventory above) — suggests `_2` was removed; should be renamed to descriptive names.

### Exit criteria

- ✅ Verification checkboxes above checked
- ✅ Any new findings or corrections noted in this section
- ✅ `git log` confirms no new PRs landed since this doc was generated

---

## Phase 2 — Promote existing work product

**Goal**: investigate provenance and decide what to promote, augment, or replace.

### Tasks

- [ ] **Investigate provenance of `compute_feature_neighborhoods.tar.gz`** (Phase 1 §"Test exemplar archive"):
  - [ ] When and by whom (`git log --all --follow -- src/Plugins/SimplnxCore/test/CMakeLists.txt | grep -B2 compute_feature_neighborhoods`)
  - [ ] Download archive locally; inspect for inner `ReadMe.md`, source pipeline, input data
  - [ ] **Confirm the circular-oracle suspicion**: was the exemplar generated by SIMPLNX **post-PR-#1485** (after the bug fix)? If yes, the existing exemplar reflects the corrected algorithm, not legacy. Mark as Class 5 (legacy expert = Mike via PR #1485 hand-verification) OR regenerate with documented oracle.
- [ ] **Read both `ComputeNeighborhoods_1` and `ComputeNeighborhoods_3` tests** — record what each fixture covers and recommend descriptive renames for Phase 8.
  > *Test 1 covers:*
  >
  > *Test 3 covers:*
- [ ] **Investigate the missing `_2`**: `git log -S "ComputeNeighborhoods_2" -- src/Plugins/SimplnxCore/test/ComputeNeighborhoodsTest.cpp` to find when `_2` was removed and why.
- [ ] **Confirm production callers** (Phase 1 §"Production callers"): grep for the filter in all `.d3dpipeline` files in the simplnx repo + the OrientationAnalysis pipelines. Record actual call sites:
  > *Confirmed callers:*
- [ ] **Decide promote/augment/replace** for each pre-existing artifact:

| Artifact | Candidate role | Decision |
|---|---|---|
| Exemplar `compute_feature_neighborhoods.tar.gz` | Class 5 (Mike via PR #1485 hand-verification) — but mark as post-fix oracle | _______________ |
| Hand-verified tests from PR #1485 | Class 1 / Class 4 candidates — promote to explicit invariant assertions in Phase 8 | _______________ |

### Exit criteria

- ✅ Archive provenance determined; circular-oracle question answered
- ✅ Both `_1` and `_3` test intents documented
- ✅ Missing `_2` history explained
- ✅ Production caller list confirmed
- ✅ Promotion table filled in

---

## Phase 3 — Algorithm Relationship classification

**Goal**: classify how SIMPLNX relates to legacy DREAM3D 6.5.172. Opens the Phase 9 report.

### Tasks

- [ ] **Open SIMPL `FindNeighborhoods.cpp`** (or whatever the legacy file is named) in DREAM3D 6.5.x. Compare the algorithm body to SIMPLNX (Phase 1 §"Algorithm summary").
- [ ] **Critical cross-check**: does legacy 6.5.171 have the **single-bin scan bug** that PR #1485 fixed? If yes, this filter relationship is "Port + behavior fix" and SIMPLNX is the corrected version. The doc currently claims the new algorithm matches the documented behavior; confirm.
- [ ] **Verify `FromSIMPLJson()` parameter mapping** in `ComputeNeighborhoodsFilter.cpp` against the 6.4 + 6.5 conversion fixtures (Phase 1 §"Source files").
- [ ] **Pick classification**: ☐ Port  ☐ Minor changes  ☐ **Rewrite** (likely choice given PR #1485)  ☐ New filter
- [ ] **Write the Phase-9 opening line**:
  > *Suggested* — *Algorithm Relationship: **Rewrite** — UUID preserved (`924c10e3-…` in SIMPLNX, `697ed3de-…` in SIMPL), but PR #1485 replaced the previous binning system entirely. The legacy implementation only found centroids in the same bin, missing valid neighbors across bin boundaries. SIMPLNX uses spatial-hash with multi-bin scan to fix this. Both implementations claim to compute the documented operation; legacy's bug means output WILL differ.*
  >
  > *Final:*

### Exit criteria

- ✅ SIMPL source compared
- ✅ Single-bin-scan bug presence in legacy confirmed/refuted
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

**Class 1 (Analytical) primary** — the operation is closed-form geometric: for each feature `i`, count other features `j ≠ i` where `||centroid[i] - centroid[j]|| <= avgDiameter * MultiplesOfAverage / 2`. Hand-derivable for any small toy fixture.

**Class 4 (Invariant) companion** — natural assertions:
1. `Neighborhoods[i] == NeighborhoodList[i].size()` (count and list must agree — this is the simplest cross-output invariant)
2. **Symmetry**: `j ∈ NeighborhoodList[i]` IFF `i ∈ NeighborhoodList[j]` (Euclidean distance is symmetric)
3. `Neighborhoods[0] == 0` and `0 ∉ NeighborhoodList[any]` (background is excluded)
4. `MultiplesOfAverage = 0` → all `Neighborhoods` are 0
5. `MultiplesOfAverage` very large (covers entire volume) → every feature is in every other feature's list, so `Neighborhoods[i] == totalFeatures - 2` for all i ≥ 1 (excluding self and background)
6. **Bin-boundary invariant**: two features whose centroids are in adjacent bins but within distance R MUST appear in each other's lists. *(This is the specific legacy-bug invariant from PR #1485 — encode it explicitly to pin the regression.)*

**Class 3 (Paper-based)** — *not* recommended unless a citation can be located. The "neighborhood count via average diameter sphere" metric is a standard DREAM3D-internal feature for synthetic-microstructure builders; no specific paper appears to be cited.

### Exit criteria

- ✅ Primary + companion class(es) recorded
- ✅ Class 5 justification recorded if applicable (not expected here)
- ✅ Second-engineer reviewer or skip reason recorded

---

## Phase 5 — Toy data design + independent expected output

**Goal**: build minimum-size synthetic fixtures exercising every code path, with expected output derived from the oracle alone.

### Tasks

- [ ] **Inventory the existing PR #1485 hand-verified fixtures** as the starting Class-5 oracle (Mike-verified). Record what each currently covers:
  > *Test 1 (`_1`) covers:*
  >
  > *Test 3 (`_3`) covers:*
- [ ] **Add the bin-boundary regression-pin fixture** (Phase 4 invariant 6 — the specific legacy bug PR #1485 fixed):
  - [ ] Fixture A — 2 features whose centroids are exactly `0.4 * avgDiameter` apart but fall in adjacent bins (because each bin width is 1.0 in normalized space). Both should appear in each other's neighbor lists. The legacy single-bin scan would miss this; the fixed spatial-hash with `±k = 1` should catch it.
- [ ] **Add Class-1 analytical fixtures**:
  - [ ] Fixture B — 1 feature pair, centroids exactly at distance `R` (boundary case for `<=` comparison)
  - [ ] Fixture C — 1 feature pair, centroids at `R + epsilon` (just outside; should NOT be neighbors)
  - [ ] Fixture D — 1 feature pair, centroids at `R - epsilon` (just inside; should be neighbors)
  - [ ] Fixture E — 3 features colinear at distance R; assert `Neighborhoods[middle] == 2`, `Neighborhoods[outer] == 1`
- [ ] **Add Class-4 invariant fixtures** (covering invariants 1–5):
  - [ ] Fixture F — count-vs-list consistency (invariant 1)
  - [ ] Fixture G — symmetry (invariant 2)
  - [ ] Fixture H — `MultiplesOfAverage = 0` boundary (invariant 4)
  - [ ] Fixture I — `MultiplesOfAverage` very large (invariant 5)
- [ ] **Add the avgDiameter-divisor probe** (Phase 1 §"Observed defects"):
  - [ ] Fixture J — Construct N=10 features with known equivalent diameters; hand-derive expected `avgDiameter` two ways: `Σ/totalFeatures` (current code) vs `Σ/(totalFeatures - 1)` (likely-intended). Document which one matches the post-fix algorithm output. This determines whether the off-by-one is real and whether to add D-candidate to Phase 9.
- [ ] **Save oracle artifacts** to `src/Plugins/SimplnxCore/vv/ComputeNeighborhoodsFilter_artifacts/`.

### Exit criteria

- ✅ Existing Mike-verified fixtures cataloged
- ✅ Bin-boundary regression-pin Fixture A designed
- ✅ Class-1 fixtures B–E designed
- ✅ Class-4 invariant fixtures F–I designed
- ✅ Off-by-one probe Fixture J designed
- ✅ Working artifact folder created
- ✅ Second-engineer review of oracle artifacts (if scheduled in Phase 4)

---

## Phase 6 — SIMPLNX vs oracle reconciliation

**Goal**: confirm SIMPLNX matches the oracle on every Phase-5 fixture.

### Tasks

- [ ] **Run the existing test suite** to confirm baseline:
  ```bash
  cd /Users/mjackson/Workspace2/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel
  ctest -R "ComputeNeighborhoods" --verbose
  ```
- [ ] **Run each Phase-5 fixture** as it's implemented in Phase 8. Record outcomes in the table below.
- [ ] **Investigate Fixture J's outcome**: does the off-by-one `avgDiameter` divisor (Phase 1 §"Observed defects") match expected behavior from PR #1485 hand-verification, or does it indicate a separate latent bug?
- [ ] **Bin-boundary fixture A is the most important**: it pins the PR #1485 regression. If it fails on `develop`, PR #1485's fix has been undone.

### Reconciliation table

| Fixture ID | Expected | SIMPLNX actual | Match? | Action |
|---|---|---|---|---|
| A — bin-boundary regression pin | both features in each other's lists | _______ | _______ | _______ |
| B — distance == R (boundary) | per `<=` semantics | _______ | _______ | _______ |
| C — distance == R + ε | NOT neighbors | _______ | _______ | _______ |
| D — distance == R - ε | neighbors | _______ | _______ | _______ |
| E — 3 colinear at R | counts {1, 2, 1} | _______ | _______ | _______ |
| F — count-vs-list invariant | always equal | _______ | _______ | _______ |
| G — symmetry | always symmetric | _______ | _______ | _______ |
| H — MultiplesOfAverage = 0 | all counts 0 | _______ | _______ | _______ |
| I — very large multiple | all `totalFeatures - 2` | _______ | _______ | _______ |
| J — avgDiameter divisor | per derivation | _______ | _______ | _______ |
| Existing `_1`, `_3` | per existing assertions | _______ | _______ | _______ |

### Exit criteria

- ✅ All Phase-5 fixtures run; outcomes recorded
- ✅ Bin-boundary Fixture A passes (PR #1485 regression pin holds)
- ✅ Off-by-one probe Fixture J resolved (real defect → fix + add D-candidate; intentional → document in Phase 11)
- ✅ Existing tests still pass

---

## Phase 7 — Algorithm Review

**Goal**: code quality pass on already-correct code.

### Tasks

- [ ] **Invoke `bluequartz-skills:review-algorithm`** with `ComputeNeighborhoodsFilter`.
- [ ] **Pre-observed quality items to confirm**:
  - [ ] **Mutex contention** in `updateNeighborHood()` (Phase 1 §"Algorithm summary"). Single coarse-grained `m_Mutex` serializes every neighbor-pair write across all threads. On dense feature grids this is the perf bottleneck. Consider per-feature mutexes or per-thread local accumulation + post-merge.
  - [ ] **`m_Neighborhoods` write via `(*m_Neighborhoods)[sourceIndex].inc()`** (line 207) — confirm `inc()` is atomic OR document that the mutex covers the access.
  - [ ] **`avgDiameter` divisor includes feature 0** (Phase 1 §"Observed defects"): line 236 divides by `totalFeatures` not `totalFeatures - 1`. Off-by-one — confirm intent.
  - [ ] **Single-threaded post-processing loop** (line 264-269) copies `m_LocalNeighborhoodList[i]` into the output NeighborList. For very large feature counts, this is unparallelized.
  - [ ] **`(*m_Neighborhoods)[i] = 0` initialization in line 233** is inside the avgDiameter loop, which runs from `i = 1`. Feature 0's `Neighborhoods` value is never explicitly initialized — relies on `CreateArrayAction` default. Confirm.
- [ ] **Address all Critical/Warning findings** from the review (fix or document deferral).

### Exit criteria

- ✅ Review skill complete
- ✅ Critical/Warning findings fixed or deferred with rationale
- ✅ Mutex contention assessed (acceptable as-is or refactor scheduled)
- ✅ Off-by-one divisor confirmed/fixed

---

## Phase 8 — Unit Test Review & Implementation

**Goal**: encode Phase-5 fixtures as TEST_CASEs, rename the cryptic `_1` / `_3` tests, and add Class-4 invariant assertions.

### Tasks

- [ ] **Rename `ComputeNeighborhoods_1` and `ComputeNeighborhoods_3`** to descriptive names per Phase 2 investigation (e.g. `Valid Filter Execution`, `<scenario>`). Preserve test bodies; only the names change.
- [ ] **Implement Fixtures A–J** as new TEST_CASEs.
- [ ] **Add Class-4 invariant assertions** to the existing tests:
  ```cpp
  // Invariant 1: count == list size
  for (size_t i = 1; i < numFeatures; i++) {
    REQUIRE(neighborhoods[i] == static_cast<int32>(neighborhoodList[i].size()));
  }

  // Invariant 2: symmetry
  for (size_t i = 1; i < numFeatures; i++) {
    for (auto j : neighborhoodList[i]) {
      const auto& jList = neighborhoodList[j];
      REQUIRE(std::find(jList.begin(), jList.end(), static_cast<int32>(i)) != jList.end());
    }
  }

  // Invariant 3: background excluded
  REQUIRE(neighborhoods[0] == 0);
  for (size_t i = 1; i < numFeatures; i++) {
    const auto& list = neighborhoodList[i];
    REQUIRE(std::find(list.begin(), list.end(), 0) == list.end());
  }
  ```
- [ ] **Wrap every `getDataRefAs<T>()` call in `REQUIRE_NOTHROW()`** per `.claude/CLAUDE.md`. Audit existing tests.
- [ ] **Plan/implement gap coverage** via `bluequartz-skills:plan-filter-tests` and `bluequartz-skills:implement-filter-tests` if Phase 1 + new fixtures still leave any code path uncovered.
- [ ] **Dual-build**: per `bluequartz-skills:dual-build-protocol`, in-core and OOC must both pass.

### Exit criteria

- ✅ `_1` / `_3` renamed to descriptive names
- ✅ Fixtures A–J implemented as TEST_CASEs
- ✅ Class-4 invariant assertions added to existing tests
- ✅ All `getDataRefAs<T>()` calls wrapped
- ✅ Dual-build pass

---

## Phase 9 — Legacy DREAM3D Comparison (diff explanation)

**Goal**: diff against DREAM3D 6.5.172 and write up differences as user-facing Deviation entries.

### Tasks

- [ ] **Invoke `bluequartz-skills:compare-legacy-dream3d`**. Use Phase-5 Fixture A (bin-boundary), B–E (geometric), and one of the Mike-verified existing tests as comparison inputs.
- [ ] **Open the report with the Phase-3 Algorithm Relationship line.**
- [ ] **For each candidate deviation**, populate or retract:

#### D1 — SIMPLNX corrects legacy single-bin-scan bug (HIGH-confidence Deviation)

> **Deviation ID:** `ComputeNeighborhoods-D1`
> **Filter UUID:** `924c10e3-2f39-4c08-9d7a-7fe029f74f6d`
> **Symptom:** SIMPLNX produces MORE neighbors per feature than DREAM3D 6.5.171 on the same input. Difference grows with feature density and bin-boundary proximity.
> **Root cause:** Bug in 6.5.171 — the previous implementation only matched centroids in the SAME spatial bin, missing valid neighbors that fall in adjacent bins but within the search radius. Fixed in SIMPLNX by PR #1485.
> **Affected users:** Anyone running this filter on real microstructure data — most production datasets will see different counts.
> **Recommendation:** **Trust SIMPLNX.** Legacy was wrong. Multi-pipeline blast radius (the 3 production callers in Phase 1) means downstream results may shift; document in user docs and migration guide.

#### D2 — avgDiameter divisor off-by-one (Medium, pending Fixture J)

> **Deviation ID:** `ComputeNeighborhoods-D2`
> **Filter UUID:** `924c10e3-2f39-4c08-9d7a-7fe029f74f6d`
> **Symptom:** *(if confirmed)* avgDiameter is biased low by `(N-1)/N` because line 236 divides the sum (which excluded feature 0) by `totalFeatures` not `totalFeatures - 1`. Effect is significant for small N.
> **Root cause:** TBD by Phase 6 Fixture J (and Phase 3 legacy comparison — does legacy have the same off-by-one?).
> **Affected users:** Datasets with small numbers of features — synthetic microstructure builders especially.
> **Recommendation:** *(populate after Phase 6)* — likely "fix in SIMPLNX, document if legacy has the same bug."

#### D3 — Per-phase vs all-features avgDiameter (Doc/Code discrepancy, severity TBD)

> **Deviation ID:** `ComputeNeighborhoods-D3`
> **Filter UUID:** `924c10e3-2f39-4c08-9d7a-7fe029f74f6d`
> **Symptom:** User doc claims avgDiameter is computed per-phase; code computes it across all features regardless of phase. Either the doc is wrong, or the code missed an intended phase filter.
> **Root cause:** TBD by Phase 11 doc reconciliation + Phase 3 legacy comparison.
> **Recommendation:** If legacy also computes across all features → fix the doc (no algorithm change). If legacy phase-filters → add the filter to SIMPLNX (real algorithm change). Phase 9 result determines the path.

### Exit criteria

- ✅ Comparison run on Phase-5 fixtures
- ✅ D1 confirmed (or retracted if legacy doesn't have the bug after all)
- ✅ D2 resolved by Phase 6 Fixture J + legacy comparison
- ✅ D3 resolved by Phase 11 doc reconciliation + legacy comparison
- ✅ Report opens with Phase-3 line

---

## Phase 10 — Exemplar Validation & Publishing

**Goal**: validate exemplars and add Oracle Provenance to the archive ReadMe.

### Tasks

- [ ] **Invoke `bluequartz-skills:validate-and-publish-exemplars`**.
- [ ] **Decide archive disposition** (depends on Phase 2 provenance):
  - [ ] Keep `compute_feature_neighborhoods.tar.gz` as-is (Class 5 with Mike named as expert per PR #1485 hand-verification)
  - [ ] Augment with new fixtures from Phase 5 (preferred for the bin-boundary regression pin and the geometric Class-1 cases — these can be in test source rather than archive)
  - [ ] Regenerate if Phase 2 found mis-versioning
- [ ] **Add Oracle Provenance block** to archive ReadMe per Phase-4 class:
  - Class 1 + Class 4: no provenance block needed (oracle in test code with derivations)
  - Class 5 (existing exemplar): named expert (Mike, MAJ), date (PR #1485 merge date), reference to PR #1485
- [ ] **Re-run dual-build with final exemplar.**

### Exit criteria

- ✅ Archive disposition decided
- ✅ Oracle Provenance block in ReadMe (per Phase-4 class)
- ✅ Dual-build pass

---

## Phase 11 — Documentation Review

**Goal**: bring user docs current with PR #1485, Phase-9 deviations, and resolve the per-phase discrepancy.

### Tasks

- [ ] **Invoke `bluequartz-skills:review-filter-docs`**.
- [ ] **Resolve the per-phase discrepancy** (Phase 1 §"Observed defects" / Phase 9 D3):
  - [ ] If algorithm is correct (across all features): update doc line 13 to remove "in a given phase"
  - [ ] If algorithm is wrong (should be per-phase): file a bug for fix; doc stays
- [ ] **Add a "Known differences from DREAM3D 6.5" section** linking the public Phase-9 Deviation entries (especially D1's user-facing impact — output WILL differ).
- [ ] **Add a "Bug fixed in DREAM3D-NX" callout** mentioning the PR #1485 single-bin-scan fix — users who relied on (buggy) legacy counts need to know.
- [ ] **Verify the embedded image** (`images/ComputeFeatureNeighborhoods_MultiplesOfAvgDiameter.png`) accurately depicts the spatial-hash algorithm post-PR-#1485, not the old single-bin behavior.

### Exit criteria

- ✅ Review skill complete
- ✅ Per-phase discrepancy resolved (doc updated OR algorithm fixed)
- ✅ Phase-9 Deviations linked from user docs
- ✅ PR #1485 fix called out for users
- ✅ Embedded image verified or replaced

---

## Phase 12 — Archive

**Goal**: assemble OneDrive archive folder with all data, scripts, and a complete ReadMe.

### Tasks

- [ ] **Invoke `bluequartz-skills:archive-filter-verification`**.
- [ ] **Add V&V-policy-required fields to archive ReadMe**:
  - Algorithm Relationship (Phase 3 — likely "Rewrite")
  - Oracle class + rationale (Phase 4)
  - Oracle Provenance block (Phase 10)
  - Second-engineer review or skip reason (Phase 4)
  - Promoted-artifacts list (Phase 2)
  - Reproduction instructions (incl. how to regenerate the bin-boundary fixture)
  - **PR #1485 history note**: previous algorithm was wrong; new algorithm is the Mike-verified spatial-hash
- [ ] **Confirm archive contents**: Phase-5 fixtures, working artifacts, Phase-6 + Phase-9 pipelines (NX and 6.5.172), Phase-9 comparison report, ReadMe.
- [ ] **Upload to OneDrive** as `ComputeNeighborhoods_VandV/`.

### Exit criteria

- ✅ Archive assembled with all artifacts
- ✅ ReadMe complete (incl. PR #1485 history note)
- ✅ Uploaded; OneDrive path recorded in Phase 13

---

## Phase 13 — Update tracking artifacts

**Goal**: close the loop.

### Tasks

- [ ] **Update Status line at top** of this doc: `*Status:* **DRAFT — ...**` → `*Status:* **COMPLETE — V&V finished YYYY-MM-DD.**`. File stays at `src/Plugins/SimplnxCore/vv/ComputeNeighborhoodsFilter.md`.
- [ ] **Record OneDrive archive path**:
  > *OneDrive:*
- [ ] **No retroactive report to update** — this filter is Tier-2 and not in the audit's 22-filter list.
- [ ] **If `docs/vv_retroactive_reports/INDEX.md` adds a Tier-2 section** in the future, add a row for this filter there.
- [ ] **Note in the team's V&V deliverable tracker** that this filter is complete.
- [ ] **Update memory** if applicable (e.g. note that PR #1485-style "new implementation" PRs need their own V&V scrutiny — same pattern as FillBadData PR #1515).

### Exit criteria

- ✅ Status updated; OneDrive recorded
- ✅ Team tracker updated
- ✅ Phase Summary block below filled in

---

## Phase Summary *(fill in at end)*

```markdown
# V&V Complete: ComputeNeighborhoodsFilter

## Algorithm Relationship
<Port|Minor|Rewrite|New filter> — <one-line evidence>

## Oracle
Class: <e.g., 1 + 4>
Justification: <one-line>
Second-engineer review: <named, date> | <skipped: reason>

## Bug fixes / corrections landed
- D1 (single-bin-scan) — already fixed in PR #1485; this V&V pin'd it via Fixture A regression test
- D2 (avgDiameter off-by-one) — <fixed in commit X | retracted as intentional | unresolved>
- D3 (per-phase doc/code discrepancy) — <doc fixed | algorithm fixed | retracted>

## Phase Results (1-line per phase)
| Phase | Status | Notes |
|---|---|---|
| 1–13 | ... | ... |

## Outstanding
<Any deferred / known limitations>
```
