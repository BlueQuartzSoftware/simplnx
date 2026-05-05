# V&V Working Document: ComputeFeatureFaceMisorientationFilter

*Status:* **DRAFT — Phase 1 (Discovery) complete; Phases 2–13 are work orders for the engineer to execute.**

This document is the engineer's working V&V doc for `ComputeFeatureFaceMisorientationFilter`. It follows the workflow in `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`. The skill itself is brand-new and unreviewed; this document is its first trial run as well as your work order.

**How to use this document:**
- Each phase has a **Goal**, **Required reading**, **Tasks** (with checkboxes you literally check off as you go), and **Exit criteria** (when you can move to the next phase).
- Where a "starting point" is provided, you can confirm it or replace it — not start from a blank page.
- Update this document as you work. It IS the deliverable.
- When all phases are complete, follow Phase 13 to update the Status line at the top to "COMPLETE". The file stays at this location.

**Source-of-truth references** (you'll consult these throughout — paths are relative to this doc's location at `src/Plugins/OrientationAnalysis/vv/`):
- Policy: [`docs/vv_templates/mtr_filter_verification_validation.md`](../../../../docs/vv_templates/mtr_filter_verification_validation.md)
- Oracle quick-reference: [`docs/vv_templates/oracle_classes_quick_reference.md`](../../../../docs/vv_templates/oracle_classes_quick_reference.md)
- Audit cross-cutting findings: [`docs/vv_retroactive_reports/INDEX.md`](../../../../docs/vv_retroactive_reports/INDEX.md) "Cross-cutting findings" section
- Skill (workflow): `/Users/mjackson/Workspace1/Claude_Support/skills/vv-filter/SKILL.md`

---

## Phase 1 — Discovery *(complete — verify findings below)*

**Goal**: locate every file related to this filter and identify the legacy DREAM3D equivalent.

### Tasks (verify the discovery findings)

- [ ] Confirm the metadata table below is accurate (UUID, plugin, legacy SIMPL UUID + ClassName)
- [ ] Confirm every file in the "Source files" list still exists at the cited path
- [ ] Confirm the test exemplar archive is accurate (run `grep download_test_data` on `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`)
- [ ] Confirm the TEST_CASE inventory matches the test file
- [ ] Confirm the material PR list — re-run `git log --since=2025-10-01 --oneline -- <files>` and check no new PRs landed since this doc was written
- [ ] Read the algorithm summary block and confirm it matches your understanding of `Algorithms/ComputeFeatureFaceMisorientation.cpp`

### Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `f3473af9-db77-43db-bd25-60df7230ea73` |
| SIMPLNX ClassName | `ComputeFeatureFaceMisorientationFilter` |
| SIMPLNX Human Name | Compute Feature Face Misorientation (Face) |
| SIMPL UUID | `7cd30864-7bcf-5c10-aea7-d107373e2d40` |
| SIMPL ClassName | `GenerateFaceMisorientation` *(per `OrientationAnalysisLegacyUUIDMapping.hpp`)* |
| Plugin | OrientationAnalysis |
| Tier | Tier-2 catalog filter (not in MTR SBIR list, not in audit's 22-filter scope) |
| V&V Mode | Fresh V&V pass (no retroactive report) |

### Source files

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureFaceMisorientationFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/ComputeFeatureFaceMisorientationTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/ComputeFeatureFaceMisorientationFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/ComputeFeatureFaceMisorientationFilter.json`
- `src/Plugins/OrientationAnalysis/docs/ComputeFeatureFaceMisorientationFilter.md`
- Pipeline reference: `src/Plugins/OrientationAnalysis/pipelines/Small_IN100_Processing/(07) Small IN100 Mesh Statistics.d3dpipeline`
- No Python binding found

### Test exemplar archive

- **Archive name**: `6_6_Small_IN100_GBCD.tar.gz` (shared with multiple GBCD-related tests)
- **Source file used**: `6_6_Small_IN100_GBCD.dream3d` (legacy 6_6-prefixed; loaded via `UnitTest::LoadDataStructure`)
- **Comparison form in existing test**: loads the legacy `.dream3d`, runs `ConvertOrientationsFilter` to derive AvgQuats from AvgEulerAngles, then runs the filter and compares the new `SurfaceMeshFaceMisorientationColors` array against the legacy-included `FaceMisorientationColors` in the same `.dream3d`.
- **Provenance status**: **Unknown** — see Phase 2 investigation tasks.

### TEST_CASE inventory (in `ComputeFeatureFaceMisorientationTest.cpp`)

1. `Valid filter execution` — happy path on Small IN100 GBCD; compares `Output Array` against legacy `FaceMisorientationColors`
2. `Invalid filter execution` — error-path tests (inconsistent tuple dimensions, etc.)
3. `SIMPL Backwards Compatibility` — added by PR #1588; exercises 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`

### Material PRs since 2025-10-01

| PR | Date | Title | Notes for this filter |
|---|---|---|---|
| #1438 | 2025-10-25 | ENH: Microtexture related filter cleanup | **Inspect carefully** — audit-wide finding: this PR has been the largest hidden-deviation hotspot |
| #1472 | 2025-11-24 | ENH: Update to EbsdLib 2.0.0 API | **Promoted out of standard prune list** — this filter delegates load-bearing math to EbsdLib (`LaueOps::calculateMisorientation`). Audit findings show #1472 has variable behavior across filters (sometimes pure renames, sometimes wholesale rewrites or precision changes). Inspect scoped diff during Phase 2 |
| #1588 | 2026-04-22 | ENH: SIMPL Backwards Compatibility Test Redesign | Added per-filter SIMPL conversion test + 6.4/6.5 fixture JSONs. Always promoted |
| #1457 | 2025-10-22 | STY: Clean up 'static inline' from filter headers | Pruned (style only) |
| #1538 | 2026-02-19 | ENH: Replace cmake subprocess tar.gz extraction with zlib | Pruned (test infrastructure only) |

### Algorithm summary (from source inspection of `Algorithms/ComputeFeatureFaceMisorientation.cpp`)

For each triangle in a `Triangle Geometry`, compute the misorientation between the two features on either side of the triangle (via `SurfaceMeshFaceLabels`). Output is a 1-component scalar per triangle (`Misorientation`):
- `[angle°]` angle in degrees.

**Restrictions / silent behaviors observed in source**:
- Only `Hexagonal_High` and `Cubic_High` Laue groups are supported (line 67). Other Laue groups silently produce `(0, 0, 0)` with no warning.
- Mixed-phase triangles (`phase1 != phase2`) silently produce `(0, 0, 0)` with no warning.
- Triangles where either `feature1 ≤ 0` or `feature2 ≤ 0` (e.g., feature 0 background or bad data) silently produce `(0, 0, 0)`.
- All three "silent (0,0,0)" cases are indistinguishable in the output from a legitimate zero misorientation.
- Per-feature input quaternions are `float32`; algorithm promotes to `QuatD` (double) before calling `LaueOps::calculateMisorientation`. Lossless promotion, but the misorientation math runs in double precision.
- Parallelized via `ParallelDataAlgorithm` over triangles. Per project thread-safety guidance, `DataArray::operator[]` is not formally thread-safe — verify during algorithm review (Phase 7).

### Exit criteria (when you can move to Phase 2)

- ✅ All Phase 1 verification checkboxes above are checked
- ✅ Any new findings or corrections are noted in the metadata, source files, or PR list

---

## Phase 2 — Promote existing work product

**Goal**: inventory what V&V evidence already exists for this filter so you don't redo work that's been done. Per policy line 262: *"Starting from scratch is the wrong default for retroactive work. The effort is to make existing evidence legible and citeable, not to redo every verification from zero."*

**Required reading**:
- Policy lines 248–262 — "Promote existing work product — don't restart"

### Tasks

- [ ] **Investigate provenance of `6_6_Small_IN100_GBCD.tar.gz`**:
  - [ ] Find when and by whom it was generated (`git log --all --follow -- src/Plugins/OrientationAnalysis/test/CMakeLists.txt | grep -B2 6_6_Small_IN100_GBCD`)
  - [ ] Determine which SIMPL/DREAM3D version produced the embedded `FaceMisorientationColors` array
  - [ ] Decide one of:
    - [ ] **Keep as-is**, classify as Class 5 with legacy origin documented (e.g., "Generated by DREAM3D 6.X.X by ___ on YYYY-MM-DD")
    - [ ] **Regenerate** from the verified Phase-6 oracle, classify as Class 2 with new script
    - [ ] **Augment**: keep legacy archive but add a regenerated copy alongside it for diff comparison
  - [ ] Cross-cutting reference: per the audit's circular-oracle finding (`docs/vv_retroactive_reports/INDEX.md` cross-cutting findings #4), several existing exemplar archives were regenerated from post-fix SIMPLNX output. Confirm whether `6_6_Small_IN100_GBCD.tar.gz` has the same problem.
- [ ] **Inventory other existing artifacts** that can be retrofit-promoted into the new format. For each, record promote / augment / replace decision in the table below:

| Existing artifact | Candidate promotion class | Decision (promote/augment/replace) | Notes |
|---|---|---|---|
| Legacy exemplar `6_6_Small_IN100_GBCD.tar.gz` | Class 5 (legacy expert) OR Class 2 (script) | Replace with small focused data set | See provenance investigation above |
| EbsdLib's `LaueOps::calculateMisorientation` (the underlying math library) | Class 3 (Rowenhorst 2015 if EbsdLib's implementation is sourced from that paper) | _______________ | Confirm by reading EbsdLib source or its citation block |
| Existing test "Valid filter execution" — implicit invariants on output magnitude/direction | Class 4 invariants (output magnitude in [0°, max-FZ-angle for Cubic/Hex]) | _______________ | Plan to promote to explicit invariant assertions in Phase 8 |

- [ ] **Look for paper references in source**: grep `Algorithms/ComputeFeatureFaceMisorientation.cpp` and the `.hpp` for citation comments, DOI strings, or `@reference` doxygen tags. Record any found:
  > *Found references:*

- [ ] **Look for prior comparison notes**: check OneDrive for any folder named `ComputeFeatureFaceMisorientation*` or `GenerateFaceMisorientation*`. Check Slack/email archives. Record what's found:
  > *Found notes:*

### Exit criteria (when you can move to Phase 3)

- ✅ Provenance of `6_6_Small_IN100_GBCD.tar.gz` is determined and a keep/regenerate/augment decision is made
- ✅ Promotion table above is filled in for each artifact
- ✅ Source has been searched for paper references (and any found are recorded)
- ✅ OneDrive/Slack/email checked for prior notes

---

## Phase 3 — Algorithm Relationship classification

**Goal**: classify how SIMPLNX relates to the legacy DREAM3D 6.5.172 implementation. This becomes the opening line of the eventual Phase 9 legacy comparison report.

**Required reading**:
- Policy lines 98–113 — full description of the four classifications (Port / Minor / Rewrite / New filter)

### Tasks

- [ ] **Open the legacy SIMPL filter source** for `FindFaceMisorientation`. Compare the algorithm body to the SIMPLNX `Algorithms/ComputeFeatureFaceMisorientation.cpp`.
- [ ] **Inspect PR #1472's scoped diff for this filter** to determine whether the EbsdLib 2.0.0 update was:
  - [ ] Pure API rename (e.g., `LaueOps::` → `ebsdlib::LaueOps::`) → consistent with **Port** classification
  - [ ] Substantive math change (e.g., precision change, different code path inside `calculateMisorientation`) → may warrant **Minor changes** classification
  - [ ] Wholesale rewrite of the call → may warrant **Rewrite** classification (reread policy lines 104 — rewrite that diverges from legacy is a red flag)
  - Run: `git show 413e6fa46 -- src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.cpp`
- [ ] **Inspect PR #1438's scoped diff for this filter** for the same reasons. Run: `git show e6896714b -- src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.cpp`
- [ ] **Pick the classification** and record it here:
  - [ ] Port
  - [ ] Minor changes
  - [ ] Rewrite
  - [ ] New filter, no legacy equivalent
- [ ] **Write the opening line** for the eventual Phase 9 report. Use the format from policy lines 107–113. Confirm or replace the suggested starting point below:
  > *Suggested starting point* — *Algorithm Relationship: **Port** — line-by-line translation from SIMPL `GenerateFaceMisorientation`, with the `LaueOps::calculateMisorientation` call reformulated through EbsdLib 2.0.0 API renames in PR #1472. No intended change in output.*
  >
  > *Your final opening line:*

### Exit criteria (when you can move to Phase 4)

- ✅ PR #1472 scoped diff inspected
- ✅ PR #1438 scoped diff inspected
- ✅ Classification chosen and checked
- ✅ Opening line written and recorded above

---

## Phase 4 — Oracle classification

**Goal**: pick the oracle class(es) that will define "correct" for this filter, independent of any DREAM3D version.

**Required reading**:
- [`docs/vv_templates/oracle_classes_quick_reference.md`](../vv_templates/oracle_classes_quick_reference.md) — one-page summary of all 5 classes
- Policy lines 25–82 — full text on oracle classes and provenance

### Tasks

- [ ] **Read the algorithm source** (`Algorithms/ComputeFeatureFaceMisorientation.cpp`) and answer: what mathematical operation does this filter perform? Write your one-sentence answer here:
  > *Your answer:*
- [ ] **Walk through each of the 5 classes** (using the quick-reference) and decide if it's a candidate oracle for this filter:
  - [ ] Class 1 (Analytical) — Is there a closed-form expected output computable on toy input? *(For this filter: yes, for special cases like identity quaternions and symmetry-equivalent quaternions. See the recommended supplemental class section below.)*
  - [ ] Class 2 (Reference implementation) — Does a trusted external library produce the expected output? *(For this filter: probably not needed — the math is fully captured by the paper and invariants below.)*
  - [ ] Class 3 (Paper-based) — Does the filter reproduce a published figure, table, or equation? *(For this filter: yes — Rowenhorst et al. 2015 defines the symmetry-equivalent disorientation calculation that EbsdLib's `LaueOps::calculateMisorientation` implements.)*
  - [ ] Class 4 (Invariant-based) — Are there derivable properties the output must satisfy? *(For this filter: yes, multiple. See list below.)*
  - [ ] Class 5 (Expert-visual) — Last resort, only if no class 1–4 is feasible. *(For this filter: not needed.)*
- [ ] **Pick a primary class and any companion classes**. The recommended stack for this filter is **Class 3 (primary) + Class 4 + Class 1** (rationale below). Confirm or replace:
  > *Your decision:*
- [ ] **If you picked Class 5**: write a justification for why no class 1–4 is feasible, per policy line 37. Record it here AND copy into the Phase 12 archive ReadMe.
- [ ] **Identify a second engineer to review your oracle choice**. Per policy line 39, oracle design "really should" be reviewed by a second engineer because the filter author is the least likely person to notice a wrong oracle:
  - [ ] Reviewer identified: name + date scheduled = ___________
  - [ ] OR skip review (no second engineer available): write the skip reason here. It will be copied into the Phase 12 archive ReadMe per policy line 39.
    > *Skip reason:*

### Recommended oracle stack for this filter (starting point)

**Primary: Class 3 (Paper-based)** — Rowenhorst, Rollett, Rohrer, Groeber, Jackson, Konijnenberg, De Graef (2015), *"Consistent representations of and conversions between 3D rotations"*, Modelling Simul. Mater. Sci. Eng. 23 083501. DOI `10.1088/0965-0393/23/8/083501`. EbsdLib's `LaueOps::calculateMisorientation` implements that paper's symmetry-equivalent disorientation computation.
- *Provenance requirement (recorded in Phase 10)*: DOI + edition + equation # + page #, embed paper PDF in archive.

**Companion: Class 4 (Invariant)** — multiple natural invariants, all assertable directly in test code:
- Output 3-vector magnitude (i.e. misorientation angle in degrees) ∈ [0°, max-fundamental-zone-angle]:
  - Cubic_High max FZ angle ≈ 62.8°
  - Hexagonal_High max FZ angle ≈ 93.8°
- Triangle (feature_i, feature_j) and (feature_j, feature_i) produce same magnitude.
- Triangles where feature_id ≤ 0 OR phases differ OR phase is not Hex/Cubic produce exactly `(0, 0, 0)`.
- Two features with identical orientation produce magnitude = 0°.

**Supplemental: Class 1 (Analytical)** — hand-derivable spot checks:
- Two cubic features with identity quaternions → output = `(0, 0, 0)`.
- Two cubic features with quaternions differing by exactly one symmetry operator → magnitude = 0° (FZ collapse).
- Two cubic features with quaternions differing by 5° about [111] → magnitude = 5°.

### Exit criteria (when you can move to Phase 5)

- ✅ Primary oracle class chosen and recorded
- ✅ Companion classes (if any) chosen and recorded
- ✅ Class 5 justification recorded if applicable
- ✅ Second-engineer reviewer identified, OR skip reason recorded
- ✅ Phase 12 archive checklist updated with: oracle class, second-engineer review status, Class 5 justification (if applicable)

---

## Phase 5 — Toy data design + independent expected output

**Goal**: design minimum-size synthetic inputs that exercise each code path, and compute the expected output for each one *without running any DREAM3D version*. The output of this phase is a set of fixtures the engineer will hand to Phase 6.

**Required reading**:
- Policy lines 41–64 — Step 0 a–c (the strict ordering: classify → toy data → independent oracle computation)

**Critical discipline**: Do **NOT** run SIMPLNX or DREAM3D on these fixtures during Phase 5. The expected output must be derived from the oracle alone (paper / hand calculation / invariant predicate). This is the discipline that prevents "the test code unconsciously gets designed around the code that already exists" (policy line 39).

### Tasks

- [ ] **Build each fixture in code** (or in a working notebook). Each fixture must be small (a few triangles, a handful of features, single Laue group unless mixed-phase is the test target). Use the recommended fixture set below as a starting point — add or remove as you understand the algorithm better.
- [ ] **For each fixture, compute the expected output independently** using the oracle class chosen in Phase 4:
  - Class 1 fixtures: hand derivation. Embed derivation comments alongside the eventual `REQUIRE` in test code.
  - Class 4 fixtures: encode the invariant predicate as code (`REQUIRE(magnitude(output) <= 62.8f)`)
  - Class 3 fixtures: cite the Rowenhorst equation; record DOI + equation # + page in the working artifact folder.
- [ ] **Save oracle artifacts** into a working folder for this filter at `src/Plugins/OrientationAnalysis/vv/ComputeFeatureFaceMisorientationFilter_artifacts/`:
  - [ ] Hand-derivation notes for Class 1 fixtures (markdown)
  - [ ] Reference to Rowenhorst paper PDF for Class 3 fixtures (embed the PDF)
  - [ ] Per-fixture expected-output table (markdown or JSON)
- [ ] **Record each fixture's expected output** in the table below. Leave "SIMPLNX actual" blank — that gets filled in Phase 6.

### Recommended fixture set (starting point — confirm or expand)

| ID | Description | Class | Inputs | Expected output |
|---|---|---|---|---|
| A | Identity invariant | 1, 4 | 1 triangle, 2 features both phase 1 (cubic), both quaternion `(0,0,0,1)` | `(0, 0, 0)` magnitude 0° |
| B | Known small rotation | 1 | 1 triangle, 2 features both phase 1 (cubic). Feature 1: identity. Feature 2: 5° about [111] | 3-vector magnitude 5°, axis ≈ `(1/√3, 1/√3, 1/√3)` × 5° |
| C | Symmetry-equivalent | 4 | 1 triangle, 2 features both phase 1 (cubic). Feature 1: identity. Feature 2: pure 90° about [001] (cubic symmetry op) | magnitude 0° (FZ collapse) |
| D | Mixed-phase silent zero | 4 | 1 triangle, 2 features in different phases | exactly `(0, 0, 0)` |
| E | Unsupported Laue silent zero | 4 | 1 triangle, 2 features same phase, phase set to e.g. `Trigonal_Low` | exactly `(0, 0, 0)` |
| F | Bad-data feature ID silent zero | 4 | 1 triangle, `feature_id = 0` on one side | exactly `(0, 0, 0)` |
| G | Hex c-axis | 1, 3 | 1 triangle, 2 features both phase Hex_High. Feature 1: identity. Feature 2: 30° about [001] | TBD per Rowenhorst Eq. for hex disorientation; cite the equation |
| H | Vertex-order symmetry | 4 | Same as Fixture B but with `face_labels = [feature2, feature1]` | same magnitude as B (axis sign may flip — confirm what `LaueOps::calculateMisorientation` returns under arg-swap) |

### Exit criteria (when you can move to Phase 6)

- ✅ At least one fixture per oracle class chosen in Phase 4
- ✅ Every silent-(0,0,0) code path has at least one fixture (D, E, F above)
- ✅ Expected output recorded for every fixture in the table above
- ✅ Working artifact folder created with hand-derivations, paper PDF, fixture-output table
- ✅ Second-engineer review (if scheduled in Phase 4) signed off on the oracle artifacts

---

## Phase 6 — SIMPLNX vs oracle reconciliation

**Goal**: run SIMPLNX on each Phase-5 fixture and confirm the output matches the independently-derived expected output. Resolve any discrepancy by fixing SIMPLNX (or fixing the oracle if it was wrong) before moving to Phase 7. **This is the gate that establishes SIMPLNX correctness independently of DREAM3D 6.5.172.**

**Required reading**:
- Policy lines 46–47 — Step 0d ("Run DREAM3DNX/SIMPLNX on the toy data and compare against the oracle. Resolve any discrepancy in SIMPLNX before moving on.")

### Tasks

- [ ] **Build a `.d3dpipeline` or unit-test fixture** that runs the filter on each Phase-5 fixture. The unit-test form is preferred because it lands directly in the test suite for Phase 8.
- [ ] **Execute each fixture** (via `nxrunner` for pipelines, or `ctest -R "ComputeFeatureFaceMisorientation"` for tests) and record actual outputs in the table below.
- [ ] **Compare each actual output against the Phase-5 expected output** at the appropriate tolerance:
  - Class 1 (Analytical): bit-exact for integer types; ULP-bound for float (typically `Catch::Approx(...).margin(1e-4f)`)
  - Class 4 (Invariant): predicate evaluation
  - Class 3 (Paper-based): use the tolerance the paper itself uses for numerical examples
- [ ] **For each discrepancy**, decide one of:
  - [ ] SIMPLNX bug → fix it now in `Algorithms/ComputeFeatureFaceMisorientation.cpp`. Add a failing test that catches the bug to the test suite (Phase 8 will encode it permanently).
  - [ ] Oracle bug → fix the oracle, return to Phase 5 to update fixture and expected output.
  - [ ] Misunderstanding of algorithm intent → revisit Phase 3 (Algorithm Relationship) and re-derive the expected output.
- [ ] **Re-run all fixtures** after any fix to confirm everything matches.

### Reconciliation table

| Fixture ID | Expected (oracle) | SIMPLNX actual | Match? | Action / resolution |
|---|---|---|---|---|
| A — Identity | `(0,0,0)` magnitude 0° | _______ | _______ | _______ |
| B — 5° about [111] | magnitude 5°, axis ≈ `(1/√3, 1/√3, 1/√3)` × 5° | _______ | _______ | _______ |
| C — Symmetry-equivalent | magnitude 0° | _______ | _______ | _______ |
| D — Mixed-phase | exactly `(0,0,0)` | _______ | _______ | _______ |
| E — Unsupported Laue | exactly `(0,0,0)` | _______ | _______ | _______ |
| F — Bad-data feature | exactly `(0,0,0)` | _______ | _______ | _______ |
| G — Hex 30° about [001] | TBD per Rowenhorst | _______ | _______ | _______ |
| H — Vertex-order swap | same magnitude as B | _______ | _______ | _______ |

### Exit criteria (when you can move to Phase 7)

- ✅ Every fixture from Phase 5 has been run through SIMPLNX
- ✅ Every discrepancy has been resolved (SIMPLNX fixed, oracle fixed, or algorithm intent re-derived)
- ✅ Every fixture matches the oracle on the second pass (after any fix)
- ✅ Any bugs surfaced and fixed have a corresponding failing-test that's now passing — these tests carry forward into Phase 8

---

## Phase 7 — Algorithm Review

**Goal**: a quality pass on already-correct code (comments, naming, memory, progress messaging, cancel checks). Correctness was already established in Phase 6 — this phase is about code quality, not correctness.

**Required reading**:
- The `bluequartz-skills:review-algorithm` skill description and any algorithm-review guidance it surfaces

### Tasks

- [ ] **Invoke the `bluequartz-skills:review-algorithm` skill** with the filter name `ComputeFeatureFaceMisorientationFilter`. The skill produces a structured report and an interactive fix checklist.
- [ ] **Bring the following pre-flagged items to the review** as starting concerns. These were surfaced from Phase 1 source inspection:
  - [ ] **Thread safety**: `ParallelDataAlgorithm` over triangles. Output array `m_Colors` is written at distinct indices (3*i+0, 3*i+1, 3*i+2 per triangle i) but per project guidance (`CLAUDE.md` and `bluequartz-skills:thread-safety`) `DataArray::operator[]` is not formally thread-safe even at distinct indices. **Verify** whether this filter is actually safe in practice; if not, document the risk or fix.
  - [ ] **No cancel check in inner loop**: `generate(start, end)` runs to completion per chunk with no `m_ShouldCancel` check. For large meshes this prevents timely cancellation. (Same pattern surfaced as audit finding in other filters; see `docs/vv_retroactive_reports/`.)
  - [ ] **No progress messaging**: the filter does not emit any progress messages during execution. For large meshes (millions of triangles) the user has no feedback.
  - [ ] **Silent (0,0,0) output for mixed phase / unsupported Laue / bad feature ID**: undocumented restriction. Decide one of: add explicit warnings/errors at preflight, OR document the behavior in the user-facing markdown (Phase 11).
- [ ] **Address every Critical and Warning finding** from the review skill (fix, OR explicitly defer with documented rationale).

### Exit criteria (when you can move to Phase 8)

- ✅ `review-algorithm` skill run to completion
- ✅ Every Critical and Warning finding has either been fixed or explicitly deferred with a documented rationale
- ✅ Pre-flagged items above all addressed (fixed, documented, or deferred)

---

## Phase 8 — Unit Test Review & Implementation

**Goal**: encode the Phase-5 oracle and the Phase-6 reconciliation into the unit test suite so they run on every build. This is where the oracle "lives in the repository" per policy table at lines 55–63.

**Required reading**:
- Policy lines 206–219 — Unit Test Review section
- `bluequartz-skills:plan-filter-tests` and `bluequartz-skills:implement-filter-tests` skill descriptions
- `bluequartz-skills:dual-build-protocol` skill description

### Tasks

- [ ] **Promote each existing test** to encode the chosen oracle:
  - [ ] `Valid filter execution` — currently a Class-5-style legacy-agreement test. Promote by adding the explicit Class-1+3+4 invariant assertions from Phase 5 fixtures alongside the existing legacy comparison.
  - [ ] `Invalid filter execution` — error-path coverage. Probably keep as-is.
  - [ ] `SIMPL Backwards Compatibility` — conversion-only, added by PR #1588. Keep.
- [ ] **Add a new TEST_CASE for each Phase-5 fixture**. Build the fixture as a tiny in-memory DataStructure (no exemplar archive needed for Class 1/4 fixtures). Encode the expected output as a `REQUIRE` per the table at policy lines 55–63:
  - [ ] Class 1 fixtures (A, B, F): `REQUIRE(value == X)` with derivation comment
  - [ ] Class 4 fixtures (C, D, E, H): `REQUIRE(predicate(...))` (e.g., `REQUIRE(magnitude <= 62.8f)`)
  - [ ] Class 3 fixtures (G): `REQUIRE(value == Catch::Approx(X).margin(...))` with citation comment (DOI + equation #)
- [ ] **Wrap every `getDataRefAs<T>()` call in `REQUIRE_NOTHROW()`** per `.claude/CLAUDE.md`.
- [ ] **For any bug surfaced during Phase 6**: ensure the failing-test → fix → green-test cycle has landed. The test that originally caught the bug stays in the suite as the regression pin.
- [ ] **Invoke `bluequartz-skills:plan-filter-tests`** if there are remaining gaps (parameter combinations not exercised, edge cases not covered).
- [ ] **Invoke `bluequartz-skills:implement-filter-tests`** to fill the gaps the plan identifies.
- [ ] **Run the dual-build protocol** per `bluequartz-skills:dual-build-protocol`:
  - [ ] In-core build passes all `ComputeFeatureFaceMisorientation*` tests
  - [ ] Out-of-core build passes all `ComputeFeatureFaceMisorientation*` tests

### Pre-built sample assertion forms

```cpp
// Class 1 example (Fixture A, identity invariant):
// All quaternions = identity → axis-angle is (axis, 0°) → output components all 0
REQUIRE(faceMisorientationColors[0] == 0.0f);
REQUIRE(faceMisorientationColors[1] == 0.0f);
REQUIRE(faceMisorientationColors[2] == 0.0f);

// Class 4 example (Fixture H, vertex-order symmetry):
const float magB = std::sqrt(/* sum of squares of fixture B's output */);
const float magH = std::sqrt(/* sum of squares of fixture H's output */);
REQUIRE(magB == Catch::Approx(magH).margin(1e-4f));

// Class 3 example (Fixture G, Rowenhorst-cited):
// Rowenhorst et al. (2015), Eq. <TBD>: hex disorientation for 30° about [001] = ...°
REQUIRE(magnitude(faceMisorientationColors, triangle_index) == Catch::Approx(expected_deg).margin(0.01f));
```

### Exit criteria (when you can move to Phase 9)

- ✅ Every Phase-5 fixture is encoded as a TEST_CASE
- ✅ Every existing test has been promoted to use explicit oracle-as-assertion form
- ✅ Every `getDataRefAs<T>()` is wrapped in `REQUIRE_NOTHROW()`
- ✅ Any bug-fix tests from Phase 6 are in the suite and passing
- ✅ Dual-build protocol passes (in-core + OOC)

---

## Phase 9 — Legacy DREAM3D Comparison (diff explanation)

**Goal**: diff SIMPLNX against DREAM3D 6.5.172 on the same toy data and write the result up as user-facing **Deviation entries**. This is **not a correctness check** (correctness was Phase 6's job) — it is a **diff-explanation** exercise to produce migration guidance.

**Required reading**:
- Policy lines 84–162 — full text on Legacy Comparison section, including the Deviation Template format and worked examples
- `bluequartz-skills:compare-legacy-dream3d` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:compare-legacy-dream3d` skill** with the filter name. Use the Phase-5 fixtures as the comparison inputs.
- [ ] **Open the comparison report** with the Algorithm Relationship line you wrote in Phase 3.
- [ ] **For each output difference between SIMPLNX and DREAM3D 6.5.172**, write a structured Deviation entry per policy lines 117–131:

```
**Deviation ID:** ComputeFeatureFaceMisorientation-D<N>
**Filter UUID:** f3473af9-db77-43db-bd25-60df7230ea73
**Symptom:** <one-sentence user-visible symptom>
**Root cause:** <bug | precision | order of operations | library | algorithmic choice>
**Affected users:** <who actually sees this>
**Recommendation:** <trust SIMPLNX | trust 6.5.172 | either acceptable within tolerance | see quick-patch link for legacy-parity>
```

- [ ] **Use the pre-flagged candidate deviations below** as starting points (most should resolve into either real Deviation entries or be retracted with a "no observed difference" note).

### Pre-flagged candidate deviations (proposed, pending comparison)

> **Deviation ID:** `ComputeFeatureFaceMisorientation-D1`
> **Filter UUID:** `f3473af9-db77-43db-bd25-60df7230ea73`
> **Symptom:** *(if observed)* per-triangle misorientation 3-vector components differ from 6.5.172 by small amount (<0.001°)
> **Root cause:** Precision + Library. Algorithm promotes float32 quaternions to QuatD before calling `LaueOps::calculateMisorientation`; legacy may have run the math in float32. PR #1472 (EbsdLib 2.0.0) also reformulated the call.
> **Affected users:** anyone comparing per-triangle misorientation outputs voxel-for-voxel between DREAM3D and DREAM3DNX
> **Recommendation:** *(TBD pending comparison)* likely "trust SIMPLNX — double precision is more numerically defensible"

> **Deviation ID:** `ComputeFeatureFaceMisorientation-D2`
> **Filter UUID:** `f3473af9-db77-43db-bd25-60df7230ea73`
> **Symptom:** *(if observed)* triangles with non-Hex/non-Cubic phase produce different output between SIMPLNX and 6.5.172
> **Root cause:** Algorithmic choice or Bug. SIMPLNX silently outputs `(0,0,0)` for unsupported Laue groups; legacy behavior unknown — may have errored, may have produced something else, may have been the same.
> **Affected users:** anyone running this filter on multi-Laue-group meshes
> **Recommendation:** *(TBD)* potentially "warn the user at preflight about the Hex/Cubic-only restriction in both versions"

> **Deviation ID:** `ComputeFeatureFaceMisorientation-D3`
> **Filter UUID:** `f3473af9-db77-43db-bd25-60df7230ea73`
> **Symptom:** *(if observed)* mixed-phase triangle outputs differ
> **Root cause:** TBD — same family as D2; depends on legacy behavior on `phase1 != phase2`

### Exit criteria (when you can move to Phase 10)

- ✅ Comparison run on every Phase-5 fixture
- ✅ Each pre-flagged candidate deviation has been either confirmed (becomes a real Deviation entry with all fields filled in), OR explicitly retracted with a "no observed difference" note
- ✅ Comparison report opens with Phase-3 Algorithm Relationship line
- ✅ Every Deviation entry has all required fields filled in (ID, UUID, Symptom, Root cause, Affected users, Recommendation)

---

## Phase 10 — Exemplar Validation & Publishing

**Goal**: validate the exemplar dataset(s) used by the test suite and ensure each archive's ReadMe captures the Oracle Provenance block per policy.

**Required reading**:
- Policy lines 65–82 — Oracle Provenance record requirements per class
- `bluequartz-skills:validate-and-publish-exemplars` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:validate-and-publish-exemplars` skill** with the filter name.
- [ ] **Decide on the exemplar archive** (depends on Phase 2 provenance investigation):
  - [ ] **Keep `6_6_Small_IN100_GBCD.tar.gz` as-is** with a Class-5 oracle ReadMe entry naming the legacy DREAM3D version + date + (if findable) the original engineer
  - [ ] **Regenerate the exemplar** from your verified Phase-6 oracle using a new Class-2 script. Replace `FaceMisorientationColors` in the `.dream3d` with the regenerated version. Document the script in the archive.
  - [ ] **Add a new exemplar alongside** the legacy one for direct diff comparison
- [ ] **Add the Oracle Provenance block to the archive ReadMe**. Required fields per class chosen in Phase 4:
  - [ ] Class 3 (Rowenhorst 2015): DOI `10.1088/0965-0393/23/8/083501` + edition + equation # + page #. Embed paper PDF in the archive.
  - [ ] Class 5 (if keeping legacy archive): named expert + date approved + signed-off screenshots + class-5-only justification (if applicable).
  - [ ] Class 1 and Class 4 fixtures: no provenance block needed — oracle lives in the test code directly.
- [ ] **Watch for circular oracles** per the audit's cross-cutting findings: if the existing `6_6_Small_IN100_GBCD.tar.gz` was generated by a previous (possibly buggy) version of SIMPLNX rather than by legacy DREAM3D, regenerating from the verified Phase-6 oracle is mandatory.
- [ ] **Re-run dual-build tests** with the final exemplar to confirm pass.

### Exit criteria (when you can move to Phase 11)

- ✅ Exemplar archive decision made (keep / regenerate / augment) with rationale
- ✅ Oracle Provenance block added to archive ReadMe per chosen oracle class
- ✅ Paper PDF embedded in archive (for Class 3)
- ✅ Dual-build tests pass with the final exemplar

---

## Phase 11 — Documentation Review

**Goal**: bring the user-facing documentation up to date — accuracy against current implementation, paper references, before/after images, warnings about restrictions, and links to public Deviation entries.

**Required reading**:
- `bluequartz-skills:review-filter-docs` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:review-filter-docs` skill** with the filter name.
- [ ] **Address each pre-flagged gap** below (these were surfaced from Phase 1 source inspection):
  - [ ] Add a "Restrictions" section noting the Hex/Cubic-only Laue group support. Currently the doc implies the filter works for any Laue group.
  - [ ] Add a "Restrictions" note that mixed-phase triangles silently produce `(0, 0, 0)`.
  - [ ] Add a "References" section citing Rowenhorst et al. 2015 (DOI `10.1088/0965-0393/23/8/083501`).
  - [ ] Add a "Known differences from DREAM3D 6.5" section linking the public Deviation entries from Phase 9.
  - [ ] Add a before/after example image showing the filter applied to a small triangle mesh (use DREAM3D-NX's annotation tools per `bluequartz-skills:filter-documentation`).
  - [ ] Consider adding an infographic explaining the unusual axis-angle vector encoding (output 3-vector components are `axis_i * angle_in_degrees`, so magnitude IS the angle). This encoding is non-obvious.
- [ ] **Add the paper citation in two places**: in the user-facing markdown AND in the test code as a comment alongside the Class-3 `REQUIRE` (so the citation travels with both the user docs and the regression pin).

### Exit criteria (when you can move to Phase 12)

- ✅ `review-filter-docs` skill run to completion
- ✅ All pre-flagged gaps addressed
- ✅ Paper citation appears in both user docs and test code
- ✅ Public Deviation entries from Phase 9 are linked from the user docs

---

## Phase 12 — Archive

**Goal**: assemble the complete OneDrive archive with all data, scripts, papers, screenshots, and a ReadMe that captures everything a future engineer needs to reproduce this V&V.

**Required reading**:
- Policy lines 230–234 — Archiving Everything When Finished
- `bluequartz-skills:archive-filter-verification` skill description

### Tasks

- [ ] **Invoke the `bluequartz-skills:archive-filter-verification` skill** with the filter name.
- [ ] **Add the V&V-policy-required fields** to the archive ReadMe (in addition to what `archive-filter-verification` produces by default):
  - [ ] **Algorithm Relationship** line from Phase 3 (e.g., "Port — line-by-line translation from SIMPL `GenerateFaceMisorientation`")
  - [ ] **Oracle class** from Phase 4 (Class 3 primary + Class 1+4 companions) with rationale
  - [ ] **Oracle Provenance block** from Phase 10 (for classes 2/3/5)
  - [ ] **Second-engineer review** from Phases 4–5: named reviewer + date, OR documented skip reason
  - [ ] **Class-5-only justification** if applicable
  - [ ] **Promoted artifacts** from Phase 2: list what existed before vs. what was created new
  - [ ] **Reproduction instructions**: how to regenerate the toy data, oracle output, and comparison from scratch
- [ ] **Confirm everything is in the archive folder**:
  - [ ] All Phase-5 fixtures (input data + expected output)
  - [ ] Working artifacts from `src/Plugins/OrientationAnalysis/vv/ComputeFeatureFaceMisorientationFilter_artifacts/`
  - [ ] DREAM3D-NX pipeline files used in Phase 6 reconciliation and Phase 9 comparison
  - [ ] DREAM3D 6.5.172 pipeline files used in Phase 9 comparison
  - [ ] Comparison report from Phase 9 (with all Deviation entries)
  - [ ] Embedded paper PDF (Rowenhorst 2015) for Class 3 oracle provenance
  - [ ] Any expert sign-off screenshots (Class 5)
  - [ ] The complete ReadMe with all required fields
- [ ] **Upload to OneDrive** in a folder named `ComputeFeatureFaceMisorientationFilter_VandV/`.

### Exit criteria (when you can move to Phase 13)

- ✅ Archive folder assembled with every required artifact
- ✅ ReadMe contains all V&V-policy-required fields above
- ✅ Folder uploaded to OneDrive
- ✅ Upload location recorded in Phase 13 below

---

## Phase 13 — Update tracking artifacts

**Goal**: close the loop on the V&V by updating shared tracking documents so future engineers can see this filter is complete.

### Tasks

- [ ] **Update this document's Status line** at the very top: change `*Status:* **DRAFT — Phase 1 (Discovery) complete; ...**` to `*Status:* **COMPLETE — V&V finished YYYY-MM-DD.**`. The file STAYS at `src/Plugins/OrientationAnalysis/vv/ComputeFeatureFaceMisorientationFilter.md` — status is tracked in the doc header, not by directory location.
- [ ] **Record the OneDrive archive path** here:
  > *OneDrive path:*
- [ ] **If `docs/vv_retroactive_reports/INDEX.md` adds a Tier-2 section** in the future, add a row for this filter there.
- [ ] **Note in the team's V&V deliverable tracker** (location TBD with project lead) that this filter is complete.
- [ ] **Update memory** if this is the first Tier-2 catalog filter through the workflow:
  - [ ] Note in `mtr_vv_policy.md` (auto-memory) that the workflow has been validated on a non-MTR Tier-2 filter

### Exit criteria (V&V is COMPLETE)

- ✅ Status line updated to "COMPLETE"
- ✅ OneDrive archive path recorded
- ✅ Team tracker updated
- ✅ Final Phase Summary block (below) filled in

---

## Phase Summary *(fill in at end of V&V pass)*

```markdown
# V&V Complete: ComputeFeatureFaceMisorientationFilter

## Algorithm Relationship
<Port|Minor|Rewrite|New filter> — <one-line evidence>

## Oracle
Class: <e.g., 3 (paper-based) + 4 (invariant) + 1 (analytical)>
Justification: <one-line>
Second-engineer review: <named, date> | <skipped: reason>

## Phase Results

| Phase | Status | Notes |
|-------|--------|-------|
| 1 Discovery | Complete | <count> source files, fresh V&V mode |
| 2 Promote existing | Complete | <count> artifacts promoted, <count> created new |
| 3 Algorithm Relationship | Confirmed | <classification> |
| 4 Oracle classification | Confirmed | Class <N> + companions |
| 5 Toy data + expected | Complete | <count> fixtures, oracle artifacts in archive |
| 6 SIMPLNX vs oracle | Reconciled | <count> bugs found and fixed during reconciliation |
| 7 Algorithm Review | Complete | <findings count>, <fixed | deferred> |
| 8 Unit Tests | Complete | <count> test cases, dual-build pass |
| 9 Legacy Comparison | Complete | <count> Deviation entries published |
| 10 Exemplar Publishing | Complete | <count> archives, Oracle Provenance recorded |
| 11 Documentation | Complete | <summary of changes> |
| 12 Archive | Complete | OneDrive at <path> |
| 13 Tracking updates | Complete | INDEX + retroactive report updated |

## Outstanding
<Any deferred issues, follow-up work, or known limitations>
```

---

## Notes on this trial run (skill validation — for project lead, not the engineer)

This is the first trial run of the `vv-filter` skill. Items to feed back into skill review:

1. **Phase 1 (Discovery)** worked cleanly for a fresh V&V pass — the discovery sub-steps (UUID, plugin, legacy mapping, files, exemplar archive, TEST_CASE inventory, PR walk) translate well from the audit's per-filter retroactive workflow.
2. **No retroactive report** to consume meant Phase 2 (Promote existing work) had less to inventory. The skill's Phase 2 still applies (existing exemplar to promote, existing tests to upgrade) — so the phase isn't retroactive-only.
3. **Oracle classification (Phase 4)** is where the bulk of the *novel* V&V intellectual work happens for this filter. The skill's per-class prompts (1/2/3/4/5) translate into concrete decisions quickly because the policy text is so specific.
4. **Toy data design (Phase 5)** required ~20 minutes of thinking to enumerate 8 fixtures covering all the silent-(0,0,0) cases plus the analytical and paper-based oracles. This is the biggest engineer-time block in the workflow; the skill could include a fixture-design checklist (identity invariant, symmetry-equivalent, mixed-phase, bad-data-feature, unsupported-Laue, paper-citable analytical case, vertex-order swap).
5. **Pre-flagged Phase 9 deviations** drawn from source inspection were useful — they give the engineer a starting point rather than a blank page when running the legacy comparison.
6. **Working-doc location** — initially used centralized `docs/vv_active/` during the trial run, then moved to plugin-local `src/Plugins/<PluginName>/vv/<FilterName>.md` per project-lead direction so engineers find the V&V doc next to the filter source. Skill updated to reflect plugin-local convention. Status tracked in doc header (DRAFT → COMPLETE), not by moving files between directories.
7. **Engineer-vs-deliverable framing** — first draft read as an audit deliverable; reorganized into work-order checklists with "Required reading", "Tasks", "Exit criteria" per phase based on user feedback. Confirms that the skill should produce engineer-runnable work orders, not just structured templates.
