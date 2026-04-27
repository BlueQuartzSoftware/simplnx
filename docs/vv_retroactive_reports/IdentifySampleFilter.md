# Retroactive V&V: IdentifySampleFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `94d47495-5a89-4c7f-a0ee-5ff20e6bd273` |
| SIMPLNX ClassName | `IdentifySampleFilter` |
| SIMPLNX Human Name | Isolate Largest Feature (Identify Sample) |
| SIMPL UUID | `0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a` |
| SIMPL ClassName | `IdentifySample` |
| SIMPL Human Name | Identify Sample |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/IdentifySampleFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/IdentifySample.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/IdentifySampleTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/IdentifySampleFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/IdentifySampleFilter.json`
- `src/Plugins/SimplnxCore/docs/IdentifySampleFilter.md`
- `src/Plugins/SimplnxCore/test/CMakeLists.txt` (data archive references)

## Algorithm Relationship

- **Tentative classification:** **Port + Minor enhancements + Bug fixes.** The SIMPLNX filter began as a direct port of the SIMPL `IdentifySample` filter (UUID `0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a`) and a new SIMPLNX UUID was assigned (`94d47495-5a89-4c7f-a0ee-5ff20e6bd273`). Since 2025-10-01 it has accumulated:
  1. A new **Slice-By-Slice** mode (XY/XZ/YZ) added before the audit window with two follow-up bug fixes during the window (PR #1562).
  2. **2D image-handling generalization** (PR #1590) — replaced the original "treat as 3D" code path with a dimensionality-aware dispatch (`Image3D`, `EmptyXImage2D`, `EmptyYImage2D`, `EmptyZImage2D`, `XImage1D`, `YImage1D`, `ZImage1D`, `SingleVoxelImage`) using the project-wide `NeighborUtilities` helpers, plus an optimization of the fill-holes pass.
  3. Refactor that hoisted the algorithm body into an `IdentifySample` Algorithm class (PR #1301, then a major rewrite/cleanup in PR #1530 that also added cancel/progress).
  4. Cancel + progress reporting (PR #1473 then PR #1530).
- **Net effect vs. SIMPL:** The slice-by-slice path is **net-new** in SIMPLNX (no SIMPL counterpart). The 2D dispatch + dimensionality-aware neighbor offsets are **net-new** in SIMPLNX. The whole-volume 6-face flood-fill core remains a port of the SIMPL algorithm.
- **Action required:** Developer to confirm whether the SIMPL 6.5.172 filter has a slice-by-slice mode (it almost certainly does not) and confirm that the whole-volume code path is still bit-for-bit comparable to legacy when both run on the same input.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline, #1439 multi-dim tuple API) and pure test-infrastructure / doc-touchup PRs (#1538 zlib extraction, #1543 pipeline-reference doc cleanup) are listed at the bottom of this section but not detailed individually — they did not change behavior of this filter. PR #1301 ("Move execution to algorithm classes") is in the broad-refactor pruned list per policy but is **mentioned briefly** here because it created the Algorithm class shell that all later changes depend on.

### PR #1473 — *"ENH: Isolate Largest Feature now checks for cancel button and sends progress"* — merged 2025-11-24

- **Files in this filter:** filter (.cpp) only
- **Diff size:** 1 file, +29 / -5 lines
- **Change nature:** **Feature addition.** Added `shouldCancel` checks and progress messaging to the (then still in-filter) algorithm. No exemplar data changes; no test changes.
- **V&V content:** None directly — purely cooperative-cancel and user-feedback plumbing. No new code paths exercised.

### PR #1301 — *"ENH: Move Execution to Algorithm Classes"* — merged 2026-01-08 (broad refactor, exception flagged)

- **Files in this filter:** algorithm (.hpp, .cpp) **created**, filter (.cpp) modified
- **Diff size:** 3 files, +497 / -0 lines
- **Change nature:** **Refactor specific to this filter.** Created the `IdentifySample` Algorithm class skeleton and moved the existing executeImpl() body into it. Listed in the user's pruned-PR table as a broad sweep; included here only because for *this* filter it is the change that established the Algorithm-class layout. No behavior change intended.
- **V&V content:** None — pure code reorganization. Closes part of issue #1284.

### PR #1530 — *"ENH: IdentifySample - use algorithm class and add cancel and message feedback"* — merged 2026-02-18

- **Files in this filter:** filter (.cpp) -445 lines, algorithm (.cpp) +176/-176, test (.cpp) +121/-121, docs (.md) +40/-40
- **Diff size:** 4 files, +172 / -628 lines
- **Change nature:** **Major restructuring + feature addition.** PR description: *"Moved actual algorithm code to the Algorithm class. Added 'Slice-by-slice execution' test with 3 SECTIONs (XY, XZ, YZ planes) using synthetic 5x5x3 geometry. Added 'UInt8 mask type' test using a 5x5x5 geometry with UInt8Array, testing both sample identification and hole filling."*
- **V&V content:** **High** — this is the PR that introduced the slice-by-slice algorithm path **and** the unit tests for it. It also widened the mask type from `bool` only to also support `uint8`. The 463→ small filter file size delta confirms full extraction to the Algorithm class. The test count expansion confirms new code paths (slice-by-slice, uint8) are at least covered by smoke tests.

### PR #1562 — *"BUG: Fix two bugs in IdentifySampleSliceBySliceFunctor"* — merged 2026-03-10

- **Files in this filter:** algorithm (.cpp) +6/-3, test (.cpp) +47/-17
- **Diff size:** 2 files, +36 / -17 lines
- **Change nature:** **Material bug fix + verification.** Two distinct bugs in the slice-by-slice fill-holes pass:
  1. *Wrong global index when filling holes:* the local plane index was being passed directly to `setValue` instead of being decomposed into (p1, p2) and re-projected with the correct strides. **Wrong results for XZ and YZ planes.**
  2. *`checked` vector not reset between phases:* voxels visited during sample-identification were skipped during the subsequent fill-holes pass, leaving holes inside the sample unfilled.
- **V&V content:** **High.** PR description and diff confirm test data was upgraded to **`identify_sample_v2.tar.gz`** with the **full 12-combination cross product** (fill/nofill × whole/sliced × XY/XZ/YZ). This is precisely the kind of regression-locking change a V&V record should capture. The bugs were introduced in PR #1530, so they did not affect SIMPL-comparable behavior; they were SIMPLNX-only regressions in a SIMPLNX-only feature.

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md) +8 lines
- **Change nature:** Documentation hygiene — added the standardized `### Slice-By-Slice Plane` subsection enumerating `XY [0]`, `XZ [1]`, `YZ [2]` with brief descriptions.
- **V&V content:** Doc currency improvement. Not algorithmic.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +46 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/IdentifySampleFilter.json` (6.4 — Filter_Name fallback path)
  - `test/simpl_conversion/6_5/IdentifySampleFilter.json` (6.5 — UUID-mapped path)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both the SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::IdentifySampleFilter: SIMPL Backwards Compatibility"`. The 6.5 fixture confirms the legacy SIMPL UUID `0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a` and the legacy parameter keys `FillHoles` + `GoodVoxelsArrayPath`.
- **V&V content:** **Pipeline-conversion correctness only** — verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the correct parameter values (FillHoles=true, geometry path, mask path). It does **not** verify that the filter's *output* matches legacy. That latter step is still missing.

### PR #1590 — *"ENH: Standardize 2D Image Handling"* — merged 2026-04-24

- **Files in this filter:** algorithm (.cpp) +247/-98 lines (rewrite of the whole-volume code path), test (.cpp) +114 lines (the three new "2D Empty {X,Y,Z} Non-Square {3,4,1}" regression tests)
- **Diff size:** 2 files, +263 / -98 lines
- **Change nature:** **Major algorithmic restructuring + targeted bug fix + optimization.** PR description (relevant bullets):
  - *"Patch bug in 2D Identify Sample Handling"* — a 2D fixture exposed an incorrect row stride / neighbor-index calculation when one of the three image dimensions was 1.
  - *"Optimize Identify Sample Fill Hole Algorithm"* — fewer allocations / better stride math in the fill-holes pass.
  - The whole-volume `IdentifySampleFunctor` was templated over `ImageDimensionality` and now dispatches to one of `Image3D`, `EmptyZImage2D`, `EmptyYImage2D`, `EmptyXImage2D`, `ZImage1D`, `YImage1D`, `XImage1D`, `SingleVoxelImage` via the new `ProcessVoxels<>` helper, using `initializeFaceNeighborOffsets<>()` and `computeValidFaceNeighbors<>()` from `NeighborUtilities`.
- **V&V content:** **High.** Three new hand-built non-square 2D regression tests were added (one per "empty axis"), each constructing a 3×4 mask with a 4-voxel and a 2-voxel connected component and asserting that only the larger survives. These are textbook **Class 1 (analytical)** assertions — the expected output is hand-derived and inlined into the test as a `std::array<bool, 12>`. This is the strongest piece of "oracle of record" code currently in the test suite for this filter. The PR also touches `ComputeFeatureSizes` and `ImageGeom::findElementSizes` and adds `NeighborUtilities` pure-function tests, which are out of scope for this report but corroborate that the 2D-handling change was systematic.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, single-line filter touch (header include or signature), no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib in unit tests | Test infrastructure (one-line constructor change) |
| #1543 | Update pipeline references in documentation files | Doc cleanup, removed one stale pipeline reference |

## Test coverage detected

`IdentifySampleTest.cpp` contains 4 `TEST_CASE`s:

1. **`SimplnxCore::IdentifySampleFilter`** — main exemplar-driven test. Iterates 12 parameter combinations (`{whole,sliced} × {xy,xz,yz} × {fill,nofill}`) using `SECTION` and compares the computed mask against an exemplar mask loaded from `identify_sample_v2/{slice_by_slice}_{plane}_{fill}.dream3d`. *(Updated by PR #1562 to use v2 data with the full 12-combo cross-product.)*
2. **`SimplnxCore::IdentifySampleFilter: SIMPL Backwards Compatibility`** — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION`. Conversion-only — does not run the algorithm. *(Added by PR #1588.)*
3. **`SimplnxCore::IdentifySampleFilter: 2D Empty Z Non-Square {3,4,1}`** — hand-built 3×4 mask, asserts the 4-voxel component survives and the 2-voxel component is cleared. *(Added by PR #1590.)*
4. **`SimplnxCore::IdentifySampleFilter: 2D Empty Y Non-Square {3,1,4}`** — same fixture re-shaped to exercise the EmptyY 2D dispatch. *(Added by PR #1590.)*
5. **`SimplnxCore::IdentifySampleFilter: 2D Empty X Non-Square {1,3,4}`** — same fixture re-shaped to exercise the EmptyX 2D dispatch. *(Added by PR #1590.)*

(Five `TEST_CASE`s total. Tests 3-5 are the strongest oracle-of-record coverage in the file; test 1 is exemplar-comparison; test 2 is conversion-only.)

## Exemplar archive

Two archives are referenced in `src/Plugins/SimplnxCore/test/CMakeLists.txt`:

- **`identify_sample.tar.gz`** (legacy / v1)
  - SHA512: `0c1da22d411ac739d3e90618141a6eab71705b47de6d4cc7501e9408bf6fcbadd46738f5e86a80ab2e0bc2344c23584cc2b89f2102c13073490e1817797ec9bc`
  - **NOT used by the current test** (the active test loads `identify_sample_v2/...`). Likely retained for older tests / pipelines that may still refer to it. **Action required:** confirm whether this older archive can be retired.
- **`identify_sample_v2.tar.gz`** (current — used by the main test)
  - SHA512: `a7ffac3eaad479c07215c1dd16274c45a52466708a9d27b5f85a29b0eba3b6705b627e1052a7a27e9bfe89cd6e7df673beb7a1e98b262b6c52ea383b4848ac31`
  - Contains 12 `.dream3d` files (one per parameter combination), each with a `Mask` array (input) and `Mask Exemplar` array (expected output) under the standard `DataContainer/CellData` path.
  - **Provenance:** *(TBD — engineer must inspect the archive to confirm how the exemplars were generated. Almost certainly produced by running an internal d3dpipeline that calls IdentifySample 12 times with the right argument combinations and writes the result with WriteDREAM3DFilter.)*

- **Action required:**
  1. Download `identify_sample_v2.tar.gz` and inspect for: an inner `ReadMe.md`, the input `.dream3d` files used to generate the exemplars, the pipeline files that produced the exemplars, and any provenance notes.
  2. Promote that content into the verification archive ReadMe per Step 0's Oracle Provenance policy.
  3. Decide the fate of the older `identify_sample.tar.gz`.

## Oracle classification (tentative)

- **Recommended class:** **4 (Invariant-based)** as the default, with **1 (Analytical)** already partially in place via PRs #1590's three hand-derived 2D fixtures.
  - **Class 4 invariants** (for any input mask `M` and output mask `M'`):
    - `M' ⊆ M` when `FillHoles=false` (a voxel is never *added* to the sample without fill-holes).
    - The set of "true" voxels in `M'` is connected under the chosen neighborhood (6-face for whole-volume; 4-neighbor for slice-by-slice).
    - The number of "true" voxels in `M'` equals the cardinality of the largest connected component of "true" voxels in `M`, when `FillHoles=false`.
    - **Idempotence:** running the filter on `M'` (with the same parameters) returns `M'` unchanged. This is a clean, easily-coded test.
  - **Class 1** is already partly satisfied: the three 2D non-square tests in `IdentifySampleTest.cpp` encode hand-derived expected output (`std::array<bool, 12> expected = {...}`) and are textbook Class-1 assertions. These should be expanded.
- **Class 3 (Paper-based) is NOT supported** by the current code: no citation, DOI, or named-algorithm reference (Hoshen–Kopelman, BFS-flood-fill, etc.) appears in `IdentifySample.{hpp,cpp}`. The implementation is a hand-rolled BFS flood-fill over a face-connected neighborhood — well-known but uncited. **Action required:** if the developer wants to promote to Class 3, add a citation in the Algorithm header (e.g., to a standard CC-labelling reference) and a one-paragraph "Algorithm" section in `docs/IdentifySampleFilter.md`.
- **Action required:** Developer to confirm Class 4 + (extended) Class 1 as the joint target classification, and to either add a citation to promote to Class 3 or accept that the algorithm is a textbook construction without a single canonical paper.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review; PR #1590 implicitly reviewed the dimension-handling code, and PR #1562 implicitly reviewed the slice-by-slice code. |
| Code path coverage (algorithmic, whole-volume) | Good | 6 SECTIONs in main test (`whole × {xy,xz,yz} × {fill,nofill}`) + 3 hand-built 2D regression tests covering EmptyX/Y/Z dispatches. |
| Code path coverage (algorithmic, slice-by-slice) | Good | 6 SECTIONs in main test (`sliced × {xy,xz,yz} × {fill,nofill}`). PR #1562 added these to lock in the bug fix. |
| Code path coverage (1D / single-voxel dispatch) | **Gap** | `ProcessVoxels<>` dispatches to `XImage1D`, `YImage1D`, `ZImage1D`, `SingleVoxelImage` for degenerate geometries; no test exercises these paths. |
| Code path coverage (uint8 mask) | **Partial** | PR #1530 description claims a uint8-mask test; current test code uses `BoolArray` exclusively. The exemplar comparison uses `CompareDataArrays<uint8>` which suggests the exemplar arrays may be uint8 — but the input arrays in all tests are bool. Worth confirming. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `identify_sample_v2.tar.gz` referenced in test/CMakeLists.txt; older `identify_sample.tar.gz` also still referenced (unclear consumer). |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | **Yes (partial)** | Three Class-1 hand-derived 2D tests added by PR #1590 are exactly this. They cover only the EmptyX/Y/Z 2D cases; no analogous Class-1 fixture exists for the 3D, 1D, or single-voxel dispatches, nor for the slice-by-slice mode (which is SIMPLNX-only and has no SIMPL counterpart to compare against). |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. The whole-volume + no-fill-holes path is the SIMPL-comparable subset. |
| Deviation entries (`IdentifySample-D<N>`) | None | Not yet written. PRs #1562 and #1590 fixed bugs that were *SIMPLNX-only* (introduced after the port), so the legacy comparison should still match for the SIMPL-comparable subset. The slice-by-slice mode and the 2D dispatch are net-new SIMPLNX features and warrant Deviation entries flagged as "added capability, no legacy equivalent." |
| Documentation currency | Probably current | Updated by PRs #1530 (slice-by-slice user-facing copy) and #1571 (ChoicesParameter description). Needs accuracy audit per `review-filter-docs`, and lacks a citation / "Algorithm" section. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **(Step 0a) Confirm the oracle.** Class 4 (invariant-based) joined with the existing Class-1 hand-derived 2D tests is the recommended starting point. Decide whether to add a citation and promote to Class 3.
2. **(Step 0b) Defend or rewrite the existing tests as oracle-class assertions.** The 12-combo exemplar test currently checks bytewise equality against `Mask Exemplar`; promote a subset to explicit Class-4 invariant assertions (`REQUIRE(count(M') == largestConnectedComponentSize(M))`, idempotence, monotonicity under repeated application). Add Class-1 fixtures for the 3D, 1D, single-voxel, and slice-by-slice dispatches.
3. **(Step 0c) Build a tiny analytic toy.** Extend the PR #1590 hand-fixture pattern (3×4 mask) to a 3×3×3 toy with a 6-voxel and a 3-voxel connected component for the 3D path. This becomes the third Class-1 record alongside the 2D fixtures and the existing exemplar comparison.
4. **(Step 0d) Inspect `identify_sample_v2.tar.gz` and document provenance.** Determine which pipeline produced the 12 exemplars, whether a SIMPL 6.5.172 pipeline was used to seed them, and what the input data was. Write an Oracle Provenance block for the archive ReadMe. Decide the fate of `identify_sample.tar.gz` (v1).
5. **(Step 0e) Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.172 on the same toy, with `SliceBySlice=false` and `FillHoles=false` (the only options shared with legacy). Expectation: bit-for-bit match. If a mismatch is found, file Deviation entry `IdentifySample-D1`.
6. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `IdentifySample` filter (UUID 0e8c0818-…). SIMPLNX-only additions: slice-by-slice mode (PR #1530, fixed by PR #1562), 2D-dimensionality-aware dispatch (PR #1590), uint8 mask support, cancel + progress (PRs #1473, #1530)."*
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `IdentifySample-D1` — *Added capability: Slice-By-Slice mode*
> **Filter UUID:** `94d47495-5a89-4c7f-a0ee-5ff20e6bd273`
> **Symptom:** SIMPLNX exposes a `SliceBySlice` (bool) parameter and a `SliceBySlicePlaneIndex` (XY/XZ/YZ choice) parameter. SIMPL 6.5.172 has no equivalent — only the whole-volume mode exists in legacy.
> **Root cause:** New feature added in SIMPLNX (PR #1530); subsequent two-bug fix in PR #1562. Not a behavioral deviation when the user opts out (`SliceBySlice=false`).
> **Affected users:** Users with overscan that opens to the boundary, or holes that sit on the volume boundary — these inputs cannot be cleanly handled by the legacy whole-volume algorithm. Slice-by-slice was added precisely for this case.
> **Recommendation:** Trust SIMPLNX. No legacy equivalent exists; document as an added capability rather than a behavioral deviation.
> **Status:** Proposed.

> **Deviation ID:** `IdentifySample-D2` — *Added capability: 2D / 1D / Single-voxel dispatch*
> **Filter UUID:** `94d47495-5a89-4c7f-a0ee-5ff20e6bd273`
> **Symptom:** SIMPLNX correctly handles ImageGeoms with one or two dimensions equal to 1 (e.g., a 3×4×1 slice or a 3×1×1 line) by dispatching to specialized neighbor-offset functors. SIMPL 6.5.172 likely either treats every input as a 3D volume (silently producing wrong neighbor counts when a dim is 1) or rejects the input.
> **Root cause:** PR #1590 added dimensionality-aware dispatch in SIMPLNX. Legacy was not updated. Pre-PR-#1590 SIMPLNX also had the bug; the three hand-built non-square 2D tests added in PR #1590 are the regression lock.
> **Affected users:** Anyone running `IdentifySample` on a 2D slice or a 1D line. In legacy, results on such inputs are likely silently wrong.
> **Recommendation:** Trust SIMPLNX. Confirm legacy behavior on a 3×4×1 input and, if legacy is wrong, document this as a SIMPLNX-only correctness improvement.
> **Status:** Proposed — pending verification on legacy.

> **Deviation ID:** `IdentifySample-D3` — *Added capability: uint8 mask support*
> **Filter UUID:** `94d47495-5a89-4c7f-a0ee-5ff20e6bd273`
> **Symptom:** SIMPLNX accepts both `boolean` and `uint8` mask arrays (see `ArraySelectionParameter::AllowedTypes` in `IdentifySampleFilter::parameters()`). SIMPL 6.5.172 likely accepts only the legacy bool/uint8 pattern that was standard at the time — confirm.
> **Root cause:** PR #1530 generalized the mask type via `ExecuteDataFunction` template dispatch.
> **Affected users:** Anyone who has thresholded into a uint8 mask rather than a bool mask.
> **Recommendation:** Trust SIMPLNX. No-op if legacy already accepts uint8.
> **Status:** Proposed — pending confirmation of legacy mask-type acceptance.
