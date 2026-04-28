# Retroactive V&V: BadDataNeighborOrientationCheckFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `3f342977-aea1-49e1-a9c2-f73760eba0d3` |
| SIMPLNX ClassName | `BadDataNeighborOrientationCheckFilter` |
| SIMPLNX Human Name | Neighbor Orientation Comparison (Bad Data) |
| SIMPL UUID | *(TBD — confirm in legacy SIMPL repo)* |
| SIMPL ClassName | `BadDataNeighborOrientationCheck` (assumed) |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | OrientationAnalysis |

### Source files scanned

- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/BadDataNeighborOrientationCheckFilter.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/BadDataNeighborOrientationCheck.{hpp,cpp}`
- `src/Plugins/OrientationAnalysis/test/BadDataNeighborOrientationCheckTest.cpp`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_4/BadDataNeighborOrientationCheckFilter.json`
- `src/Plugins/OrientationAnalysis/test/simpl_conversion/6_5/BadDataNeighborOrientationCheckFilter.json`
- `src/Plugins/OrientationAnalysis/docs/BadDataNeighborOrientationCheckFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port (with substantive bug fix)** — the SIMPLNX filter is a translation of the legacy SIMPL `BadDataNeighborOrientationCheck` filter, but PR #1499 (REV) explicitly fixed a bug in the iterative-flip loop guard ("BUG: Fix only checking values greater than the supplied min number of neighbors") that may diverge from the legacy DREAM3D 6.5.172 output.
- **Evidence:** UUID-style continuity, identical parameter set (misorientation tolerance, number of neighbors, mask, quats, phases, crystal structures), 6-face neighbor kernel hard-coded. Mid-2026 review (PR #1499) explicitly cleaned up logic and corrected an iterative-loop bug.
- **Action required:** Confirm with `compare-legacy-dream3d` against DREAM3D 6.5.172 on a shared toy dataset to determine whether the fix in PR #1499 represents a Deviation vs. legacy.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs that touched this file but did not change behavior are listed at the bottom of this section but not detailed individually.

### PR #1439 — *"ENH/API: Multi-Dimensional Tuple Support for StringArray and NeighborList"* — merged 2025-10-03

- **Files in this filter:** test (.cpp), 54 lines touched (whitespace-dominant)
- **Change nature:** API refactor — touched the test file as part of repo-wide changes for tuple-dim handling on StringArray/NeighborList. No behavioral change to the filter algorithm.
- **V&V content:** None.

### PR #1457 — *"STY: Clean up 'static inline' from filter headers"* — merged 2025-10-22

- **Files in this filter:** filter (.hpp) only, +7/-7
- **Change nature:** Pure style sweep removing `static inline` from header constants. No behavioral change.
- **V&V content:** None.

### PR #1438 — *"ENH: Microtexture related filter cleanup"* — merged 2025-10-25 *(broad refactor, exception flagged because policy maintainer flags #1438 as "always inspect carefully")*

- **Files in this filter:** test (.cpp), 2 lines touched
- **Change nature:** Per-PR diff stat shows only a 2-line change in the test file (likely include/path/namespace adjustment as part of broader microtexture pipeline cleanup). Substantive correctness fixes in this PR landed on neighboring filters (CAxisSegmentFeatures, ComputeAvgOrientations, FindFeatureReferenceCAxisOrientation, ReadH5Ebsd, etc.), not on `BadDataNeighborOrientationCheck`.
- **V&V content:** Effectively none for this filter, but the PR is flagged for the audit because the bundle of fixes was tied to the broader microtexture-pipeline correctness pass that this filter is also a member of.

### PR #1472 — *"ENH: Update to EbsdLib 2.0.0 API"* — merged 2025-11-24 *(broad refactor, exception flagged because this filter delegates misorientation math to EbsdLib's `LaueOps::calculateMisorientation`)*

- **Files in this filter:** algorithm (.cpp), 16 lines touched
- **Change nature:** EbsdLib API bump. Inspection of the current source confirms the algorithm calls `ebsdlib::LaueOps::GetAllOrientationOps()` and `orientationOps[laueClass1]->calculateMisorientation(quat1, quat2)`. This PR converted those call sites to the 2.0.0 API.
- **V&V content:** No new tests in the diff. The semantic behavior of misorientation calculation is now coupled to EbsdLib 2.0.0. Any V&V claim about this filter is implicitly a V&V claim about that pinned EbsdLib version's `calculateMisorientation` for each Laue group. That dependency must be recorded in the verification archive.

### PR #1499 — *"REV: Bad Data Neighbor Orientation Check"* — merged 2026-02-02

- **Files in this filter:** docs (.md) +41/-, algorithm (.cpp) ~240 lines reworked, filter (.cpp) ~99 lines reworked, test (.cpp) **+1726 / -376** lines (massive expansion), CMakeLists (.txt) +1
- **Change nature:** **Material algorithm review + bug fix + comprehensive test rewrite.** PR description (verbatim, abbreviated): *"Clean up extraneous preflight checks; commenting code and adjusting the logic when to increment the neighbor count; remove extra reads in alg; **BUG: Fix only checking values greater than the supplied min number of neighbors**; performance changes and warning cleanup; documentation cleanup; image table doc cleanup; add a test case to test a real world dataset against 6.5 output; cleaned up test namespace; merged old test case file into new test file; **18 Test Cases to validate base cases**; **Add sequential and recursive test cases**; **Add long recursive/sequential tests and a complete functionality test**; update test and data repo."*
- **V&V content:** **Very high — this is the central V&V event in the filter's history.** The PR (a) explicitly states a behavioral bug was fixed, (b) added a comparison test against legacy 6.5 output ("Case 4" in the test file looks like that test), and (c) bumped the suite from a handful of tests to 28 cases. The current 27-cell hand-derived expected mask in Case 1.1.1 and the 125-cell hand-derived expected mask in Case 4 both originate from this PR. The fact that the PR replaced the data archive (`7_bad_data_neighbor_orientation_check.tar.gz`) is itself a V&V artifact: the previous archive was incompatible with the post-fix output.

### PR #1523 — *"ENH: Factor out the 6-face neighbor code that is systemic through out the code base"* — merged 2026-02-05

- **Files in this filter:** algorithm (.cpp), ~99 lines touched
- **Change nature:** Refactor — extracted the 6-face neighbor offset/validity computation into a shared `NeighborUtilities` header (`initializeFaceNeighborOffsets`, `initializeFaceNeighborInternalIdx`, `computeValidFaceNeighbors`, `VoxelNeighbors<Image3D>::k_FaceNeighborCount`). Behavior preserved. The two neighbor-loop sites in the algorithm now use the shared utilities.
- **V&V content:** Refactor only, but it does change the *blast radius* of any future bug in `NeighborUtilities` — a bug there would now affect every 6-face-neighbor filter simultaneously. Worth recording in the verification archive that this filter consumes the shared utility.

### PR #1538 — *"ENH: Replace cmake subprocess tar.gz extraction with zlib in unit tests"* — merged 2026-02-19

- **Files in this filter:** test (.cpp), 54 lines touched (test sentinel boilerplate updates)
- **Change nature:** Test infrastructure. Updated the sentinel/extraction mechanism. No algorithmic change.
- **V&V content:** None.

### PR #1543 — *"DOC: Update pipeline references in each of the documentation files"* — merged 2026-02-24

- **Files in this filter:** docs (.md), 3 lines touched
- **Change nature:** Doc hygiene — updated example-pipeline filenames in the doc. No algorithmic change.
- **V&V content:** Doc currency only.

### PR #1547 — *"DOC: Fix filter documentation and documentation related code bugs"* — merged 2026-03-10

- **Files in this filter:** docs (.md), 2 lines touched
- **Change nature:** Doc tweak.
- **V&V content:** None.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +48 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/BadDataNeighborOrientationCheckFilter.json` (1031 bytes)
  - `test/simpl_conversion/6_5/BadDataNeighborOrientationCheckFilter.json` (1088 bytes)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test (`OrientationAnalysis::BadDataNeighborOrientationCheckFilter: SIMPL Backwards Compatibility`) that loads the SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) JSON fixtures via `Pipeline::FromSIMPLFile`, builds the filter, and asserts each parameter (misorientation tolerance = 2.5, number of neighbors = 5, all selection paths) was decoded correctly.
- **V&V content:** **Pipeline-conversion correctness only** — verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces the right parameter values. It does **not** verify that the filter's *output* matches legacy. That step is still missing.

### PR #1590 — *"ENH: Standardize 2D Image Handling"* — merged 2026-04-23

- **Files in this filter:** algorithm (.cpp), 11 lines touched
- **Change nature:** Adapted `NeighborUtilities` to be dimensionality-aware so 2D images correctly skip the +/-Z face neighbors. The filter algorithm picked up these changes via the `VoxelNeighbors<Image3D>` / `computeValidFaceNeighbors` calls.
- **V&V content:** **Material — touches 2D semantics.** The doc already warns *"If the user is processing a 2D data set, none of the voxels can have 6 neighbors since there are no neighbors in the +/-Z directions."* Behavior on 2D images should be re-validated; tests should include at least one 2D case if they don't already. (Spot check of the 28 algorithmic test cases is needed to confirm 2D coverage.)

### Pruned PRs

None pruned beyond what's listed above. Every PR that touched the filter since 2025-10-01 is enumerated.

## Test coverage detected

`BadDataNeighborOrientationCheckTest.cpp` contains **29 `TEST_CASE`s**:

**Series 1 — Base cases (18 tests, parameterized over phase/tolerance/min-neighbors):**

1. `Case 1.1.1` — Base Case | 2 phase | Tolerance 5 | 1 Min Neighbors
2. `Case 1.1.2` — Invalid Base Case | 3 phase | Tolerance 5 | 1 Min Neighbors
3. `Case 1.1.3` — Mixed-validity Base Case | 2-phase split | Tolerance 5 | 1 Min Neighbors
4. `Case 1.2.1` — Base Case | 2 phase | Tolerance 5 | 2 Min Neighbors
5. `Case 1.2.2` — 3 phase variant | 2 Min Neighbors
6. `Case 1.2.3` — Mixed validity | 2 Min Neighbors
7. `Case 1.3.1` — Base Case | 3 Min Neighbors
8. `Case 1.3.2` — 3 phase variant | 3 Min Neighbors
9. `Case 1.3.3` — Mixed validity | 3 Min Neighbors
10. `Case 1.4.1` — Base Case | 4 Min Neighbors
11. `Case 1.4.2` — 3 phase variant | 4 Min Neighbors
12. `Case 1.4.3` — Mixed validity | 4 Min Neighbors
13. `Case 1.5.1` — Base Case | 5 Min Neighbors
14. `Case 1.5.2` — 3 phase variant | 5 Min Neighbors
15. `Case 1.5.3` — Mixed validity | 5 Min Neighbors
16. `Case 1.6.1` — Base Case | 6 Min Neighbors
17. `Case 1.6.2` — 3 phase variant | 6 Min Neighbors
18. `Case 1.6.3` — Mixed validity | 6 Min Neighbors

**Series 2 — Sequential / iterative-decay tests (6 tests):**

19. `Case 2.1` — Sequential / multi-iteration | tolerance 5 | min-neighbors 1
20. `Case 2.2` — Sequential | min-neighbors 2
21. `Case 2.3` — Sequential | min-neighbors 3
22. `Case 2.4` — Sequential | min-neighbors 4
23. `Case 2.5` — Sequential | min-neighbors 5
24. `Case 2.6` — Sequential | min-neighbors 6

**Series 3 — Recursive / flood-fill tests (2 tests):**

25. `Case 3.1` — Recursive flood-fill behavior | one variant
26. `Case 3.2` — Recursive flood-fill behavior | second variant

**Series 4 — Real-world / full-functionality test (1 test):**

27. `Case 4` — 5×5×5 hand-derived expected mask, tolerance 5, min-neighbors 4 — the "complete functionality" case described in PR #1499.

**Series 5 — SIMPL backwards-compatibility (1 test, 2 DYNAMIC_SECTIONs):**

28. `SIMPL Backwards Compatibility` — SIMPL 6.4 (Filter_Name) and 6.5 (UUID) conversion paths *(added by PR #1588)*

(Note: counted 28 unique `TEST_CASE` entries; the rubric of "Series 1 = 18 base cases" matches PR #1499's claim verbatim.)

The base-case tests use 3×3×3 (27-cell) and 5×5×5 (125-cell) hand-derived golden masks embedded as `std::array<uint8, N>` in the test source. This makes the test suite an **explicit Class-1 (analytical) oracle** for the small fixtures — the expected output is hard-coded directly, not loaded from an exemplar file.

## Exemplar archive

- **Archive name:** `7_bad_data_neighbor_orientation_check.tar.gz`
- **SHA512:** `60089eecfe679466f63ef46839f194f83185a5987f51a0e23b9670e50d967ae49451bcfa43c0d44d6fb12cd55b73d208b36825251842d2b2568ffe521be12fbe`
- **Referenced in:** `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` (line 139)
- **Used by:** Every `Case 1.*`, `Case 2.*`, `Case 3.*`, and `Case 4` test as the *input* dream3d (`case_*_input.dream3d`). Expected masks are NOT in the archive — they are hand-derived in the test source.
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the input fixtures were generated, whether they were authored by hand or produced by a generator pipeline, and whether a ReadMe is included.)*
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files in their case directories, any pipeline/generator script that produced them, and provenance notes. Promote into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class — primary:** **4 (Invariant-based)**
  - **Monotonicity invariant:** Across one outer iteration the count of "good" voxels (mask == true) is non-decreasing. The algorithm only ever flips false → true, never true → false.
  - **Idempotence invariant:** Running the filter on a fully-good volume (all mask == true) is a no-op. Running on a fully-bad volume (all mask == false) leaves it unchanged because no good neighbors ever exist.
  - **Convergence invariant:** The inner `while(counter > 0)` loop is guaranteed to terminate because (a) the total number of voxels is finite and (b) every iteration that does work flips at least one mask false → true (irreversible).
  - **Tolerance invariant:** Every voxel that gets promoted to "good" had at least N neighbors with same-phase quaternions whose pairwise misorientation (per the appropriate Laue group) was strictly less than the tolerance.
- **Recommended class — companion:** **3 (Paper-based)** for the misorientation-math piece — the per-Laue-group `calculateMisorientation` is documented in Rowenhorst, D., Rollett, A. D., Rohrer, G. S., Groeber, M., Jackson, M. A., Konijnenberg, P. J., & De Graef, M. (2015). *Consistent representations of and conversions between 3D rotations.* Modelling and Simulation in Materials Science and Engineering, 23(8), 083501. The current implementation delegates to `ebsdlib::LaueOps::calculateMisorientation`, which is the EbsdLib expression of that paper.
- **Recommended class — also active:** **1 (Analytical)** — the existing 28 test cases already are analytical-oracle tests, with hand-derived expected masks for 3×3×3 and 5×5×5 fixtures embedded in the test source. These should be preserved and explicitly labeled as the Class-1 oracle of record.
- **Action required:** Developer to defend or replace; in particular to confirm Rowenhorst 2015 is the right paper citation for the EbsdLib `calculateMisorientation` family.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | **Yes — informally** | PR #1499 (`REV:` prefix) is an explicit code review pass. No structured review report on file though. |
| Code path coverage (algorithmic) | **Excellent** | 28 hand-derived test cases covering all min-neighbor values 1–6, single-/multi-phase, sequential/recursive flood-fill behavior, plus a full 5×5×5 functionality test. |
| Code path coverage (SIMPL conversion) | **Yes** | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `7_bad_data_neighbor_orientation_check.tar.gz`. Note: the archive holds *inputs only*; expected outputs are baked into the test source. |
| Exemplar provenance documented | **Unknown** | TBD by inspecting archive contents (no ReadMe inspected here). |
| Oracle class recorded | **No** | This document is the first to propose one (Class 4 + Class 3 + Class 1 stack). |
| Toy data / independent expected output (Step 0 c) | **Yes (implicit)** | The hand-derived `expectedMask` arrays in the test source are exactly toy-data independent expected outputs. Should be promoted to a formal Step 0 c artifact. |
| Legacy comparison report (Step 0 e) | **Partial** | PR #1499 mentions "test against 6.5 output." A full `compare-legacy-dream3d` run vs. DREAM3D 6.5.172 is still not on file as a structured artifact. |
| Deviation entries (`BadDataNeighborOrientationCheck-D<N>`) | **None written** | PR #1499's "BUG: Fix only checking values greater than the supplied min number of neighbors" is a strong Deviation candidate vs. legacy 6.5.172. See proposals below. |
| Documentation currency | Probably current | Doc was rewritten in PR #1499; pipeline list refreshed in PR #1543; minor fix in PR #1547. Needs an audit by `review-filter-docs` to verify accuracy against the post-#1590 (2D-aware) implementation. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle stack.** Class 4 (invariants) + Class 3 (Rowenhorst 2015 for the misorientation math) + Class 1 (existing hand-derived `expectedMask` arrays). Defend or replace. (Step 0 a, b)
2. **Promote existing hand-derived masks to formal Class-1 oracles.** The `std::array<uint8, 27>` and `std::array<uint8, 125>` constants in the test source already are independent expected outputs — declare them as the Step 0 c artifact and document how they were derived (by hand? by reasoning about the algorithm? by running an authoritative reference implementation?). Without the derivation note they're just numbers. (Step 0 c)
3. **Add invariant-based assertions to the existing tests.** For each of the 28 cases, in addition to the cell-by-cell mask comparison, add: (a) `REQUIRE(count_after >= count_before)` (monotonicity); (b) `REQUIRE(no_voxel_flipped_true_to_false)`; (c) idempotence test — run the filter twice and assert the second run is a no-op. (Step 0 d)
4. **Inspect `7_bad_data_neighbor_orientation_check.tar.gz` and document provenance.** Determine how the input `.dream3d` fixtures were generated and whether they originated from a SIMPL-era reference. Write an Oracle Provenance block for the archive ReadMe. (Step 0 c)
5. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same input. The expected outcome is at least one Deviation entry: `BadDataNeighborOrientationCheck-D1` for the iteration-guard fix from PR #1499. (Step 0 e)
6. **2D coverage check.** PR #1590 made `NeighborUtilities` dimensionality-aware. Confirm that at least one of the 28 test cases exercises a 2D image (Z dim = 1) so the 2D path is covered. If not, add one. (Step 0 d)
7. **Pin and record EbsdLib version.** PR #1472 bumped this filter's misorientation math to EbsdLib 2.0.0. Record the exact EbsdLib version used for the V&V evidence in the archive ReadMe so future EbsdLib changes can be regression-tested. (Step 0 a)
8. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `BadDataNeighborOrientationCheck` filter (UUID `3f342977-aea1-49e1-a9c2-f73760eba0d3`); reviewed and corrected in PR #1499 (iteration-guard bug fix); 6-face neighbor logic factored into shared `NeighborUtilities` (PR #1523) and made 2D-aware (PR #1590); misorientation math delegated to EbsdLib 2.0.0 (PR #1472)."*
9. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `BadDataNeighborOrientationCheck-D1`
> **Filter UUID:** `3f342977-aea1-49e1-a9c2-f73760eba0d3`
> **Symptom:** SIMPLNX promotes more voxels (or different voxels) to "good" than SIMPL 6.5.172 on the same input, particularly for `NumberOfNeighbors` < 6.
> **Root cause:** Bug in the iterative-decay loop control in legacy SIMPL — *"only checking values greater than the supplied min number of neighbors"* per PR #1499's bug-fix note. The legacy implementation likely used a strict `>` where it should have been `>=`, causing the final iteration at the user-requested floor to be skipped. SIMPLNX corrects this in PR #1499.
> **Affected users:** Anyone running the filter with `NumberOfNeighbors` strictly less than 6 (i.e., the default usage for the Small IN100 reconstruction pipeline, which uses 4) and comparing against legacy DREAM3D output.
> **Recommendation:** Trust SIMPLNX. Legacy was wrong. A patch to the legacy 6.5.172 algorithm to flip `>` to `>=` is the minimal fix; engineer to evaluate whether legacy patch is needed for users who must reproduce exact legacy counts.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits the bug (run `compare-legacy-dream3d`).

> **Deviation ID:** `BadDataNeighborOrientationCheck-D2` *(speculative)*
> **Filter UUID:** `3f342977-aea1-49e1-a9c2-f73760eba0d3`
> **Symptom:** Per-Laue-group misorientation result differs between SIMPLNX (EbsdLib 2.0.0) and SIMPL 6.5.172 (older EbsdLib).
> **Root cause:** EbsdLib 2.0.0 API change in PR #1472 may have come with numerical or representation-level changes in `LaueOps::calculateMisorientation`.
> **Affected users:** Any user comparing per-voxel mask outputs near the misorientation tolerance boundary across versions.
> **Recommendation:** Run `compare-legacy-dream3d` with a tight tolerance (e.g., misorientation_tolerance = 5.0 with quaternion pairs whose true misorientation is in the 4.95–5.05 deg band) to detect any EbsdLib drift. If detected, document and trust SIMPLNX (newer library).
> **Status:** Speculative — pending verification.

> **Deviation ID:** `BadDataNeighborOrientationCheck-D3` *(potential, behavior-of-record)*
> **Filter UUID:** `3f342977-aea1-49e1-a9c2-f73760eba0d3`
> **Symptom (potential):** The algorithm exhibits **iteration-order dependence**. The promotion of bad voxels to good happens in raster order, and once a voxel is promoted in iteration *k*, its (still-bad) neighbors get their `neighborCount` incremented immediately, which can promote them in the **same** outer iteration. This means the final mask depends on the linear scan order of the voxels, not just on the input data and parameters.
> **Why flagged:** This is documented behavior (the doc explicitly says "This can lead to a Flood Fill type of algorithm" and warns the user) but it is the kind of property that should be explicitly captured as an algorithm characteristic, not a deviation. If the legacy algorithm has the same order-dependence, this is a *Property*, not a *Deviation*.
> **Recommendation:** Confirm the legacy SIMPL algorithm exhibits the identical raster-order-dependent flood fill. If it does, demote D3 from the deviation list and capture it instead in the V&V notes as an "Algorithm Characteristic" (with a note that running on a permuted DataStructure layout would *not* produce a permuted output).
> **Status:** Proposed for review — promote to Deviation only if legacy and SIMPLNX disagree on order semantics; otherwise document as a property.

> **Deviation ID:** `BadDataNeighborOrientationCheck-D4` *(potential, mixed-phase neighbors)*
> **Filter UUID:** `3f342977-aea1-49e1-a9c2-f73760eba0d3`
> **Symptom (potential):** The algorithm requires **same-phase neighbors** (`cellPhases[voxelIndex] == cellPhases[neighborPoint]`) to count toward the threshold, AND requires the cell's own phase to be `> 0`. A bad voxel sitting at a phase boundary can therefore never be promoted, regardless of how many same-phase good neighbors are nearby on the other side of the boundary.
> **Why flagged:** This is documented implicitly but is the kind of behavior users should expect to be captured in the V&V invariant set. Verify this matches legacy.
> **Recommendation:** Compare against legacy on a fixture with a deliberate phase boundary and a bad-voxel at the boundary. If both agree, document as an invariant ("voxels with phase==0 are never promoted; voxels are only promoted via same-phase neighbors"). If they disagree, promote to a real Deviation.
> **Status:** Proposed for review.
