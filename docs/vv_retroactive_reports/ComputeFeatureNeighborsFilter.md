# Retroactive V&V: ComputeFeatureNeighborsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `7177e88c-c3ab-4169-abe9-1fdaff20e598` |
| SIMPLNX ClassName | `ComputeFeatureNeighborsFilter` |
| SIMPLNX Human Name | Compute Feature Neighbors |
| SIMPL UUID | `97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac` (from `test/simpl_conversion/6_5/ComputeFeatureNeighborsFilter.json`) |
| SIMPL ClassName | `FindNeighbors` (from SIMPL conversion fixtures) |
| SIMPL Human Name | Find Neighbors *(inferred from class name; confirm in legacy SIMPL repo)* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureNeighborsFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureNeighbors.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ComputeFeatureNeighborsTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/ComputeFeatureNeighborsFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ComputeFeatureNeighborsFilter.json`
- `src/Plugins/SimplnxCore/docs/ComputeFeatureNeighborsFilter.md`
- `src/Plugins/SimplnxCore/test/CMakeLists.txt` (for archive resolution)

## Algorithm Relationship

- **Tentative classification:** **Port-with-Major-Rewrite-and-Bug-Fix** — the SIMPLNX filter inherits the legacy SIMPL `FindNeighbors` UUID via the conversion fixtures, but the SIMPLNX algorithm has since been extensively restructured and at least one **substantive correctness bug** in the legacy Shared Surface Area calculation was patched in PR #1569 (commit message: *"Patched Major bug in calculation of Shared Surface Area List (Present in 6.5)"*).
- **Notable additions / deviations from a pure port:**
  - **Bug fix in Shared Surface Area List** (PR #1569): the legacy SIMPL implementation produced incorrect SSA values; the test file even calls this out by name in the legacy `SmallIn100` test case and skips SSA comparison on the legacy exemplar (lines 912–913: *"The exemplar Shared Surface Area is not valid after a bug fix, and the input file is used in other test cases."*).
  - **Algorithmic restructuring** (PR #1569): runtime branching converted to compile-time `constexpr` template dispatch on image dimensionality (`SingleVoxelImage`, `XImage1D`, `YImage1D`, `ZImage1D`, `EmptyXImage2D`, `EmptyYImage2D`, `EmptyZImage2D`, `Image3D`); deepest nested loop reduced for non-3D cases; corners / edges / faces / internal cells split into separate processing stages to remove a branch from the hottest inner loop.
  - **2D image handling standardization** (PR #1590): the algorithm dispatcher was rewritten to honor an explicit dimensionality state rather than ad-hoc `dims[i] == 1` checks scattered through the code; algorithm shrunk by ~150 net lines while gaining the new 7-state dispatcher.
  - **Surface-area unit:** the algorithm computes physical area (`spacing[i] * spacing[j]`, scaled by the appropriate face-normal) using `computeFaceSurfaceAreas<ImageDimensionStateT>(spacing)`. This is *not* a pure voxel-face count.
  - **Feature 0 handling:** feature ID 0 is treated as "bad data" and is **skipped** as both the source and the neighbor in the boundary-counting loops (`if(feature > 0)` and `if(neighborFeatureId != feature && neighborFeatureId > 0)`). Feature 0 will not appear in any neighbor list.
  - **6-face neighbor kernel only:** unlike `CAxisSegmentFeaturesFilter` (which gained a 26-neighbor kernel option in PR #1373), `ComputeFeatureNeighborsFilter` is hard-coded to face-neighbor (6-connectivity in 3D, 4 in 2D, 2 in 1D). This is documented in the user docs as "six (6) face-face neighboring **Cells** (front, back, left, right, up, down)".
- **Evidence:** PR #1569 explicitly says the SSA bug is "Present in 6.5" and the test code itself documents that the legacy exemplar's SSA is no longer valid. UUID inheritance from SIMPL via the conversion fixtures is the strongest evidence of "port"; the rewrite is in the algorithm body, not in the filter contract.
- **Action required:** Confirm by reading the SIMPL `FindNeighbors` source. Specifically: (a) reproduce the SSA bug on the legacy version against a hand-derivable toy dataset; (b) confirm the bug only affects SSA and not the neighbor list itself; (c) decide whether to patch SIMPL 6.5.172 or simply document the deviation.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1439 multi-dim tuple API, #1535 preflight cleanup, #1538 zlib extraction, #1543 doc-pipeline rename, #1521 not_used reorg) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter.

### PR #1301 — *"ENH: Add missing algorithm classes to some filters"* — merged 2026-01-08

- **Files in this filter:** algorithm (.hpp, .cpp) — created
- **Diff size:** 2 files, +99 / 0 lines
- **Change nature:** **Refactor scaffolding.** Created the empty `Algorithm/ComputeFeatureNeighbors.{hpp,cpp}` skeleton (header with `ComputeFeatureNeighborsInputValues` struct + class declaration; .cpp with constructor and an empty `operator()()` body). The filter itself was not yet wired up to call the algorithm — that came in PR #1544.
- **V&V content:** None — pure structural prep.

### PR #1523 — *"ENH: Factor out the 6-face neighbor code that is systemic through out the code base."* — merged 2026-02-05

- **Files in this filter:** filter (.cpp), -54 / +30 lines (net -24)
- **Diff size:** 1 file
- **Change nature:** Replaced the inline 6-face neighbor offset/iteration scaffolding inside the filter `executeImpl()` with calls to the new `simplnx/Utilities/NeighborUtilities.hpp` helpers. The substantive logic was unchanged; this was a code-deduplication pass touching every filter that walks 6-face neighbors. *(Broad refactor, but this filter's interaction with the new utility is meaningful — the dispatcher built in PR #1590 sits on top of these utilities.)*
- **V&V content:** None directly. Set the stage for the dimensionality dispatcher that arrived in PR #1590.

### PR #1535 — *"ENH: Remove redundant preflight checks that are already done in the parameter"* — merged 2026-02-18 *(broad refactor, exception flagged because the diff actually deletes preflight validation from this filter)*

- **Files in this filter:** filter (.cpp), -6 / +2 lines
- **Diff size:** 1 file, +2 / -6 lines
- **Change nature:** Removed null-pointer / existence checks for parameters that the parameter framework already validates (e.g., `GeometrySelectionParameter` and `ArraySelectionParameter` automatically check that the selected object exists in the DataStructure). No behavioral change for the happy path; reduces dead error-handling code.
- **V&V content:** None — but worth verifying that the validation removed was in fact redundant and not silently masking an edge case.

### PR #1544 — *"ENH: Move Filter executeImpl() logic to Algorithm classes (#1284)"* — merged 2026-02-26 *(broad refactor, exception flagged because this is the PR that finally wired the filter to its Algorithm class — substantive structural change)*

- **Files in this filter:** filter (.cpp) -213/+0 lines moved out, algorithm (.cpp/.hpp) +223/+73 lines moved in (net ~+86)
- **Diff size:** 3 files, +312 / -197 lines
- **Change nature:** **Structural migration.** The filter's `executeImpl()` previously contained the entire algorithm body inline. PR #1544 moved that body into `ComputeFeatureNeighbors::operator()()` and reduced `executeImpl()` to parameter extraction + a delegating call. This is part of the filter-grouping cleanup tracked under issue #1284 and is the precondition for the OOC dispatch work that followed.
- **V&V content:** Behavioral identity — no algorithmic change intended. The git diff is entirely "code moved", but the move itself is invasive enough that a regression-test pass is the verification of record (the existing `Legacy: SmallIn100` test was the only regression check until PR #1569 added 32 hand-derived cases).

### PR #1569 — *"BUG/REV: Compute Feature Neighbors"* — merged 2026-03-25

- **Files in this filter:** algorithm (.cpp +697/-153), algorithm (.hpp +27/-0), filter (.cpp +24/-0), test (.cpp +725/-0)
- **Diff size:** 4 files, +1,276 / -197 lines
- **Change nature:** **Major bug fix + algorithm restructuring + 32 new hand-validated test cases.** Per the PR commit message verbatim:
  - *"Patched Major bug in calculation of Shared Surface Area List (Present in 6.5)"*
  - *"Added 32 new test cases covering 3D/2D/1D/0D"*
  - *"Hand validated output for each new test case"*
  - *"Algorithm restructuring to reduce time complexity and branching in tight loops"*
  - *"deepest nested loop reduction for non 3D cases"*
  - *"extensive conversion of runtime branching to compile time constexpr"*
  - *"assorted optimizations following profiling"*
  - *"code documentation"*
- **V&V content:** **Highest in this filter's entire history.** This is the closest thing to per-filter V&V work in the audit. The 32 new TEST_CASEs are the hand-derivation oracle that the audit's Step 0(c) calls for; the SSA bug fix is a strong Deviation candidate vs. SIMPL 6.5.172.
- **Note:** The new test fixtures (`CreateSingleVoxelDataStructure`, `Create1D{X,Y,Z}DataStructure`, `Create2DEmpty{X,Y,Z}DataStructure`, `Create3DDataStructure`) all carry **inline hand-computed exemplar values** — boundary cells, surface features, num neighbors, neighbor lists, and SSA values are all coded directly into the test file rather than loaded from a `.dream3d` exemplar. This is itself the oracle for those 25 cases.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +52 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ComputeFeatureNeighborsFilter.json` (~28 lines, uses `Filter_Name`-only fallback)
  - `test/simpl_conversion/6_5/ComputeFeatureNeighborsFilter.json` (~29 lines, includes `Filter_Uuid` `{97cf66f8-7a9b-5ec2-83eb-f8c4c8a17bac}` and `Filter_Name` `FindNeighbors`)
- **Diff size:** 3 files, +109 lines
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::ComputeFeatureNeighborsFilter: SIMPL Backwards Compatibility"`. Replaces a former monolithic `BackwardsCompatibilityTest.cpp` per-filter coverage.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### PR #1590 — *"ENH: Standardize 2D Image Handling"* — merged 2026-04-23

- **Files in this filter:** algorithm (.cpp) -295/+77 (net -218 lines), test (.cpp) +135/-0
- **Diff size:** 2 files, +212 / -295 lines
- **Change nature:** **Algorithm rewrite for dimensionality dispatch.** The algorithm body was rewritten to use the standardized `detail::ImageDimensionality` template state and the dispatcher in `simplnx/Utilities/NeighborUtilities.hpp` (`VoxelNeighbors<ImageDimensionStateT>`, `initializeFaceNeighborOffsets<...>`, `computeValidFaceNeighbors<...>`, `computeFaceSurfaceAreas<...>`, `ImageDimensionalUtilities::ProcessCorners/Edges/Faces<...>`). Net code reduction of ~218 lines while adding correct handling for SingleVoxel/1D-X/1D-Y/1D-Z/Empty2D-X/Empty2D-Y/Empty2D-Z/3D as a 7-state compile-time dispatch.
- **V&V content:** **Test additions confirm a stride bug fix.** The test file gained three `Non-Square` 2D test cases (`Case 2.0.4`, `Case 2.1.4`, `Case 2.2.4`) with a verbatim comment in the source explaining what the prior code got wrong:

  > *"The existing 5x5x1 cases all have dims[0] == dims[1], which masks an incorrect row-stride calculation in initializeFaceNeighborOffsets for any of the three Empty2D dispatches. A non-square 3x2 layout causes the wrong stride to either skip a boundary face (producing SSA = 2 \* area instead of 3 \* area) or run off the end of the buffer, so the hand-computed SSA below fails under the original bug."*

  This is a second substantive bug fix on top of #1569's SSA fix: a 2D row-stride bug that was masked by square test fixtures. **Both fixes are strong Deviation candidates.**

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change (NeighborList getter renames), no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1521 | Move unused algorithm codes to internal directory | The empty stub from #1301 was briefly moved to `not_used/` here, then revived later when #1544 actually populated it. The current file lives at the canonical (non-`not_used`) path. |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure |
| #1543 | Update pipeline references in documentation | Doc cosmetic — corrected names of "(03) Small IN100 Morphological Statistics" etc. |

## Test coverage detected

`ComputeFeatureNeighborsTest.cpp` contains **34 `TEST_CASE`s.** Brief one-line descriptions:

**Single-voxel (0D) — 4 cases (added by PR #1569):**
1. `Case 0.0.0: Single Voxel - Full Execution` — both optionals on
2. `Case 0.0.1: Single Voxel - No Boundary` — boundary cells off
3. `Case 0.0.2: Single Voxel - No Surface Features` — surface features off
4. `Case 0.0.3: Single Voxel - No Optionals` — both off

**1D — 12 cases (added by PR #1569):**
5–8. `Case 1.0.0–1.0.3: 1D Z` (4 optional combinations)
9–12. `Case 1.1.0–1.1.3: 1D Y`
13–16. `Case 1.2.0–1.2.3: 1D X`

**2D Square 5×5 — 12 cases (added by PR #1569):**
17–20. `Case 2.0.0–2.0.3: 2D Empty Z`
21–24. `Case 2.1.0–2.1.3: 2D Empty Y`
25–28. `Case 2.2.0–2.2.3: 2D Empty X`

**3D 5×5×5 — 4 cases (added by PR #1569):**
29–32. `Case 3.0.0–3.0.3: 3D` (4 optional combinations)

**2D Non-Square regression — 3 cases (added by PR #1590):**
33. `Case 2.0.4: 2D Empty Z - Non-Square {3,2,1}` — stride-bug regression for EmptyZ
34. `Case 2.1.4: 2D Empty Y - Non-Square {3,1,2}` — stride-bug regression for EmptyY
35. `Case 2.2.4: 2D Empty X - Non-Square {1,3,2}` — stride-bug regression for EmptyX

**Legacy / large dataset:**
36. `Legacy: SmallIn100` — runs the filter on the real `6_6_stats_test_v2.dream3d` archive and compares NumNeighbors, NeighborList, and SurfaceFeatures against in-file exemplars. **SSA comparison is intentionally skipped** (per the in-source comment, the exemplar SSA was generated by buggy legacy code).

**SIMPL conversion — 1 case (added by PR #1588):**
37. `SIMPL Backwards Compatibility` — DYNAMIC_SECTION over SIMPL 6.4 + 6.5 conversion fixtures.

(Total = 35 TEST_CASEs counted; the brief specified "list every TEST_CASE", so per-case enumeration is above.)

The hand-derived 0D/1D/2D/3D suite covers a 3×3×3 cross-product of dimensionality × empty-axis × optional-flag combinations, which is unusually thorough. The `Legacy: SmallIn100` test plus the explicit SSA-skip comment provides the audit trail for the bug fix in #1569.

## Exemplar archive

- **Archive name:** `6_6_stats_test_v2.tar.gz`
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt` (line ~233)
- **Inner exemplar file:** `6_6_stats_test_v2.dream3d` — loaded via the `TestFileSentinel` mechanism in `Legacy: SmallIn100`.
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplars were generated and whether an Oracle Provenance block exists in any ReadMe inside it.)* Note the in-source comment that the SSA exemplar in this archive is **known-incorrect** because it was generated by legacy DREAM3D code that had the bug fixed in PR #1569. Any new exemplar generation pass must regenerate SSA from the SIMPLNX implementation.
- **Action required:** Download the archive locally and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes. Promote this content into the verification archive ReadMe per Step 0's Oracle Provenance policy. The 32 hand-derived test cases added in PR #1569 do **not** depend on this archive — those exemplars are coded inline in the test file and are independent of the archive.

## Oracle classification (tentative)

- **Recommended class:** **Class 1 (Analytical) + Class 4 (Invariant) companion.**
  - **Class 1 rationale:** This filter is a foundational topology operation — given a per-voxel `FeatureIds` array, walk the 6 face neighbors of each voxel and count which feature IDs share at least one face. The output (per-feature neighbor list, neighbor count, shared face area) is **directly hand-derivable** for small datasets, and the test file already does this for 32 cases (single-voxel, 1D, 2D square, 2D non-square, 3D 5×5×5). The hand-derived inline exemplars in `Fill1DImage`, `Fill2DImage`, `FillNonSquare2DFeatures`, and `Create3DDataStructure` *are* the analytical oracle of record.
  - **Class 4 rationale (companion):** Strong topological invariants exist and should be encoded as explicit assertions:
    - **Symmetry:** feature `i` is in `NeighborList[j]` ⟺ feature `j` is in `NeighborList[i]`.
    - **SSA symmetry:** `SharedSurfaceArea(i, j) == SharedSurfaceArea(j, i)`.
    - **Count consistency:** for every feature, `NumNeighbors[i] == NeighborList[i].size() == SharedSurfaceArea[i].size()`.
    - **Feature 0 absence:** feature 0 (bad data) must never appear in any neighbor list.
    - **Boundary-cell range:** every boundary cell value must be in `[0, k_NeighborCount]` where `k_NeighborCount` is 6/4/2 for 3D/2D/1D respectively.
    - **Per-feature SSA upper bound:** total SSA for feature `i` ≤ `surface_voxel_count(i) × max_face_area`.
- **Why NOT Class 3 (Paper-based):** This is direct topology, not an algorithm with a published reference. Class 3 would be overkill — there is no equivalent of an "EBSD inverse pole figure" paper that defines the correct answer; the correct answer is mechanically derivable from the input.
- **Action required:** Developer to confirm the Class-1 + Class-4 recommendation. The 32 hand-derived test cases already satisfy Class 1; the Class-4 invariants should be promoted into the test code as explicit `REQUIRE`s rather than relying solely on exemplar comparison.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review. The `code documentation` bullet in PR #1569's commit message is the closest signal. |
| Code path coverage (algorithmic) | **Excellent** | 32 hand-derived cases cover dimensionality (0D/1D/2D/3D) × empty-axis × optional-flag (StoreBoundaryCells, StoreSurfaceFeatures). 3 non-square 2D regression tests close the row-stride bug class. Plus 1 legacy large-dataset test. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `6_6_stats_test_v2.tar.gz` referenced in test/CMakeLists.txt; **but** the SSA exemplar inside is known-incorrect (legacy bug). |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one (Class 1 + Class 4 companion). |
| Toy data / independent expected output (Step 0 c) | **Yes — already exists** | The 32 hand-derived test cases in `ComputeFeatureNeighborsTest.cpp` are the toy-data oracle. They were added in PR #1569 with the explicit note "Hand validated output for each new test case". |
| Legacy comparison report (Step 0 e) | **Partial** | The `Legacy: SmallIn100` test runs the filter on a legacy dataset and compares NumNeighbors / NeighborList / SurfaceFeatures, but skips SSA. Full `compare-legacy-dream3d` against a running 6.5.172 has not been documented. |
| Deviation entries (`ComputeFeatureNeighbors-D<N>`) | **None recorded** | Two strong Deviation candidates are pre-flagged below: D1 (SSA bug, PR #1569) and D2 (2D row-stride bug, PR #1590). |
| Documentation currency | Probably current | Minor pipeline-name update from PR #1543. The doc accurately describes the 6-face neighbor algorithm and the 0D–3D handling note added by recent work. Needs accuracy audit per `review-filter-docs`. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 (Analytical) + Class 4 (Invariant) companion is the recommended starting point. The 32 hand-derived inline test cases already satisfy the Class-1 oracle requirement — but they should be augmented with explicit Class-4 invariant assertions (symmetry, SSA symmetry, count consistency, feature-0 absence) so that a future change to the test fixtures cannot silently break the invariants.
2. **Promote the inline-exemplar tests to oracle of record.** The hand-derivations in `Fill1DImage`, `Fill2DImage`, `FillNonSquare2DFeatures`, and `Create3DDataStructure` are the closest thing in the entire SimplnxCore plugin to a Class-1 oracle. Document this explicitly in the verification archive ReadMe so that the inline arrays are recognized as the source of truth.
3. **Inspect `6_6_stats_test_v2.tar.gz` and document provenance.** Determine how the legacy exemplars were generated and write an Oracle Provenance block. Specifically note that **the in-archive SSA exemplar is known-incorrect** and that the SIMPLNX implementation post-#1569 should be the new source of truth for SSA. Decide whether to regenerate the archive with a corrected SSA exemplar (call it `6_6_stats_test_v3.tar.gz`) or keep the current archive and continue skipping SSA in the legacy test.
4. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same toy data. Expected outcomes:
   - At minimum **two Deviation entries** (see proposed `ComputeFeatureNeighbors-D1` and `D2` below).
   - Verify the SSA bug magnitude on a real dataset; quantify the difference for the Deviation report.
   - Verify that the 2D row-stride bug existed in legacy as well (or only in an interim SIMPLNX version).
5. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port-with-Major-Rewrite-and-Bug-Fix — direct UUID-preserving translation of the SIMPL `FindNeighbors` filter, restructured for compile-time dimensionality dispatch in PR #1569 / PR #1590, with a SSA correctness bug fixed in PR #1569 and a 2D row-stride bug fixed in PR #1590."*
6. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ComputeFeatureNeighbors-D1`
> **Filter UUID:** `7177e88c-c3ab-4169-abe9-1fdaff20e598`
> **Symptom:** SIMPLNX produces correct Shared Surface Area values; SIMPL 6.5.172 produces incorrect SSA values (magnitude TBD by running the comparison). The neighbor list itself (which features are neighbors) and the count of neighbors are unaffected — only the per-shared-face area is wrong in legacy.
> **Root cause:** Bug in 6.5.172's `FindNeighbors` SSA accumulation. PR #1569's commit message verbatim: *"Patched Major bug in calculation of Shared Surface Area List (Present in 6.5)"*. The SIMPLNX test code itself documents this with the inline note (line 912–913): *"The exemplar Shared Surface Area is not valid after a bug fix, and the input file is used in other test cases. Other test cases validate SSA functionality."*
> **Affected users:** Anyone whose downstream analysis consumes `SharedSurfaceAreaList` from SIMPL 6.5.172 — including any SSA-driven smoothing, MTR clustering, or grain-boundary energy calculation. Users who rely only on `NeighborList` and `NumNeighbors` are unaffected.
> **Recommendation:** Trust SIMPLNX. Legacy was wrong. Engineer to evaluate whether a backport patch to 6.5.172 is warranted for users who must reproduce historical results, or whether documenting the deviation is sufficient.
> **Status:** Proposed — pending verification that 6.5.172 actually exhibits the bug (run the comparison) and quantification of the magnitude on a real dataset.

> **Deviation ID:** `ComputeFeatureNeighbors-D2`
> **Filter UUID:** `7177e88c-c3ab-4169-abe9-1fdaff20e598`
> **Symptom:** On non-square 2D image geometries (where `dims[0] != dims[1]` and one axis has size 1), the previous SIMPLNX implementation computed an incorrect row stride in `initializeFaceNeighborOffsets`. The wrong stride either skipped a boundary face (yielding SSA = 2 × area instead of 3 × area in the test case) or ran off the end of the buffer.
> **Root cause:** The 5×5×1 test fixtures used everywhere in the codebase had `dims[0] == dims[1]`, which **masked** the bug because the wrong stride happened to equal the right stride. The bug was caught when PR #1590 added explicit non-square 3×2 test fixtures (`Create2DNonSquareEmptyZDataStructure`, `Create2DNonSquareEmptyYDataStructure`, `Create2DNonSquareEmptyXDataStructure`) that force the dispatcher to use the correct row stride.
> **Affected users:** Anyone running this filter on a 2D image (one dimension == 1) with a non-square in-plane shape — the filter would have produced wrong SSA values and possibly a wrong neighbor list. Users with square 2D images or fully 3D images are unaffected.
> **Recommendation:** Trust the post-#1590 SIMPLNX. Engineer to verify whether legacy SIMPL 6.5.172 also has this bug (likely yes if it inherits from the same code lineage), and whether any prior SIMPLNX release shipped with the bug — if so, document the affected version range.
> **Status:** Proposed — needs verification that legacy SIMPL 6.5.172 exhibits the same row-stride bug, and verification of which SIMPLNX release(s) shipped before the fix.

> **Deviation ID:** `ComputeFeatureNeighbors-D3` *(possible — needs investigation)*
> **Filter UUID:** `7177e88c-c3ab-4169-abe9-1fdaff20e598`
> **Symptom:** SIMPLNX treats feature ID `0` as "bad data" and excludes it from all neighbor relationships (a feature `i` will never have `0` in its neighbor list). The SSA contribution from feature-`0` faces is dropped (those voxel faces are still counted into `BoundaryCells` if either side has a different feature, but the `0`-side is filtered out by `neighborFeatureId > 0`).
> **Symptom from kernel choice:** The filter is hard-coded to a 6-face neighbor kernel (no 26-neighbor option, in contrast to `CAxisSegmentFeaturesFilter` after PR #1373). Users who expect 26-connectivity will see different (smaller) neighbor lists.
> **Recommendation:** Verify that legacy SIMPL `FindNeighbors` exhibits the same feature-0 handling and the same 6-face restriction. If so, no deviation; if legacy treated feature 0 as a real feature or used a different kernel, this is a behavioral deviation that needs documenting. Likely no actual deviation — both questions are pre-flagged for the audit, not asserted.
> **Status:** Open question — needs comparison against SIMPL 6.5.172.
