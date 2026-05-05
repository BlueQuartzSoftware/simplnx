# V&V Working Document: ComputeGroupingDensityFilter

*Status:* **DRAFT — Phase 1 (Discovery) complete; Phases 2–13 are work orders for the engineer to execute.**

This document is the engineer's working V&V doc for `ComputeGroupingDensityFilter`. It follows the workflow in `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`. This is a **retroactive promotion pass** — a tentative retroactive V&V report already exists at `docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md` and most discovery work and tentative classifications are pre-populated below.

**How to use this document:**
- Each phase has a **Goal**, **Required reading**, **Tasks** (with checkboxes you literally check off as you go), and **Exit criteria** (when you can move to the next phase).
- Where a "starting point" is provided, you can confirm it or replace it — not start from a blank page.
- Update this document as you work. It IS the deliverable.
- When all phases are complete, follow Phase 13 to update the Status line at the top to "COMPLETE". The file stays at this location.

**Source-of-truth references** (you'll consult these throughout — paths are relative to this doc's location at `src/Plugins/SimplnxCore/vv/`):
- Policy: [`docs/vv_templates/mtr_filter_verification_validation.md`](../../../../docs/vv_templates/mtr_filter_verification_validation.md)
- Oracle quick-reference: [`docs/vv_templates/oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md)
- Audit cross-cutting findings: [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md) "Cross-cutting findings" section
- Retroactive V&V report (this filter): [`docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md`](../../../../docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md)
- Skill (workflow): `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`

---

## Phase 1 — Discovery *(complete — verify findings below)*

**Goal**: locate every file related to this filter and identify the legacy DREAM3D equivalent.

### Tasks (verify the discovery findings)

- [ ] Confirm the metadata table below is accurate (UUID, plugin, legacy SIMPL UUID + ClassName)
- [ ] Confirm every file in the "Source files" list still exists at the cited path
- [ ] Confirm the test exemplar archive name + SHA512 by running `grep -A2 compute_grouping_densities src/Plugins/SimplnxCore/test/CMakeLists.txt`
- [ ] Confirm the TEST_CASE inventory matches the test file by running `grep TEST_CASE src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp`
- [ ] Confirm the material PR list — re-run `git log --since=2025-10-01 --oneline -- <files>` and check no new PRs landed since this doc was written
- [ ] Read the algorithm summary block and confirm it matches your understanding of `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.cpp`

### Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `ff46afcf-de32-4f37-98bc-8f0fd4b3c122` |
| SIMPLNX ClassName | `ComputeGroupingDensityFilter` |
| SIMPLNX Human Name | Compute Grouping Densities |
| SIMPL UUID | `708be082-8b08-4db2-94be-52781ed4d53d` *(via `SimplnxCoreLegacyUUIDMapping.hpp`)* |
| SIMPL ClassName | `FindGroupingDensity` *(per SIMPL backwards-compat fixture)* |
| SIMPL Human Name | "Find Grouping Density" |
| Plugin | SimplnxCore |
| Tier | Tier-1 (in MTR SBIR list) |
| V&V Mode | Retroactive promotion pass — retroactive report exists at `docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md` |

### Source files

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeGroupingDensityFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json` *(no 6_4 fixture — confirm intentional in Phase 9)*
- `src/Plugins/SimplnxCore/docs/ComputeGroupingDensityFilter.md`
- `src/Plugins/SimplnxCore/docs/Images/ComputeGroupingDensity_FeatureIds.png`
- `src/Plugins/SimplnxCore/docs/Images/ComputeGroupingDensity_ParentIds.png`
- `src/Plugins/SimplnxCore/docs/Images/ComputeGroupingDensity_Algorithm.png`
- `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` (legacy UUID map entry)
- `src/Plugins/SimplnxCore/test/CMakeLists.txt` (test registration + archive download)
- No Python binding found

### Test exemplar archive

- **Archive name**: `compute_grouping_densities.tar.gz`
- **SHA512**: `96066196d6aa5f87cc7b717f959848c2f3025b7129589abe1eded2a8d725c539a89b0a6290a388a56b5a401e0bd3041698fbd8e8cf37a1f18fdd937debd21531`
- **Test data dir**: `compute_grouping_densities/` (extracted into `unit_test::k_TestFilesDir`)
- **Files expected inside**: `compute_grouping_densities.dream3d` containing the 20×5 worked-example geometry, with an exemplar `GroupingDensities (false, false)` array stored at `ImageGeom/ParentFeatureData/GroupingDensities (false, false)`. **The variant naming `(false, false)` strongly suggests sibling exemplars `(true, false)`, `(false, true)`, `(true, true)` may also be in the archive** — Test 1 only consumes the first; Phase 10 should investigate and expand exemplar coverage if siblings exist.
- **Provenance status**: **Unknown** — no `ReadMe.md` was inspected inside the archive. See Phase 2 investigation tasks.

### TEST_CASE inventory (in `ComputeGroupingDensityTest.cpp`, 9 total)

| # | Name | Tag(s) | Coverage |
|---|---|---|---|
| 1 | `Basic Density (contiguous, no checked features)` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Exemplar-based, also asserts hand-derived `45/70`, `55/70` |
| 2 | `Contiguous Only, No Checked Features` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Synthetic, hand-derived densities |
| 3 | `With Non-Contiguous Neighbors` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Synthetic, hand-derived `45/100`, `55/100` |
| 4 | `With Checked Features` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Verifies `CheckedFeatures` "largest-parent-volume wins" tie-break |
| 5 | `Both Options Enabled` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Combined verification |
| 6 | `Preflight Error - Feature tuple count mismatch` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Negative-path |
| 7 | `Preflight Error - Volumes not in AttributeMatrix` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Negative-path |
| 8 | `Preflight Error - Parent Volumes not in AttributeMatrix` | `[SimplnxReview][ComputeGroupingDensityFilter]` ⚠️ | Negative-path |
| 9 | `SIMPL Backwards Compatibility` | `[SimplnxCore][ComputeGroupingDensityFilter][BackwardsCompatibility]` ✓ | SIMPL 6.5 conversion path |

⚠️ **Test-tag bug**: 8 of 9 tests use `[SimplnxReview]` tag (the filter was likely prototyped in `SimplnxReview` plugin before being moved). Phase 8 should fix these tags to `[SimplnxCore]`. This is a low-risk single-line cleanup but matters for `ctest -R "SimplnxCore::"` discoverability.

### Material PRs since 2025-10-01

The filter did not exist before 2026-02-25. Only **two PRs in the entire git history** touch this filter — no broad-refactor PRs reached it because they merged before #1548 or didn't touch newly-added files:

| PR | Date | Title | Notes for this filter |
|---|---|---|---|
| #1548 | 2026-02-25 | FILT: Compute Grouping Density filter added. | Initial introduction of filter + algorithm + doc + 8 tests + legacy UUID map entry. **+1203 lines, –0**. Filter authored by hand-derived expected values in tests — gold-standard Class-1 oracle pattern. |
| #1588 | 2026-04-22 | ENH: SIMPL Backwards Compatibility Test Redesign | Added per-filter SIMPL 6.5 conversion test + 6.5 fixture JSON. No 6.4 fixture (engineer to confirm intentional in Phase 9). |

### Algorithm summary (from source inspection of `Algorithms/ComputeGroupingDensity.cpp`)

For each parent (microtexture region), compute the closed-form density:

```
density(p) = ParentVolume(p) / Σ FeatureVolume(f)
```

where `f` ranges over (children of p) ∪ (contiguous neighbors of children) ∪ (optionally, non-contiguous neighbors of children), each `f` counted exactly once via a `totalFeatureCheckList` set.

**Parameters / template specializations**:
- `UseNonContiguousNeighbors` (bool): includes non-contiguous neighbors in the sum
- `FindCheckedFeatures` (bool): writes a per-feature `CheckedFeatures` array indicating which parent claimed each feature (largest-parent-volume wins)

These give 4 template specializations: `(false,false)`, `(true,false)`, `(false,true)`, `(true,true)`. All 4 are exercised by Tests 2–5 with hand-derived expected values.

**Sentinel**: density = `-1.0f` for parents with no children matching the parent's `ParentId`.

**Threading**: Single-threaded (no `ParallelDataAlgorithm`). Cancel check inside parent loop. `ThrottledMessenger` for progress reporting.

### Exit criteria (when you can move to Phase 2)

- ✅ All Phase 1 verification checkboxes above are checked
- ✅ Any new findings or corrections are noted in the metadata, source files, or PR list
- ✅ `git log` confirms no new PRs landed on the filter's files since this doc was generated

---

## Phase 2 — Promote existing work product

**Goal**: inventory what V&V evidence already exists for this filter so you don't redo work that's been done. This filter is **richly pre-populated** — the audit identified multiple promotable artifacts.

**Required reading**:
- Policy lines 248–262 — "Promote existing work product — don't restart"

### Tasks

- [ ] **Investigate provenance of `compute_grouping_densities.tar.gz`**:
  - [ ] Find when and by whom it was generated (`git log --all --follow -- src/Plugins/SimplnxCore/test/CMakeLists.txt | grep -B2 compute_grouping_densities`)
  - [ ] Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` file used to seed the pipeline, the `.d3dpipeline` file that produced the exemplar, any provenance notes
  - [ ] Determine which version produced the embedded `GroupingDensities (false, false)` array (likely SIMPLNX since the filter is new in SIMPLNX, but confirm)
  - [ ] **Check for sibling exemplars**: the variant naming `(false, false)` suggests `(true, false)`, `(false, true)`, `(true, true)` may also be in the archive. If so, Phase 10 should expand Test 1 to consume all four.
- [ ] **Inventory promotable artifacts** in the table below. For each, record promote / augment / replace decision:

| Existing artifact | Candidate promotion class | Decision (promote/augment/replace) | Notes |
|---|---|---|---|
| Hand-derived constants in tests (`45/70`, `55/70`, `45/100`, `55/100`) with derivation comments in code | **Class 1 (Analytical)** — gold standard | _______________ | Already in test source. Just needs to be formally recognized as the Class-1 oracle in Phase 4. |
| Exemplar `compute_grouping_densities.tar.gz` | Class 5 (legacy expert) OR Class 2 (SIMPLNX-pipeline-generated) | _______________ | Decision depends on provenance investigation above |
| Doc worked example with explicit numbers + 3 diagrams (`Images/ComputeGroupingDensity_*.png`) | Class 1 augmentation (the doc IS the hand-derivation) | _______________ | Already published to users. Phase 11 will link Phase-9 deviations from this doc. |
| Existing `[SimplnxReview]` tag on 8 of 9 tests | N/A — known bug to fix | **Replace with `[SimplnxCore]`** in Phase 8 | Single-line cleanup. Do NOT modify in this audit; defer to Phase 8 fix. |

- [ ] **Look for paper references in source**: grep `Algorithms/ComputeGroupingDensity.cpp` and the `.hpp` for citation comments, DOI strings, or `@reference` doxygen tags. The audit did not find any:
  > *Paper references found:* (none observed by audit; engineer to confirm)
- [ ] **Investigate whether the metric has a literature reference**: this filter computes a microtexture-region density used in α/β titanium dwell-fatigue analysis. Check Pilchak/Williams or Bridier (or related MTR papers) for a "grouping density" definition. If found, oracle class upgrades from Class 1 to Class 3:
  > *Literature search result:*
- [ ] **Look for prior comparison notes**: check OneDrive for any folder named `ComputeGroupingDensity*` or `FindGroupingDensity*`. Check Slack/email archives:
  > *Found notes:*

### Exit criteria (when you can move to Phase 3)

- ✅ Provenance of `compute_grouping_densities.tar.gz` is determined
- ✅ Sibling exemplar variants (`(true,false)`, `(false,true)`, `(true,true)`) confirmed present or absent in the archive
- ✅ Promotion table above is filled in for each artifact
- ✅ Source has been searched for paper references (and any found are recorded)
- ✅ Literature search for MTR-density references completed (with result recorded)
- ✅ OneDrive/Slack/email checked for prior notes

---

## Phase 3 — Algorithm Relationship classification

**Goal**: classify how SIMPLNX relates to the legacy DREAM3D 6.5.172 implementation. This becomes the opening line of the eventual Phase 9 legacy comparison report.

**Required reading**:
- Policy lines 98–113 — full description of the four classifications (Port / Minor / Rewrite / New filter)

### Tasks

- [ ] **Open the legacy SIMPL filter source** for `FindGroupingDensity` in DREAM3D 6.5.x. Compare the algorithm body to the SIMPLNX `Algorithms/ComputeGroupingDensity.cpp`.
- [ ] **Verify the `FromSIMPLJson()` parameter mapping** in `ComputeGroupingDensityFilter.cpp` against the SIMPL fixture at `test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json`. The audit confirmed 9 SIMPL parameter keys map to 9 SIMPLNX keys via `SIMPLConversion::ConvertParameter` calls.
- [ ] **Pay particular attention to the tie-breaking rule** in `FindCheckedFeatures` ("largest-parent-volume wins"). Verify the legacy `FindGroupingDensity` uses the same rule. This is a likely Deviation candidate if rules differ.
- [ ] **Pick the classification** and record it here:
  - [ ] Port
  - [ ] Minor changes
  - [ ] Rewrite
  - [ ] New filter, no legacy equivalent
- [ ] **Write the opening line** for the eventual Phase 9 report. Use the format from policy lines 107–113. Confirm or replace the suggested starting point below:
  > *Suggested starting point* — *Algorithm Relationship: **Port** — direct translation of SIMPL `FindGroupingDensity` (UUID `708be082-…`); renamed to `ComputeGroupingDensity` per the SIMPLNX `Find* → Compute*` convention; output and parameter semantics intended to match legacy. No bug fixes applied since creation.*
  >
  > *Your final opening line:*

### Recommended starting point (from audit)

**Tentative classification: Port (with rename and reorganization)**.

Evidence supporting Port:
- `SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter's SIMPLNX UUID
- The SIMPL 6.5 conversion fixture maps 9 SIMPL parameter keys to 9 SIMPLNX parameter keys via `SIMPLConversion::ConvertParameter` calls in `FromSIMPLJson()`
- The algorithm structure (parent-loop → feature-loop → contiguous-neighbor-loop → optional-non-contiguous-neighbor-loop → density = parentVolume / totalCheckedVolume) is consistent with the legacy DREAM3D `FindGroupingDensity` design

### Exit criteria (when you can move to Phase 4)

- ✅ SIMPL `FindGroupingDensity` source has been read and compared to SIMPLNX algorithm
- ✅ `FromSIMPLJson()` parameter mapping verified against the 6.5 conversion fixture
- ✅ Tie-breaking rule in `FindCheckedFeatures` cross-checked against legacy
- ✅ Classification chosen and checked
- ✅ Opening line written and recorded above

---

## Phase 4 — Oracle classification

**Goal**: pick the oracle class(es) that will define "correct" for this filter, independent of any DREAM3D version.

**Required reading**:
- [`docs/vv_templates/oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md) — one-page summary of all 5 classes
- Policy lines 25–82 — full text on oracle classes and provenance

### Tasks

- [ ] **Read the algorithm source** (`Algorithms/ComputeGroupingDensity.cpp`) and confirm the closed-form definition: `density(p) = ParentVolume(p) / Σ FeatureVolume(f)` where `f` ranges over the union described in Phase 1.
- [ ] **Walk through each of the 5 classes** (using the quick-reference) and decide if it's a candidate oracle:
  - [ ] **Class 1 (Analytical)** — Closed-form definition admits hand calculation on toy data. **The existing test source already contains this** with derivations as code comments. **Strongly applies — this is the recommended primary class.**
  - [ ] **Class 2 (Reference implementation)** — No clear external library implements "grouping density". *(Likely not applicable.)*
  - [ ] **Class 3 (Paper-based)** — Engineer to investigate during Phase 2 literature search. If a Pilchak/Williams/Bridier paper defines this density metric, upgrade primary class from 1 → 3 and embed the paper in the archive.
  - [ ] **Class 4 (Invariant-based)** — Multiple natural invariants. **Recommended companion class.** See list below.
  - [ ] **Class 5 (Expert-visual)** — Not needed.
- [ ] **Pick a primary class and any companion classes**. The recommended stack for this filter is **Class 1 (primary) + Class 4 (companion)**, with potential upgrade to Class 3 if the literature search in Phase 2 finds a citable reference. Confirm or replace:
  > *Your decision:*
- [ ] **Identify a second engineer to review your oracle choice**. Per policy line 39, oracle design "really should" be reviewed by a second engineer:
  - [ ] Reviewer identified: name + date scheduled = ___________
  - [ ] OR skip review (no second engineer available): write the skip reason here. It will be copied into the Phase 12 archive ReadMe per policy line 39.
    > *Skip reason:*

### Recommended oracle stack for this filter (starting point)

**Primary: Class 1 (Analytical)** — the closed-form `density = ParentVolume / Σ FeatureVolume` is hand-derivable for any toy fixture. The existing tests (TEST_CASEs 2–5) **already use this oracle** with explicit hand-derived constants `45/70`, `55/70`, `45/100`, `55/100`. The work for this filter is to **formalize** this as the oracle of record, not to design new oracle artifacts.
- *Provenance requirement (Class 1)*: none — the oracle lives in the test code with derivation comments.

**Companion: Class 4 (Invariant)** — natural invariants, all assertable directly in test code:
- `density(p) >= 0.0f` OR `density(p) == -1.0f` (sentinel for empty parent)
- `density(p) <= 1.0f + ε` for parents whose neighbors don't cross outside (loose bound — engineer to defend the precise invariant)
- `density(p) == -1.0f` IFF the parent has zero children matching its `ParentId`
- `len(GroupingDensities) == numParents`
- `len(CheckedFeatures) == numFeatures` (when `FindCheckedFeatures = true`)
- `CheckedFeatures[f] ∈ {0, 1, ..., numParents-1}` (each feature claimed by exactly one parent)

**Possible upgrade: Class 3 (Paper-based)** — only if the Phase 2 literature search finds a citable MTR-density reference. If so:
- Add DOI + edition + equation # + page # to archive Oracle Provenance block
- Embed paper PDF in archive

### Exit criteria (when you can move to Phase 5)

- ✅ Primary oracle class chosen and recorded
- ✅ Companion classes (if any) chosen and recorded
- ✅ Class 5 justification recorded if applicable (NOT applicable here)
- ✅ Second-engineer reviewer identified, OR skip reason recorded
- ✅ Phase 12 archive checklist updated with: oracle class, second-engineer review status

---

## Phase 5 — Toy data design + independent expected output

**Goal**: design minimum-size synthetic inputs that exercise each code path, and compute the expected output for each one *without running any DREAM3D version*. **This filter has a head-start** — the existing test file already contains the fixtures and expected outputs.

**Required reading**:
- Policy lines 41–64 — Step 0 a–c (the strict ordering: classify → toy data → independent oracle computation)

**Critical discipline**: Do **NOT** run SIMPLNX or DREAM3D on these fixtures during Phase 5. The expected output must be derived from the oracle alone (paper / hand calculation / invariant predicate). For this filter, the existing tests already enforced this discipline at creation; your job is to formalize and extend.

### Tasks

- [ ] **Catalog the existing fixtures** in the table below. They're already in the test file — promote them to the formal oracle.
- [ ] **For each existing fixture, confirm or correct the hand-derivation comment** in the test code so the oracle is self-documenting.
- [ ] **Add Class-4 invariant fixtures** if not present. The existing tests probably already cover these implicitly; make them explicit.
- [ ] **Save oracle artifacts** into a working folder for this filter at `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter_artifacts/`:
  - [ ] Hand-derivation notes for each fixture (markdown — copy from test source comments and elaborate)
  - [ ] Per-fixture expected-output table (markdown or JSON)
  - [ ] If a paper reference is found in Phase 2: embed PDF + cite equation #
- [ ] **If sibling exemplar variants are confirmed in Phase 2** (`(true,false)`, `(false,true)`, `(true,true)`), document each as an additional Class-2 fixture in the table.

### Existing fixtures (starting point — already in test source)

| ID | Description | Class | Source location | Expected output |
|---|---|---|---|---|
| A | Basic Density (contiguous, no checked features) — exemplar comparison | 1 + 2 (exemplar) | TEST_CASE 1 | `45/70 ≈ 0.6429`, `55/70 ≈ 0.7857`; matches exemplar `GroupingDensities (false, false)` |
| B | Contiguous Only, No Checked Features — fully synthetic | 1 | TEST_CASE 2 | Hand-derived densities, see test comments |
| C | With Non-Contiguous Neighbors | 1 | TEST_CASE 3 | `45/100 = 0.45`, `55/100 = 0.55` |
| D | With Checked Features (tie-breaking rule) | 1 + 4 | TEST_CASE 4 | `CheckedFeatures = [0,1,1,2,2,2]` (largest-parent-volume wins) |
| E | Both Options Enabled | 1 + 4 | TEST_CASE 5 | Combined verification |
| F | Preflight Error - Feature tuple count mismatch | 4 (negative-path invariant) | TEST_CASE 6 | preflight INVALID |
| G | Preflight Error - Volumes not in AttributeMatrix | 4 (negative-path invariant) | TEST_CASE 7 | preflight INVALID |
| H | Preflight Error - Parent Volumes not in AttributeMatrix | 4 (negative-path invariant) | TEST_CASE 8 | preflight INVALID |

### Recommended additional Class-4 invariant fixtures to add

- [ ] **Sentinel invariant**: parent with no matching children → `density == -1.0f`
- [ ] **Empty-feature-set invariant**: parent whose total checked volume is 0 → `density == -1.0f` (or whatever sentinel the algorithm produces)
- [ ] **Cardinality invariant**: `len(GroupingDensities) == numParents`
- [ ] **CheckedFeatures range invariant**: every value in `[0, numParents-1]`

### Exit criteria (when you can move to Phase 6)

- ✅ Existing fixtures cataloged with hand-derivations
- ✅ Class-4 invariant fixtures designed (test code TBD in Phase 8)
- ✅ Working artifact folder created with hand-derivations and fixture tables
- ✅ Sibling exemplar variants (if found in Phase 2) documented as additional Class-2 fixtures
- ✅ Second-engineer review (if scheduled in Phase 4) signed off on the oracle artifacts

---

## Phase 6 — SIMPLNX vs oracle reconciliation

**Goal**: confirm SIMPLNX matches the Phase-5 oracle. **For this filter, the existing tests already do this** — but you must explicitly confirm and run them as the formal Phase-6 reconciliation gate.

**Required reading**:
- Policy lines 46–47 — Step 0d ("Run DREAM3DNX/SIMPLNX on the toy data and compare against the oracle. Resolve any discrepancy in SIMPLNX before moving on.")

### Tasks

- [ ] **Run the existing test suite** scoped to this filter:
  ```bash
  cd /Users/mjackson/Workspace2/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel
  ctest -R "ComputeGroupingDensityFilter" --verbose
  ```
- [ ] **Confirm all 8 algorithmic tests + the SIMPL backcompat test pass** on `develop`.
- [ ] **If any test fails**: this is a regression that wasn't present at PR #1548 merge. Investigate and fix in this V&V pass.
- [ ] **If all tests pass**: SIMPLNX is verified-correct against the Phase-4 oracle for the existing fixtures. Move to Phase 7.
- [ ] **For any new Class-4 invariant fixtures designed in Phase 5** (if not already in the test suite): write them as TEST_CASEs, run, confirm pass before Phase 7.
- [ ] **No bugs are pre-flagged for this filter** by the audit — the algorithm is short (~140 lines), single-threaded, and has no obvious bug surface. If you find any during reconciliation, follow the standard fix → failing-test → green-test cycle.

### Reconciliation table

| Fixture ID | Expected (oracle) | SIMPLNX actual | Match? | Action / resolution |
|---|---|---|---|---|
| A — Basic Density | `45/70`, `55/70` + exemplar | _______ | _______ | _______ |
| B — Contiguous Only | per test comments | _______ | _______ | _______ |
| C — Non-Contiguous | `45/100`, `55/100` | _______ | _______ | _______ |
| D — CheckedFeatures | `[0,1,1,2,2,2]` | _______ | _______ | _______ |
| E — Both Options | combined | _______ | _______ | _______ |
| F — Preflight: tuple mismatch | INVALID | _______ | _______ | _______ |
| G — Preflight: Volumes not in AM | INVALID | _______ | _______ | _______ |
| H — Preflight: Parent Volumes not in AM | INVALID | _______ | _______ | _______ |
| (new) Class-4 invariants | per Phase 5 | _______ | _______ | _______ |

### Exit criteria (when you can move to Phase 7)

- ✅ Every existing fixture has been run through SIMPLNX (`ctest -R "ComputeGroupingDensityFilter"`)
- ✅ All tests pass on `develop`
- ✅ Any new Class-4 invariant fixtures designed in Phase 5 are added to the test suite and pass
- ✅ Any regressions surfaced have been fixed and the failing-test → fix → green-test cycle is complete

---

## Phase 7 — Algorithm Review

**Goal**: a quality pass on already-correct code (comments, naming, memory, progress messaging, cancel checks). Correctness was already established in Phase 6.

**Required reading**:
- The `bluequartz-skills:review-algorithm` skill description and any algorithm-review guidance it surfaces

### Tasks

- [ ] **Invoke the `bluequartz-skills:review-algorithm` skill** with the filter name `ComputeGroupingDensityFilter`.
- [ ] **Bring the following pre-observed items to the review** as starting context (none are pre-flagged as bugs, just observations from audit):
  - [ ] **Single-threaded design**: the algorithm uses a flat parent-loop, no `ParallelDataAlgorithm`. No thread-safety concerns. Verify this is intentional (parents are independent — could parallelize) or a deliberate simplicity choice.
  - [ ] **Cancel check inside parent loop**: present and correct. Confirm placement and frequency.
  - [ ] **`ThrottledMessenger` for progress reporting**: present and correct. Confirm message content is informative for users on large datasets.
  - [ ] **Sentinel value `-1.0f`**: confirmed for empty parents. Verify no float-equality comparisons against sentinel (use `<` or NaN-safe comparison if relevant).
  - [ ] **Templated worker `FindDensityGrouping<UseNonContiguousNeighbors, FindCheckedFeatures>`**: 4 specializations. Verify code generation produces all 4 with no dead code.
- [ ] **Address every Critical and Warning finding** from the review skill (fix, OR explicitly defer with documented rationale).

### Exit criteria (when you can move to Phase 8)

- ✅ `review-algorithm` skill run to completion
- ✅ Every Critical and Warning finding has either been fixed or explicitly deferred with a documented rationale
- ✅ Pre-observed items above all confirmed (or any concerns documented)

---

## Phase 8 — Unit Test Review & Implementation

**Goal**: encode the Phase-5 oracle into the test suite (already done for this filter), fix the test-tag bug, and add Class-4 invariant assertions.

**Required reading**:
- Policy lines 206–219 — Unit Test Review section
- `bluequartz-skills:plan-filter-tests` and `bluequartz-skills:implement-filter-tests` skill descriptions
- `bluequartz-skills:dual-build-protocol` skill description

### Tasks

- [ ] **Fix the test-tag bug**: change `[SimplnxReview]` to `[SimplnxCore]` on the first 8 TEST_CASEs:
  ```bash
  sed -i.bak 's|"\[SimplnxReview\]\[ComputeGroupingDensityFilter\]"|"[SimplnxCore][ComputeGroupingDensityFilter]"|g' \
    src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp
  rm src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp.bak
  ```
  Verify with `grep TEST_CASE src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp`.
- [ ] **Add Class-4 invariant assertions** to existing tests (or as a new TEST_CASE). Specifically:
  ```cpp
  // Class 4 invariants:
  REQUIRE(density == Catch::Approx(-1.0f) || density >= 0.0f);
  REQUIRE(groupingDensities.getNumberOfTuples() == numParents);
  if (findCheckedFeatures) {
    REQUIRE(checkedFeatures.getNumberOfTuples() == numFeatures);
    for (auto cf : checkedFeatures) {
      REQUIRE(cf >= 0);
      REQUIRE(cf < static_cast<int32>(numParents));
    }
  }
  ```
- [ ] **Wrap every `getDataRefAs<T>()` call in `REQUIRE_NOTHROW()`** per `.claude/CLAUDE.md`. Audit the existing test file for any unwrapped calls.
- [ ] **Add hand-derivation comments** to the test source for each Class-1 fixture if not already present. The existing tests have some; ensure all 4 algorithmic specializations are documented inline.
- [ ] **Expand Test 1 to cover sibling exemplars** if Phase 2 confirmed they exist:
  ```cpp
  // For each of (false,false), (true,false), (false,true), (true,true):
  //   - load the corresponding GroupingDensities ({A},{B}) array from the exemplar
  //   - run filter with matching parameters
  //   - REQUIRE arrays match
  ```
- [ ] **Invoke `bluequartz-skills:plan-filter-tests`** if there are remaining gaps.
- [ ] **Invoke `bluequartz-skills:implement-filter-tests`** to fill the gaps the plan identifies.
- [ ] **Run the dual-build protocol** per `bluequartz-skills:dual-build-protocol`:
  - [ ] In-core build passes all `ComputeGroupingDensity*` tests
  - [ ] Out-of-core build passes all `ComputeGroupingDensity*` tests

### Exit criteria (when you can move to Phase 9)

- ✅ Test-tag bug fixed (`[SimplnxReview]` → `[SimplnxCore]` on all 8 affected TEST_CASEs)
- ✅ Class-4 invariant assertions added
- ✅ Every `getDataRefAs<T>()` is wrapped in `REQUIRE_NOTHROW()`
- ✅ Hand-derivation comments are present on every Class-1 fixture
- ✅ Sibling exemplar coverage added (if applicable per Phase 2)
- ✅ Dual-build protocol passes (in-core + OOC)

---

## Phase 9 — Legacy DREAM3D Comparison (diff explanation)

**Goal**: diff SIMPLNX against DREAM3D 6.5.172 on the same toy data and write the result up as user-facing **Deviation entries**. This is **not a correctness check** (correctness was Phase 6's job) — it is a **diff-explanation** exercise.

**Required reading**:
- Policy lines 84–162 — full text on Legacy Comparison section, including the Deviation Template format and worked examples
- `bluequartz-skills:compare-legacy-dream3d` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:compare-legacy-dream3d` skill** with the filter name. Use the Phase-5 fixtures as comparison inputs (especially the 4 specialization tests B, C, D, E).
- [ ] **Open the comparison report** with the Algorithm Relationship line you wrote in Phase 3.
- [ ] **Confirm or address the no-6.4-fixture question**: PR #1588 added a 6.5 fixture but no 6.4 fixture. Verify whether legacy `FindGroupingDensity` was reachable via the 6.4 `Filter_Name`-fallback path. If yes, add a 6.4 fixture; if no, document why 6.4 is intentionally skipped.
- [ ] **For each output difference between SIMPLNX and DREAM3D 6.5.172**, write a structured Deviation entry per policy lines 117–131:

```
**Deviation ID:** ComputeGroupingDensity-D<N>
**Filter UUID:** ff46afcf-de32-4f37-98bc-8f0fd4b3c122
**Symptom:** <one-sentence user-visible symptom>
**Root cause:** <bug | precision | order of operations | library | algorithmic choice>
**Affected users:** <who actually sees this>
**Recommendation:** <trust SIMPLNX | trust 6.5.172 | either acceptable | see quick-patch>
```

### Pre-flagged candidate deviations (proposed, pending comparison)

The audit found no PR-driven bug history for this filter. The candidates below are speculative — to be populated only if `compare-legacy-dream3d` finds discrepancies.

> **Deviation ID:** `ComputeGroupingDensity-D1` *(placeholder — populate only if found)*
> **Filter UUID:** `ff46afcf-de32-4f37-98bc-8f0fd4b3c122`
> **Symptom:** *(if observed)* `CheckedFeatures` array values differ between SIMPLNX and 6.5.172 when two parents have equal volume
> **Root cause:** Algorithmic choice or Bug. The "largest-parent-volume wins" tie-breaking rule may differ between SIMPLNX and legacy when volumes tie exactly. SIMPLNX uses first-encountered-with-max-volume; legacy may use last-encountered or first-by-parent-id.
> **Affected users:** Anyone whose dataset has parents with equal volumes
> **Recommendation:** *(TBD pending comparison)*

> **Deviation ID:** `ComputeGroupingDensity-D2` *(placeholder)*
> **Filter UUID:** `ff46afcf-de32-4f37-98bc-8f0fd4b3c122`
> **Symptom:** *(if observed)* sentinel value for empty parent differs (`-1.0f` in SIMPLNX vs. some other value in legacy)
> **Root cause:** TBD by comparison
> **Affected users:** Anyone with parents that have no children matching their `ParentId`

> **Deviation ID:** `ComputeGroupingDensity-D3` *(placeholder)*
> **Filter UUID:** `ff46afcf-de32-4f37-98bc-8f0fd4b3c122`
> **Symptom:** *(if observed)* density values differ at low-significance bits due to float32 vs float64 internal accumulation order
> **Root cause:** Precision or order of operations
> **Affected users:** Anyone comparing density values at < 1e-6 precision

### Exit criteria (when you can move to Phase 10)

- ✅ Comparison run on each Phase-5 fixture (especially B, C, D, E)
- ✅ Each pre-flagged candidate deviation has been either confirmed (becomes a real Deviation entry with all fields filled in), OR explicitly retracted with a "no observed difference" note
- ✅ Comparison report opens with Phase-3 Algorithm Relationship line
- ✅ 6.4 fixture decision recorded (added if needed, OR explicitly skipped with reason)
- ✅ Every Deviation entry has all required fields filled in (ID, UUID, Symptom, Root cause, Affected users, Recommendation)

---

## Phase 10 — Exemplar Validation & Publishing

**Goal**: validate the exemplar dataset and ensure the archive ReadMe captures the Oracle Provenance block per policy.

**Required reading**:
- Policy lines 65–82 — Oracle Provenance record requirements per class
- `bluequartz-skills:validate-and-publish-exemplars` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:validate-and-publish-exemplars` skill** with the filter name.
- [ ] **Decide on the exemplar archive** (depends on Phase 2 provenance investigation):
  - [ ] **Keep `compute_grouping_densities.tar.gz` as-is** with provenance ReadMe naming the SIMPLNX engineer + date generated (likely PR #1548 author)
  - [ ] **Regenerate** if Phase 2 found the existing exemplar is mis-versioned or incomplete
  - [ ] **Expand** to include sibling variants `(true, false)`, `(false, true)`, `(true, true)` if Phase 2 confirmed they exist or should be added
- [ ] **Add the Oracle Provenance block to the archive ReadMe** per the chosen oracle class:
  - [ ] If Class 1 only: no provenance block required (oracle lives in test code)
  - [ ] If Class 2 (exemplar-as-oracle for any variant): SIMPLNX/DREAM3D version + script/pipeline filename used to generate + seed if any
  - [ ] If Class 3 (paper-based, if literature search found one): DOI + edition + equation # + page #. Embed paper PDF.
- [ ] **Re-run dual-build tests** with the final exemplar to confirm pass.

### Exit criteria (when you can move to Phase 11)

- ✅ Exemplar archive decision made (keep / regenerate / expand) with rationale
- ✅ Oracle Provenance block added to archive ReadMe per chosen oracle class
- ✅ Paper PDF embedded (if Phase 2 found a citable reference)
- ✅ Dual-build tests pass with the final exemplar

---

## Phase 11 — Documentation Review

**Goal**: bring the user-facing documentation up to date — accuracy against current implementation, paper references (if any), warnings about parameter constraints, and links to public Deviation entries.

**Required reading**:
- `bluequartz-skills:review-filter-docs` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:review-filter-docs` skill** with the filter name.
- [ ] **Confirm doc accuracy against current implementation**. The audit rated the doc "Excellent" — worked example with explicit numbers, three diagrams (`ComputeGroupingDensity_FeatureIds.png`, `ComputeGroupingDensity_ParentIds.png`, `ComputeGroupingDensity_Algorithm.png`), parameter descriptions, interpretation table.
- [ ] **Address each pre-observed gap** below:
  - [ ] Empty "References" section — populate if Phase 2 literature search found a citation; otherwise note "No external paper reference; see worked example above and code documentation"
  - [ ] Add a "Known differences from DREAM3D 6.5" section linking the public Deviation entries from Phase 9 (only if Phase 9 produced any)
  - [ ] Confirm worked example numbers (`45/70`, `55/70`, etc.) match the test source — they should, but verify
- [ ] **Cross-reference the doc and test source**: the same hand-derived constants should appear in both, with consistent comments. Update either if they drift.

### Exit criteria (when you can move to Phase 12)

- ✅ `review-filter-docs` skill run to completion
- ✅ References section populated (with paper citation OR explicit "no external reference")
- ✅ Public Deviation entries from Phase 9 linked from user docs (if any exist)
- ✅ Worked-example numbers match test source

---

## Phase 12 — Archive

**Goal**: assemble the complete OneDrive archive with all data, scripts, papers, and a ReadMe capturing everything a future engineer needs to reproduce this V&V.

**Required reading**:
- Policy lines 230–234 — Archiving Everything When Finished
- `bluequartz-skills:archive-filter-verification` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:archive-filter-verification` skill** with the filter name.
- [ ] **Add the V&V-policy-required fields** to the archive ReadMe:
  - [ ] **Algorithm Relationship** line from Phase 3 (e.g., "Port — direct translation of SIMPL `FindGroupingDensity`...")
  - [ ] **Oracle class** from Phase 4 (Class 1 + Class 4 companion, with potential Class 3 upgrade if literature found)
  - [ ] **Oracle Provenance block** from Phase 10 (if Class 2/3/5 used)
  - [ ] **Second-engineer review** from Phases 4–5: named reviewer + date, OR documented skip reason
  - [ ] **Promoted artifacts** from Phase 2: list what existed before vs. what was created new (this filter has a LOT of pre-existing — list it all)
  - [ ] **Reproduction instructions**: how to regenerate the toy data, oracle output, and comparison from scratch (Phase 5 hand-derivations, the `compute_grouping_densities.dream3d` exemplar source pipeline, the legacy comparison pipeline)
- [ ] **Confirm everything is in the archive folder**:
  - [ ] All Phase-5 fixtures (input data + expected output)
  - [ ] Working artifacts from `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter_artifacts/`
  - [ ] DREAM3D-NX pipeline files used in Phase 6 reconciliation and Phase 9 comparison
  - [ ] DREAM3D 6.5.172 pipeline files used in Phase 9 comparison
  - [ ] Comparison report from Phase 9 (with all Deviation entries OR retraction notes)
  - [ ] Embedded paper PDF (if Phase 2 found one)
  - [ ] The complete ReadMe with all required fields
- [ ] **Upload to OneDrive** in a folder named `ComputeGroupingDensity_VandV/`.

### Exit criteria (when you can move to Phase 13)

- ✅ Archive folder assembled with every required artifact
- ✅ ReadMe contains all V&V-policy-required fields
- ✅ Folder uploaded to OneDrive
- ✅ Upload location recorded in Phase 13

---

## Phase 13 — Update tracking artifacts

**Goal**: close the loop on the V&V by updating shared tracking documents.

### Tasks

- [ ] **Update this document's Status line** at the very top: change `*Status:* **DRAFT — ...**` to `*Status:* **COMPLETE — V&V finished YYYY-MM-DD.**`. The file STAYS at `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter.md`.
- [ ] **Record the OneDrive archive path** here:
  > *OneDrive path:*
- [ ] **Update the retroactive V&V report** at `docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md`:
  - [ ] Status: DRAFT → confirmed
  - [ ] Confirmed Algorithm Relationship (Phase 3)
  - [ ] Confirmed Oracle class (Phase 4)
  - [ ] Resolved V&V status table (replace "Unknown" / "No" entries with confirmed status)
  - [ ] Final Deviation entries (replace "proposed, pending" with shipped IDs from Phase 9, OR mark all as retracted with "no observed difference")
- [ ] **Update `docs/vv_retroactive_reports/INDEX.md`**:
  - [ ] Move filter from DRAFT row to confirmed row
  - [ ] No bug flag to clear (this filter had none)
  - [ ] Update overall counts in the at-a-glance metrics table
- [ ] **Note in the team's V&V deliverable tracker** that this filter is complete for SBIR purposes.
- [ ] **Update memory** to note that the workflow has been validated end-to-end on the first Tier-1 retroactive promotion pilot:
  - [ ] Note in `mtr_vv_policy.md` (auto-memory) that ComputeGroupingDensity is the first filter to complete the full vv-filter workflow

### Exit criteria (V&V is COMPLETE)

- ✅ Status line at top updated to "COMPLETE"
- ✅ OneDrive archive path recorded
- ✅ Retroactive report updated DRAFT → confirmed
- ✅ INDEX.md updated
- ✅ Team tracker updated
- ✅ Final Phase Summary block (below) filled in

---

## Phase Summary *(fill in at end of V&V pass)*

```markdown
# V&V Complete: ComputeGroupingDensityFilter

## Algorithm Relationship
<Port|Minor|Rewrite|New filter> — <one-line evidence>

## Oracle
Class: <e.g., 1 (analytical) + 4 (invariant)>
Justification: <one-line>
Second-engineer review: <named, date> | <skipped: reason>

## Phase Results
| Phase | Status | Notes |
|---|---|---|
| 1 Discovery | Complete | 9 source files; retroactive promotion mode |
| 2 Promote existing | Complete | <count> artifacts promoted; provenance investigation result for compute_grouping_densities.tar.gz |
| 3 Algorithm Relationship | Confirmed | Port (with rename) |
| 4 Oracle classification | Confirmed | Class 1 + 4 (or Class 3 if literature found) |
| 5 Toy data + expected | Complete | 8 existing fixtures + Class-4 invariant additions |
| 6 SIMPLNX vs oracle | Reconciled | <count> bugs found and fixed during reconciliation (expected: 0) |
| 7 Algorithm Review | Complete | <findings count>, <fixed | deferred> |
| 8 Unit Tests | Complete | <count> test cases, dual-build pass; test-tag bug fixed; sibling exemplars added (if applicable) |
| 9 Legacy Comparison | Complete | <count> Deviation entries published OR all retracted |
| 10 Exemplar Publishing | Complete | compute_grouping_densities.tar.gz <kept|regenerated|expanded>, Oracle Provenance recorded |
| 11 Documentation | Complete | References section populated, Phase-9 Deviations linked |
| 12 Archive | Complete | OneDrive at <path> |
| 13 Tracking | Complete | INDEX + retroactive report updated; this doc Status → COMPLETE |

## Outstanding
<Any deferred issues, follow-up work, or known limitations>
```
