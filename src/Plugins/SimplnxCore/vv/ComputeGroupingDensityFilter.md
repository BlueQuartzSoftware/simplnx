# V&V Working Document: ComputeGroupingDensityFilter

*Status:* **DRAFT — Phase 1 (Discovery) complete; Phases 2–13 are work orders for the engineer to execute.**

This document is the engineer's working V&V doc for `ComputeGroupingDensityFilter`. It follows the workflow in `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`. **Retroactive promotion pass** — a tentative retroactive V&V report exists at [`docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md`](../../../../docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md) and most discovery work is pre-populated below.

**How to use this document:**
- Each phase has a **Goal**, **Tasks** (checkboxes), and **Exit criteria**. Phase 1 is the canonical source for discovery context; later phases reference Phase 1 rather than restate.
- Where a "starting point" is provided, you can confirm it or replace it.
- Update this document as you work. It IS the deliverable. Phase 13 updates the Status line at the top to "COMPLETE"; the file stays at this location.

**Source-of-truth references** (paths relative to this doc at `src/Plugins/SimplnxCore/vv/`):
- Policy: [`docs/vv_templates/mtr_filter_verification_validation.md`](../../../../docs/vv_templates/mtr_filter_verification_validation.md)
- Oracle quick-reference: [`docs/vv_templates/oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md)
- Audit cross-cutting findings: [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md) "Cross-cutting findings"
- Skill: `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`

---

## Phase 1 — Discovery *(complete; this is the canonical source for all later phases)*

**Goal**: locate every file related to this filter and identify the legacy DREAM3D equivalent.

### Tasks (verify findings)

- [ ] Confirm metadata, source files, exemplar archive, test inventory, PR list, and algorithm summary below are accurate
- [ ] Re-run `git log --since=2025-10-01 --oneline -- <Phase-1 files>` to check no new PRs landed since this doc was generated

### Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `ff46afcf-de32-4f37-98bc-8f0fd4b3c122` |
| SIMPLNX ClassName | `ComputeGroupingDensityFilter` |
| Human Name | Compute Grouping Densities |
| SIMPL UUID | `708be082-8b08-4db2-94be-52781ed4d53d` |
| SIMPL ClassName | `FindGroupingDensity` |
| Plugin | SimplnxCore |
| Tier | Tier-1 (MTR SBIR list) |
| Mode | Retroactive promotion (retroactive report exists) |

### Source files

| Path | Notes |
|---|---|
| `Filters/ComputeGroupingDensityFilter.{hpp,cpp}` | Filter |
| `Filters/Algorithms/ComputeGroupingDensity.{hpp,cpp}` | Algorithm (~140 lines, single-threaded) |
| `test/ComputeGroupingDensityTest.cpp` | 9 TEST_CASEs (see inventory) |
| `test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json` | 6.5 conversion fixture |
| `test/simpl_conversion/6_4/...` | ⚠️ Missing — confirm intentional in Phase 9 |
| `docs/ComputeGroupingDensityFilter.md` | User doc — has worked example with explicit numbers, 3 diagrams (`Images/ComputeGroupingDensity_{Algorithm,FeatureIds,ParentIds}.png`), parameter table. Empty References section. |
| `SimplnxCoreLegacyUUIDMapping.hpp` | Legacy UUID map entry |
| `test/CMakeLists.txt` | Archive download line ~288 |

Paths are relative to `src/Plugins/SimplnxCore/`. No Python binding. All file references in later phases use just `<Filename>` and resolve here.

### Test exemplar archive

- **Archive**: `compute_grouping_densities.tar.gz`
- **SHA512**: `96066196d6aa5f87cc7b717f959848c2f3025b7129589abe1eded2a8d725c539a89b0a6290a388a56b5a401e0bd3041698fbd8e8cf37a1f18fdd937debd21531`
- **Test data dir**: `compute_grouping_densities/`
- **Confirmed contents**: `compute_grouping_densities.dream3d` with exemplar array `GroupingDensities (false, false)` at `ImageGeom/ParentFeatureData/`
- **Suspected contents**: sibling exemplars `(true, false)`, `(false, true)`, `(true, true)` may also be in the archive — variant suffix in the existing array name suggests it. Phase 2 investigates.
- **Provenance**: not yet documented. Phase 2 investigates.

### Material PRs since 2025-10-01

Filter did not exist before 2026-02-25. Only **2 PRs** ever touched these files (no broad-refactor PRs reached this filter — they merged before #1548):

| PR | Date | Title | Effect on this filter |
|---|---|---|---|
| #1548 | 2026-02-25 | FILT: Compute Grouping Density filter added. | Initial introduction (+1203 lines, –0): filter, algorithm, doc with 3 diagrams + worked example, 8 tests with hand-derived expected values, legacy UUID map entry. |
| #1588 | 2026-04-22 | ENH: SIMPL Backwards Compatibility Test Redesign | Added 6.5 conversion test + 6.5 fixture JSON. No 6.4 fixture. |

### TEST_CASE inventory

`ComputeGroupingDensityTest.cpp` contains 9 `TEST_CASE`s. Tests 1–8 use tag `[SimplnxReview]` ⚠️ (should be `[SimplnxCore]`); Test 9 uses correct `[SimplnxCore]` tag.

| # | Name | Coverage | Encodes oracle? |
|---|---|---|---|
| 1 | `Basic Density (contiguous, no checked features)` | Exemplar comparison + hand-derived `45/70`, `55/70` | Class 1 + 2 |
| 2 | `Contiguous Only, No Checked Features` | `(useNonContiguous=false, findCheckedFeatures=false)` synthetic | Class 1 |
| 3 | `With Non-Contiguous Neighbors` | `(true, false)` — hand-derived `45/100`, `55/100` | Class 1 |
| 4 | `With Checked Features` | `(false, true)` — verifies "largest-parent-volume wins" tie-break, expected `[0,1,1,2,2,2]` | Class 1 + 4 |
| 5 | `Both Options Enabled` | `(true, true)` combined | Class 1 + 4 |
| 6 | `Preflight Error - Feature tuple count mismatch` | Negative-path | Class 4 |
| 7 | `Preflight Error - Volumes not in AttributeMatrix` | Negative-path | Class 4 |
| 8 | `Preflight Error - Parent Volumes not in AttributeMatrix` | Negative-path | Class 4 |
| 9 | `SIMPL Backwards Compatibility` ✓ | SIMPL 6.5 conversion | Conversion-only |

⚠️ **Tag bug**: Tests 1–8 use `[SimplnxReview]` instead of `[SimplnxCore]`. Filter was likely prototyped in `SimplnxReview` plugin before being moved. Phase 8 fixes.

### Algorithm summary

`density(p) = ParentVolume(p) / Σ FeatureVolume(f)` where `f` ranges over (children of `p`) ∪ (contiguous neighbors of children) ∪ (optionally non-contiguous neighbors of children), each `f` counted once via a `totalFeatureCheckList` set.

- **Parameters**: `UseNonContiguousNeighbors` (bool), `FindCheckedFeatures` (bool) → 4 template specializations of `FindDensityGrouping<...>`. Tests 2–5 cover the 2×2 cross-product.
- **`FindCheckedFeatures` writes a per-feature `CheckedFeatures` array**: which parent claimed each feature; tie-break = largest-parent-volume wins.
- **Sentinel**: `density = -1.0f` for parents whose `ParentId` matches no children.
- **Threading**: single-threaded; cancel check inside parent loop; `ThrottledMessenger` for progress.
- **Hand-derived constants in test source**: `45/70`, `55/70`, `45/100`, `55/100` with derivation comments. These ARE the de-facto Class-1 oracle today.
- **Doc has worked example** matching the test constants and three diagrams illustrating algorithm flow.

### Exit criteria

- ✅ Verification checkboxes above checked
- ✅ Any new findings or corrections noted in this section
- ✅ `git log` confirms no new PRs landed since this doc was generated

---

## Phase 2 — Promote existing work product

**Goal**: investigate provenance of pre-existing artifacts and decide what to promote, augment, or replace.

### Tasks

- [ ] **Investigate provenance of `compute_grouping_densities.tar.gz`** (Phase 1 §"Test exemplar archive"):
  - [ ] When and by whom: `git log --all --follow -- src/Plugins/SimplnxCore/test/CMakeLists.txt | grep -B2 compute_grouping_densities`
  - [ ] Download the archive locally; inspect for inner `ReadMe.md`, source pipeline `.d3dpipeline`, input `.dream3d`
  - [ ] **Confirm or refute the suspected sibling variants** `(true,false)`, `(false,true)`, `(true,true)` (Phase 1). If present, Phase 8 expands Test 1 to consume all four; Phase 10 documents them.
- [ ] **Search source for paper references**: `grep -E "@(reference|cite)|10\.\d{4}/" src/Plugins/SimplnxCore/src/SimplnxCore/Filters/{Algorithms/,}ComputeGroupingDensity*`. Audit found none; confirm.
  > *Found:*
- [ ] **Literature search for MTR-density definition**: check Pilchak/Williams or Bridier (α/β titanium dwell-fatigue / MTR papers) for a "grouping density" metric. If a citation exists, oracle Class promotes 1 → 3 in Phase 4.
  > *Result:*
- [ ] **Check OneDrive/Slack/email** for prior `ComputeGroupingDensity*` or `FindGroupingDensity*` notes.
  > *Found:*
- [ ] **Decide promote/augment/replace** for each pre-existing artifact:

| Artifact (already in repo) | Candidate role | Decision |
|---|---|---|
| Hand-derived constants in tests (Phase 1 §"Algorithm summary") | Class 1 oracle of record | _______________ |
| Exemplar `compute_grouping_densities.tar.gz` (Phase 1 §"Test exemplar archive") | Class 2 (script-derived) OR Class 5 (legacy expert) — depends on provenance | _______________ |
| User doc with worked example + 3 diagrams (Phase 1 §"Source files") | Public Class-1 derivation | _______________ |
| `[SimplnxReview]` tag on Tests 1–8 (Phase 1 §"TEST_CASE inventory") | N/A — bug | **Replace in Phase 8** |

### Exit criteria

- ✅ Archive provenance determined
- ✅ Sibling-variant question answered (present or not)
- ✅ Literature search recorded (with result)
- ✅ Promotion table filled in

---

## Phase 3 — Algorithm Relationship classification

**Goal**: classify how SIMPLNX relates to legacy. Opens the Phase 9 report.

**Required reading**: Policy lines 98–113 (the four classifications).

### Tasks

- [ ] **Read SIMPL `FindGroupingDensity` source** in DREAM3D 6.5.x. Compare to SIMPLNX algorithm (Phase 1 §"Algorithm summary").
- [ ] **Verify `FromSIMPLJson()` parameter mapping** in `ComputeGroupingDensityFilter.cpp` against `test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json`. Audit confirmed 9 SIMPL keys → 9 SIMPLNX keys via `SIMPLConversion::ConvertParameter`.
- [ ] **Cross-check the "largest-parent-volume wins" tie-break** in `FindCheckedFeatures` against legacy. Likely Deviation candidate if the rules differ.
- [ ] **Pick classification**: ☐ Port  ☐ Minor changes  ☐ Rewrite  ☐ New filter
- [ ] **Write the Phase-9 opening line**:
  > *Suggested* — *Algorithm Relationship: **Port** — direct translation of SIMPL `FindGroupingDensity` (UUID `708be082-…`); renamed per SIMPLNX `Find* → Compute*` convention. Output and parameter semantics intended to match legacy. No bug fixes since creation.*
  >
  > *Final:*

### Recommended starting point

**Tentative: Port (with rename).** Evidence: legacy UUID map entry, 9-for-9 parameter mapping in `FromSIMPLJson()`, algorithm structure (parent-loop → feature-loop → contiguous-neighbor-loop → optional non-contiguous-neighbor-loop → density ratio) consistent with legacy design.

### Exit criteria

- ✅ Legacy source compared; tie-break rule cross-checked
- ✅ Classification chosen; opening line written

---

## Phase 4 — Oracle classification

**Goal**: pick oracle class(es) defining "correct" independently of legacy DREAM3D.

**Required reading**: [`oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md); policy lines 25–82.

### Tasks

- [ ] **Confirm or replace the recommended stack** below.
- [ ] **Identify second-engineer reviewer** (policy line 39 — author is least likely to notice a wrong oracle):
  - [ ] Reviewer + date: ___________
  - [ ] OR skip reason (recorded in Phase 12 archive ReadMe):
    > *Skip reason:*

### Recommended starting point

**Class 1 (Analytical) primary** — `density = ParentVolume / Σ FeatureVolume` is hand-derivable. Tests 2–5 already encode this oracle (Phase 1 §"TEST_CASE inventory"). Work for this filter is **formalize, not design**.

**Class 4 (Invariant) companion** — natural assertions:
- `density >= 0.0f` OR `density == -1.0f` (sentinel)
- `len(GroupingDensities) == numParents`
- `CheckedFeatures[f] ∈ [0, numParents-1]` when `FindCheckedFeatures = true`
- `density == -1.0f` IFF parent has no matching children

**Class 3 (Paper-based) potential upgrade** — only if Phase 2 literature search finds a citable MTR-density reference (Pilchak/Williams/Bridier). Embed paper PDF in archive if so.

### Exit criteria

- ✅ Primary + companion class(es) recorded
- ✅ Class 5 justification recorded if applicable (not expected here)
- ✅ Second-engineer reviewer or skip reason recorded

---

## Phase 5 — Toy data design + independent expected output

**Goal**: ensure every code path has a fixture with an expected output derived from the oracle (not from a DREAM3D run). **Existing tests already do most of this**; your job is mostly to formalize and add Class-4 invariants.

**Required reading**: Policy lines 41–64 (Step 0 a–c).

**Critical**: don't run SIMPLNX or DREAM3D in this phase. Expected output comes from the oracle alone.

### Tasks

- [ ] **For each existing fixture (Phase 1 §"TEST_CASE inventory")**, confirm the hand-derivation comment is present and correct in the test source. Add or elaborate where missing.
- [ ] **Design Class-4 invariant fixtures** (recommended below) for any not already present.
- [ ] **Save oracle artifacts** to `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter_artifacts/`:
  - [ ] Hand-derivation notes (markdown — promote test-source comments)
  - [ ] If literature search (Phase 2) found a paper: embed PDF + cite equation #
- [ ] **If sibling exemplars confirmed in Phase 2**: list them in the fixture table as additional Class-2 fixtures.

### Recommended Class-4 invariant fixtures to add

- [ ] **Sentinel**: parent with no matching children → `density == -1.0f`
- [ ] **Cardinality**: `len(GroupingDensities) == numParents`
- [ ] **CheckedFeatures range**: every value in `[0, numParents-1]`

### Exit criteria

- ✅ Hand-derivations confirmed/elaborated for Tests 2–5
- ✅ Class-4 invariant fixtures designed
- ✅ Working artifact folder created
- ✅ Sibling-variant fixtures documented (if Phase 2 confirmed they exist)
- ✅ Second-engineer review of oracle artifacts (if scheduled in Phase 4)

---

## Phase 6 — SIMPLNX vs oracle reconciliation

**Goal**: confirm SIMPLNX matches the Phase-5 oracle. Existing tests do this; run them as the formal Phase-6 gate.

**Required reading**: Policy lines 46–47 (Step 0d).

### Tasks

- [ ] **Run the test suite scoped to this filter**:
  ```bash
  cd /Users/mjackson/Workspace2/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel
  ctest -R "ComputeGroupingDensityFilter" --verbose
  ```
- [ ] **All existing tests must pass on `develop`**. Any failure = regression to investigate and fix in this V&V pass.
- [ ] **Run any new Class-4 invariant fixtures** added in Phase 5 (after Phase 8 implements them).
- [ ] **No bugs are pre-flagged** for this filter (algorithm is short, single-threaded, no obvious bug surface). If any surface, follow failing-test → fix → green-test cycle.

### Exit criteria

- ✅ All existing tests pass on `develop`
- ✅ New Class-4 invariant fixtures pass after Phase 8 lands them
- ✅ Any regressions resolved

---

## Phase 7 — Algorithm Review

**Goal**: code quality pass on already-correct code. Correctness is Phase 6's job.

### Tasks

- [ ] **Invoke `bluequartz-skills:review-algorithm`** with `ComputeGroupingDensityFilter`.
- [ ] **Pre-observed quality items to confirm** (none flagged as bugs by audit):
  - [ ] Single-threaded by choice — verify intent (parents are independent, could parallelize)
  - [ ] Cancel check placement and frequency in parent loop
  - [ ] `ThrottledMessenger` message content informativeness for large datasets
  - [ ] No float-equality comparisons against the `-1.0f` sentinel
  - [ ] All 4 template specializations of `FindDensityGrouping` compile without dead code
- [ ] **Address all Critical/Warning findings** from the review (fix or document deferral).

### Exit criteria

- ✅ Review skill complete
- ✅ Critical/Warning findings fixed or deferred with rationale

---

## Phase 8 — Unit Test Review & Implementation

**Goal**: encode the oracle into the test suite formally, fix the test-tag bug, add Class-4 invariants.

### Tasks

- [ ] **Fix the `[SimplnxReview]` → `[SimplnxCore]` tag bug** (flagged in Phase 1 §"TEST_CASE inventory"):
  ```bash
  sed -i.bak 's|"\[SimplnxReview\]\[ComputeGroupingDensityFilter\]"|"[SimplnxCore][ComputeGroupingDensityFilter]"|g' \
    src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp
  rm src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp.bak
  ```
  Verify with `grep TEST_CASE src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp`.
- [ ] **Implement Class-4 invariant assertions** designed in Phase 5. Sample form:
  ```cpp
  REQUIRE(density == Catch::Approx(-1.0f) || density >= 0.0f);
  REQUIRE(groupingDensities.getNumberOfTuples() == numParents);
  if (findCheckedFeatures) {
    REQUIRE(checkedFeatures.getNumberOfTuples() == numFeatures);
    for (auto cf : checkedFeatures) { REQUIRE(cf >= 0); REQUIRE(cf < static_cast<int32>(numParents)); }
  }
  ```
- [ ] **Wrap every `getDataRefAs<T>()` call in `REQUIRE_NOTHROW()`** per `.claude/CLAUDE.md`. Audit existing tests.
- [ ] **Expand Test 1 to cover sibling variants** if Phase 2 confirmed they exist. Loop over the 4 `(useNonContiguous, findCheckedFeatures)` combinations and compare each against its `GroupingDensities ({A},{B})` sibling exemplar.
- [ ] **Plan/implement gap coverage** via `bluequartz-skills:plan-filter-tests` and `bluequartz-skills:implement-filter-tests` if Phase 1 inventory plus the additions above leave any code path uncovered.
- [ ] **Dual-build**: per `bluequartz-skills:dual-build-protocol`, in-core and OOC must both pass.

### Exit criteria

- ✅ Tag bug fixed
- ✅ Class-4 invariants in tests
- ✅ All `getDataRefAs<T>()` calls wrapped
- ✅ Sibling-variant coverage added (if applicable)
- ✅ Dual-build pass

---

## Phase 9 — Legacy DREAM3D Comparison (diff explanation)

**Goal**: diff against DREAM3D 6.5.172 and write up differences as user-facing Deviation entries. **Not a correctness check** — that was Phase 6.

**Required reading**: Policy lines 84–162 (Legacy Comparison + Deviation Template).

### Tasks

- [ ] **Invoke `bluequartz-skills:compare-legacy-dream3d`**. Use Tests 2–5 fixtures as comparison inputs.
- [ ] **Open the report with the Phase-3 Algorithm Relationship line.**
- [ ] **Decide on the missing 6.4 fixture** (Phase 1 §"Source files"): was legacy `FindGroupingDensity` reachable via 6.4 `Filter_Name`-fallback? Add fixture if yes; document why skipped if no.
- [ ] **For each output difference**, write a Deviation entry per policy lines 117–131 using the candidates below as starting points (each entry will be either populated or retracted with a "no observed difference" note).

### Pre-flagged candidate deviations (audit found no PR-driven bug history; these are speculative)

> **Deviation ID:** `ComputeGroupingDensity-D1` *(placeholder)*
> **Filter UUID:** `ff46afcf-de32-4f37-98bc-8f0fd4b3c122`
> **Symptom:** *(if observed)* `CheckedFeatures` differs when two parents have equal volume — tie-break may select a different parent
> **Root cause:** Algorithmic choice or Bug
> **Affected users:** datasets with parents at equal volume
> **Recommendation:** *(TBD)*

> **Deviation ID:** `ComputeGroupingDensity-D2` *(placeholder)*
> **Symptom:** *(if observed)* sentinel for empty-parent differs (`-1.0f` SIMPLNX vs. legacy)
> **Root cause:** TBD

> **Deviation ID:** `ComputeGroupingDensity-D3` *(placeholder)*
> **Symptom:** *(if observed)* density differs at low-significance bits (precision / accumulation order)
> **Root cause:** Precision or order of operations

### Exit criteria

- ✅ Comparison run on Tests 2–5 fixtures
- ✅ Each placeholder deviation either populated or retracted
- ✅ 6.4 fixture decision recorded
- ✅ Report opens with Phase-3 line

---

## Phase 10 — Exemplar Validation & Publishing

**Goal**: validate exemplars and add Oracle Provenance to the archive ReadMe.

**Required reading**: Policy lines 65–82.

### Tasks

- [ ] **Invoke `bluequartz-skills:validate-and-publish-exemplars`**.
- [ ] **Decide archive disposition** (depends on Phase 2 provenance):
  - [ ] Keep `compute_grouping_densities.tar.gz` as-is with provenance ReadMe documenting SIMPLNX engineer + date generated
  - [ ] Regenerate if Phase 2 found mis-versioning or incompleteness
  - [ ] Expand to include sibling variants if confirmed in Phase 2
- [ ] **Add Oracle Provenance block to archive ReadMe** for the Phase-4 oracle class:
  - Class 1 only → no block needed (oracle in test code)
  - Class 2 (any variant treated as exemplar-as-oracle) → SIMPLNX/DREAM3D version + script/pipeline + seed
  - Class 3 (if literature found) → DOI + edition + equation # + page #; embed PDF
- [ ] **Re-run dual-build with final exemplar.**

### Exit criteria

- ✅ Archive disposition decided
- ✅ Oracle Provenance block in ReadMe (per Phase-4 class)
- ✅ Dual-build pass

---

## Phase 11 — Documentation Review

**Goal**: bring user docs current with Phase-9 deviations and any paper reference.

### Tasks

- [ ] **Invoke `bluequartz-skills:review-filter-docs`**.
- [ ] **Confirm doc accuracy** against current implementation (Phase 1 §"Source files" notes the doc is rated "Excellent" by audit — worked example, 3 diagrams, parameter table; only defect is the empty References section).
- [ ] **Populate References section**: paper citation if Phase 2 found one; else "No external paper reference; see worked example and code documentation".
- [ ] **Add "Known differences from DREAM3D 6.5" section** linking the public Phase-9 Deviation entries (only if Phase 9 produced any).
- [ ] **Verify worked-example numbers in user doc match test-source constants** (Phase 1 §"Algorithm summary"). They should; flag and reconcile any drift.

### Exit criteria

- ✅ Review skill complete
- ✅ References section populated
- ✅ Phase-9 Deviations linked (if any exist)
- ✅ Worked-example numbers consistent with test source

---

## Phase 12 — Archive

**Goal**: assemble OneDrive archive folder with all data, scripts, papers, and a complete ReadMe.

### Tasks

- [ ] **Invoke `bluequartz-skills:archive-filter-verification`**.
- [ ] **Add V&V-policy-required fields to archive ReadMe**:
  - Algorithm Relationship (Phase 3)
  - Oracle class + rationale (Phase 4)
  - Oracle Provenance block (Phase 10, for Classes 2/3/5)
  - Second-engineer review or skip reason (Phase 4)
  - Promoted-artifacts list (Phase 2 — extensive for this filter)
  - Reproduction instructions
- [ ] **Confirm archive folder contents**: Phase-5 fixtures, working artifacts, Phase-6 + Phase-9 pipelines (NX and 6.5.172), Phase-9 comparison report, paper PDF if applicable, ReadMe.
- [ ] **Upload to OneDrive** as `ComputeGroupingDensity_VandV/`.

### Exit criteria

- ✅ Archive assembled with all artifacts
- ✅ ReadMe complete
- ✅ Uploaded; OneDrive path recorded in Phase 13

---

## Phase 13 — Update tracking artifacts

**Goal**: close the loop.

### Tasks

- [ ] **Update Status line at top** of this doc: `*Status:* **DRAFT — ...**` → `*Status:* **COMPLETE — V&V finished YYYY-MM-DD.**`. File stays at `src/Plugins/SimplnxCore/vv/ComputeGroupingDensityFilter.md`.
- [ ] **Record OneDrive archive path**:
  > *OneDrive:*
- [ ] **Update [`docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md`](../../../../docs/vv_retroactive_reports/ComputeGroupingDensityFilter.md)** — DRAFT → confirmed; populate confirmed Algorithm Relationship, Oracle class, V&V status table, Deviation entries (or retraction notes).
- [ ] **Update [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md)** — move filter to confirmed row; update at-a-glance metrics. No bug flag to clear.
- [ ] **Update memory** (`mtr_vv_policy.md`) noting ComputeGroupingDensity is the first filter to complete the full vv-filter workflow.
- [ ] **Note in team V&V deliverable tracker.**

### Exit criteria

- ✅ Status updated; OneDrive recorded
- ✅ Retroactive report DRAFT → confirmed
- ✅ INDEX updated
- ✅ Memory updated
- ✅ Phase Summary block below filled in

---

## Phase Summary *(fill in at end)*

```markdown
# V&V Complete: ComputeGroupingDensityFilter

## Algorithm Relationship
<Port|Minor|Rewrite|New filter> — <one-line evidence>

## Oracle
Class: <e.g., 1 + 4>
Justification: <one-line>
Second-engineer review: <named, date> | <skipped: reason>

## Phase Results (1-line per phase)
| Phase | Status | Notes |
|---|---|---|
| 1–13 | ... | ... |

## Outstanding
<Any deferred / known limitations>
```
