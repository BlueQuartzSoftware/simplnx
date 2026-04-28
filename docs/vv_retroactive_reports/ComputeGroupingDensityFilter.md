# Retroactive V&V: ComputeGroupingDensityFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `ff46afcf-de32-4f37-98bc-8f0fd4b3c122` |
| SIMPLNX ClassName | `ComputeGroupingDensityFilter` |
| SIMPLNX Human Name | Compute Grouping Densities |
| SIMPL UUID | `708be082-8b08-4db2-94be-52781ed4d53d` *(via `SimplnxCoreLegacyUUIDMapping.hpp`)* |
| SIMPL ClassName | `FindGroupingDensity` *(per SIMPL backwards-compat fixture)* |
| SIMPL Human Name | "Find Grouping Density" *(per fixture `Filter_Human_Label`)* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeGroupingDensityFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json`
- `src/Plugins/SimplnxCore/docs/ComputeGroupingDensityFilter.md`
- `src/Plugins/SimplnxCore/docs/Images/ComputeGroupingDensity_FeatureIds.png`
- `src/Plugins/SimplnxCore/docs/Images/ComputeGroupingDensity_ParentIds.png`
- `src/Plugins/SimplnxCore/docs/Images/ComputeGroupingDensity_Algorithm.png`
- `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` (legacy UUID map entry)
- `src/Plugins/SimplnxCore/test/CMakeLists.txt` (test registration + archive download)

## Algorithm Relationship

- **Tentative classification:** **Port (with rename and reorganization)** — the SIMPLNX filter is a translation of the legacy SIMPL `FindGroupingDensity` filter (UUID `708be082-8b08-4db2-94be-52781ed4d53d`); it has been renamed `ComputeGroupingDensity` in SIMPLNX following the platform-wide `Find* → Compute*` naming convention. A `FromSIMPLJson()` conversion path is implemented and a SIMPL 6.5 backwards-compat fixture exercises it.
- **Evidence:**
  - `SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter's SIMPLNX UUID.
  - The SIMPL 6.5 conversion fixture (`test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json`) declares `Filter_Name: FindGroupingDensity` and translates 9 SIMPL parameter keys to 9 SIMPLNX parameter keys via `SIMPLConversion::ConvertParameter` calls in `FromSIMPLJson()`.
  - The algorithm structure (parent-loop → feature-loop → contiguous-neighbor-loop → optional-non-contiguous-neighbor-loop → density = parentVolume / totalCheckedVolume) is consistent with the legacy DREAM3D `FindGroupingDensity` design described in the docs and the worked example.
- **Action required:** Confirm by reading the corresponding SIMPL filter source (`FindGroupingDensity.cpp` in DREAM3D 6.5.x) and running the `compare-legacy-dream3d` skill against a shared toy dataset. Note any algorithmic deviations (e.g., the `FindCheckedFeatures` "max-parent-volume wins" tie-breaking rule should be verified to match legacy).

## PRs inspected (since 2025-10-01)

> The filter did not exist before 2026-02-25. Only **two PRs in the entire git history** touch this filter:
>
> - **#1548** — initial creation (filter, algorithm, doc, test, legacy UUID map entry)
> - **#1588** — added the per-filter SIMPL backwards-compat test
>
> No broad-refactor PRs (e.g., #1457, #1472, #1501, #1535, #1571, #1582) touched this filter, presumably because they all merged before #1548 or because their sweeps did not reach this newly-added file. There is therefore no "pruned PRs" table at the bottom — there are no pruned PRs.

### PR #1548 — *"FILT: Compute Grouping Density filter added."* — merged 2026-02-25

- **Files in this filter:** filter (.hpp +126, .cpp +278), algorithm (.hpp +52, .cpp +219), doc (.md +90), three doc images (Algorithm.png, FeatureIds.png, ParentIds.png), legacy UUID map (+3 lines), plugin CMakeLists (+2), test CMakeLists (+2), unit test (.cpp +431).
- **Diff size:** 12 files, +1203 lines, –0.
- **Change nature:** **New filter / Port.** Initial introduction of `ComputeGroupingDensityFilter` plus its algorithm, documentation (with worked example and three illustrative images), `FromSIMPLJson()` conversion path, legacy UUID map entry, and a 431-line test file containing one exemplar-based test, four execution tests covering all four template specializations of `FindDensityGrouping<UseNonContiguousNeighbors, FindCheckedFeatures>`, and three preflight error tests. The filter is implemented as the standard Filter+Algorithm pair per the project convention and uses `MessageHelper`/`ThrottledMessenger` for progress reporting and a `m_ShouldCancel` check inside the parent loop.
- **V&V content:** **High** for a single-PR introduction.
  - **Algorithmic verification (Step 0 c):** The `Basic Density (contiguous, no checked features)` test contains **explicit hand-derived expected values** (`45/70 = 0.6429`, `55/70 = 0.7857`) with the derivation written in code comments — this is a textbook Class-1 analytical oracle.
  - **Code path coverage:** All four `(UseNonContiguousNeighbors × FindCheckedFeatures)` template specializations exercised by separate `TEST_CASE`s with hand-derived expected outputs.
  - **Preflight coverage:** Three negative tests covering tuple-count mismatch, feature-volumes-not-in-AM, and parent-volumes-not-in-AM.
  - **Exemplar archive:** `compute_grouping_densities.tar.gz` registered in `test/CMakeLists.txt` with SHA512.
  - **Doc:** Worked example with explicit numbers and three diagrams.
- **Notes / surprises:**
  - Despite being labeled `[SimplnxReview]` in the test tags (5 of 8 `TEST_CASE`s) — likely a copy-paste from the `SimplnxReview` plugin where this filter may have been prototyped — the filter actually lives in `SimplnxCore`. The 8th (backcompat) test correctly uses `[SimplnxCore]`. **Tag inconsistency to flag for cleanup.**
  - The test file uses both an exemplar comparison **and** explicit hand-derived constants in the same test — this is the gold-standard pattern for the V&V policy and should be highlighted.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +49 lines, plus one new fixture file:
  - `test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json` (+42 lines)
- **Diff size:** 2 files, +91 / -0.
- **Change nature:** **Test addition.** Added the per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises SIMPL 6.5 (UUID-mapped) pipeline conversion. The test verifies the legacy `FindGroupingDensity` UUID `708be082-8b08-4db2-94be-52781ed4d53d` round-trips into the SIMPLNX `ComputeGroupingDensityFilter` and that all 9 mapped parameter values arrive at the right keys with the right `DataPath`s. Test name: `"SimplnxCore::ComputeGroupingDensityFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy `FindGroupingDensity` output. That is still missing.
- **Notes:**
  - There is **no SIMPL 6.4 fixture** for this filter (only 6.5). This makes sense because the legacy filter only used the new (6.5) UUID-based path; the 6.4 `Filter_Name`-fallback is not exercised. Engineer to confirm this is intentional or whether a 6.4 fixture should be added.

### Pruned PRs

None. Filter did not exist when the broad-refactor PRs (#1457, #1472, #1501, #1535, #1571, #1582) merged.

## Test coverage detected

`ComputeGroupingDensityTest.cpp` contains **8 `TEST_CASE`s**:

1. `SimplnxReview::ComputeGroupingDensityFilter: Basic Density (contiguous, no checked features)` — exemplar-based test that loads `compute_grouping_densities.dream3d`, runs the filter, and compares **both** against the exemplar `Float32Array` **and** against hand-derived constants (`45/70` and `55/70`). *(Tag should be `[SimplnxCore]` not `[SimplnxReview]`.)*
2. `SimplnxReview::ComputeGroupingDensityFilter: Contiguous Only, No Checked Features` — `(useNonContiguous=false, findCheckedFeatures=false)` — fully synthetic 5-feature / 2-parent fixture, verifies hand-derived densities.
3. `SimplnxReview::ComputeGroupingDensityFilter: With Non-Contiguous Neighbors` — `(true, false)` — verifies hand-derived densities `45/100` and `55/100` when non-contiguous neighbors expand the checked set to all 5 features.
4. `SimplnxReview::ComputeGroupingDensityFilter: With Checked Features` — `(false, true)` — verifies the `CheckedFeatures` output array reflects the "largest-parent-volume wins" tie-breaking rule (expected `[0,1,1,2,2,2]`).
5. `SimplnxReview::ComputeGroupingDensityFilter: Both Options Enabled` — `(true, true)` — combined verification (densities + checked-features assignment when all features are reachable from both parents).
6. `SimplnxReview::ComputeGroupingDensityFilter: Preflight Error - Feature tuple count mismatch` — preflight returns `INVALID` when ParentIds is in an AM with a different tuple count than Volumes.
7. `SimplnxReview::ComputeGroupingDensityFilter: Preflight Error - Volumes not in AttributeMatrix` — preflight returns `INVALID` when Feature Volumes is not parented by an `AttributeMatrix`.
8. `SimplnxReview::ComputeGroupingDensityFilter: Preflight Error - Parent Volumes not in AttributeMatrix` — preflight returns `INVALID` when Parent Volumes is not parented by an `AttributeMatrix`.
9. `SimplnxCore::ComputeGroupingDensityFilter: SIMPL Backwards Compatibility` — SIMPL 6.5 conversion path via `DYNAMIC_SECTION` *(added by PR #1588)*.

Tests 2–5 fully cover the 2×2 `(UseNonContiguousNeighbors × FindCheckedFeatures)` cross-product. Tests 6–8 are negative-path preflight validation. Test 1 is the only exemplar-based test. Test 9 is conversion-only.

## Exemplar archive

- **Archive name:** `compute_grouping_densities.tar.gz`
- **SHA512:** `96066196d6aa5f87cc7b717f959848c2f3025b7129589abe1eded2a8d725c539a89b0a6290a388a56b5a401e0bd3041698fbd8e8cf37a1f18fdd937debd21531`
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt` (line ~288)
- **Test data dir:** `compute_grouping_densities/` (extracted into `unit_test::k_TestFilesDir`)
- **Files expected inside:** `compute_grouping_densities.dream3d` (exemplar) — contains the 20×5 worked-example geometry, with an exemplar `GroupingDensities (false, false)` array stored at `ImageGeom/ParentFeatureData/GroupingDensities (false, false)`.
- **Provenance:** *(TBD — no `ReadMe.md` was inspected inside the archive.)* The exemplar was almost certainly generated by running a DREAM3D-NX pipeline that reproduces the worked example. The exemplar variant naming `(false, false)` strongly suggests the archive may also contain `(true, false)`, `(false, true)`, `(true, true)` exemplars — Test 1 only exercises the first.
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` file used to seed the pipeline, the `.d3dpipeline` file that produced the exemplar, and any provenance notes. Promote into the verification archive ReadMe per Step 0's Oracle Provenance policy. **Strongly consider expanding Test 1** to also compare exemplars for the other three variants — they are likely already in the archive given the naming convention.

## Oracle classification (tentative)

- **Recommended class:** **Class 1 (Analytical / closed-form)** — primary, with optional **Class 4 (Invariant-based)** as a companion check.
- **Rationale:**
  - The algorithm computes a closed-form scalar per parent: `density(p) = ParentVolume(p) / Σ FeatureVolume(f)`, where `f` ranges over (children of p) ∪ (contiguous neighbors of children) ∪ (optionally non-contiguous neighbors of children), each `f` counted exactly once.
  - This is **directly hand-derivable** for any toy fixture — the existing tests already do this for both options on/off and prove the implementation matches.
  - Companion invariants for Class 4 are also straightforward: density values are non-negative *or* the sentinel `-1.0`; for any non-empty parent, density ≤ 1.0 if every neighbor is outside the parent (loose bound — actually density ≤ 1 + maxOverlapTerm; engineer to defend the precise invariant); `density == -1.0` iff the parent has zero children matching its `ParentId`.
  - **Class 3 (Paper-based)** is *probably not* applicable — there is no canonical literature reference for "grouping density" cited in the doc or in the algorithm header. The metric is a SIMPL/DREAM3D-internal microtexture-region (MTR) helper, intended to feed downstream MTR analysis. Engineer to confirm whether a Pilchak/Williams or Bridier reference describes this exact ratio in the dwell-fatigue/MTR literature; if so, upgrade to Class 3.
- **Action required:** Developer to confirm Class 1 (recommended starting point) is acceptable. Codify the existing hand-derived expected values (`45/70`, `55/70`, `45/100`, `55/100`) as the formal oracle, and add a short Class-4 invariant block to the test (e.g., `REQUIRE(d == -1.0f || d >= 0.0f)`).

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. Algorithm is short (~140 lines including the templated `FindDensityGrouping` worker) and looks clean — `ThrottledMessenger`, cancel check inside parent loop, no parallel/data-race concerns (single-threaded). |
| Code path coverage (algorithmic) | **Excellent** | 4 tests cover the 2×2 `(UseNonContiguousNeighbors × FindCheckedFeatures)` cross-product **with hand-derived expected values**. |
| Code path coverage (preflight error paths) | Good | 3 negative tests for tuple mismatch and AM-parent requirement. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.5 conversion test. (No 6.4 fixture — engineer to confirm intentional.) |
| Exemplar data in Data_Archive | **Yes** | `compute_grouping_densities.tar.gz` registered with SHA512. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. Class 1 (Analytical) recommended given existing hand-derived test constants. |
| Toy data / independent expected output (Step 0 c) | **Yes** | Test file contains hand-derived constants `45/70`, `55/70`, `45/100`, `55/100` and explicit derivations in code comments. The doc contains the same worked example. |
| Legacy comparison report (Step 0 e) | No | `compare-legacy-dream3d` has not been run against `FindGroupingDensity` in DREAM3D 6.5.171. |
| Deviation entries (`ComputeGroupingDensity-D<N>`) | None | Not yet written. None expected from the source — the algorithm is short, has no obvious bug surface, and no fix-PRs have touched it since creation. |
| Documentation currency | **Excellent** | Doc was authored at filter creation (#1548); includes worked example, three diagrams, parameter descriptions, and an interpretation table. Empty "References" section is the one defect. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

> Because this filter is a **Port** of legacy `FindGroupingDensity`, the Legacy Comparison step (Step 0 e) **does** apply.

1. **Confirm the oracle.** Class 1 (Analytical) is the strong starting point given the closed-form definition and the existing hand-derived test constants. Codify a short Class-4 invariant block in the test as a companion. Investigate whether a Pilchak/Williams/Bridier MTR paper describes this density metric — if so, upgrade to Class 3.
2. **Promote the existing `[SimplnxReview]` tags to `[SimplnxCore]`.** The first 7 `TEST_CASE`s use the wrong tag, presumably from a prototype location. This is a low-risk single-line cleanup but matters for `ctest -R "SimplnxCore::"` discoverability and for the test policy. *(Defer this to a small follow-up PR; do not modify in this audit.)*
3. **Inspect `compute_grouping_densities.tar.gz` and document provenance.** Determine how the exemplar was generated, whether a SIMPLNX or SIMPL pipeline produced it, and what the input data was. Promote any other variant exemplars (`(true, false)`, `(false, true)`, `(true, true)`) into Test 1 — the variant suffix in the name `GroupingDensities (false, false)` strongly suggests siblings exist in the archive.
4. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.171 `FindGroupingDensity` on a shared dataset (the worked example would be a good toy candidate; a real microtexture pipeline output is the realistic test). Document any discrepancies as Deviation entries.
5. **Add a 6.4 SIMPL conversion fixture** *if* legacy `FindGroupingDensity` was reachable via the `Filter_Name`-fallback path in 6.4 pipelines. Otherwise document why 6.4 is intentionally skipped.
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of SIMPL `FindGroupingDensity` (UUID `708be082-...`); renamed to `ComputeGroupingDensity` per the SIMPLNX `Find* → Compute*` convention; output and parameter semantics intended to match legacy. No bug fixes applied since creation."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder, including the archive `.tar.gz`, the worked-example pipeline, hand-derivation notes, and the legacy-comparison results.

## Recommended Deviation entries (proposed, pending legacy comparison)

> No Deviation entries are pre-known from the git history — there are no bug-fix PRs against this filter since creation, and the filter has only existed for ~2 months. The list below is a placeholder that will be populated only if `compare-legacy-dream3d` finds discrepancies.

> **Deviation ID:** `ComputeGroupingDensity-D1` *(placeholder — to be populated only if legacy comparison finds a difference)*
> **Filter UUID:** `ff46afcf-de32-4f37-98bc-8f0fd4b3c122`
> **Symptom:** *(TBD)* — likely candidates if any: (a) `CheckedFeatures` tie-breaking rule (max-parent-volume vs. first-wins) differs between legacy and SIMPLNX; (b) order of insertion into `totalFeatureCheckList` differs in a way that affects `outCheckedFeatures` when two parents have equal volume; (c) sentinel value (`-1.0` for empty parent) differs between legacy and SIMPLNX.
> **Root cause:** TBD by comparison.
> **Affected users:** Anyone using grouping-density output downstream in MTR pipelines.
> **Recommendation:** TBD.
> **Status:** Placeholder — pending `compare-legacy-dream3d` run.
