# Retroactive V&V: ReplaceElementAttributesWithNeighborValuesFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `65128c53-d3be-4a69-a559-32a48d603884` |
| SIMPLNX ClassName | `ReplaceElementAttributesWithNeighborValuesFilter` |
| SIMPLNX Human Name | Replace Element Attributes with Neighbor (Threshold) |
| SIMPL UUID (legacy) | `17410178-4e5f-58b9-900e-8194c69200ab` *(noted as legacy in header)* |
| SIMPL ClassName | *(TBD — likely `ReplaceElementAttributesWithNeighborValues` in legacy SIMPL repo)* |
| SIMPL Human Name | *(TBD — confirm in legacy SIMPL repo)* |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/ReplaceElementAttributesWithNeighborValuesTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/ReplaceElementAttributesWithNeighborValuesFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/ReplaceElementAttributesWithNeighborValuesFilter.json`
- `src/Plugins/SimplnxCore/docs/ReplaceElementAttributesWithNeighborValuesFilter.md`
- `src/Plugins/SimplnxCore/pipelines/ReplaceElementAttributesWithNeighbor.d3dpipeline`

## Algorithm Relationship

- **Tentative classification:** **Port** — direct translation of the SIMPL filter; the SIMPLNX header preserves the legacy SIMPL UUID `17410178-4e5f-58b9-900e-8194c69200ab` in a comment, indicating a deliberate UUID change while staying functionally equivalent. The two-phase design (mark phase that fills `bestNeighbor[voxelIndex]`, then transfer phase that calls `copyTuple`), the per-iteration counter `count`, the `LessThanComparison` / `GreaterThanComparison` comparator class hierarchy, and the optional `loopUntilDone` outer `while` loop all read as a faithful translation of the legacy filter rather than a fresh design.
- **Evidence against rewrite:** No PR in the inspected window introduces or removes algorithm steps. PR #1523 (NeighborUtilities factor-out) is purely a code-extraction refactor — the per-direction `if(plane != 0)` / `if(column != 0)` … guard pattern and the `neighborPoints` → `neighborVoxelIndexOffsets` array were preserved exactly. PR #1590 only re-typed the array length to `VoxelNeighbors<Image3D>::k_FaceNeighborCount`.
- **Action required:** Confirm by reading the legacy SIMPL filter source. Run `compare-legacy-dream3d` (Step 0 e) on a shared toy dataset to confirm bit-equivalent output (or document any deviation).

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1457 static-inline cleanup, #1538 zlib unit-test extraction) are listed at the bottom of this section but not detailed individually — they did not change the behavior of this filter.

### PR #1523 — *"ENH: Factor out the 6-face neighbor code that is systemic through out the code base."* — merged 2026-02-05 *(broad refactor, exception flagged because this filter was a direct beneficiary of the new NeighborUtilities and the diff is non-trivial)*

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +28 / -28 lines (essentially a 1:1 substitution)
- **Change nature:** **Refactor.** Replaced a hand-rolled six-element offset array (`neighborPoints[]` initialized inline as `{-dims[0]*dims[1], -dims[0], -1, 1, dims[0], dims[0]*dims[1]}`) with a call to the new shared `initializeFaceNeighborOffsets(dims)` utility from `simplnx/Utilities/NeighborUtilities.hpp`. Also renamed the loop induction variable from `i` to `voxelIndex` throughout both inner loops and dropped a now-unused `DataGroup.hpp` include.
- **V&V content:** Refactor only — the offset values are identical to the previous inline literal. **However**, this PR did *not* add a regression test that pins the numerical output. The existing exemplar test (one TEST_CASE) covers it implicitly only because the exemplar is binary-equal to legacy.

### PR #1571 — *"DOC: Add standardized ChoicesParameter descriptions to filter docs"* — merged 2026-03-30

- **Files in this filter:** docs (.md), small addition
- **Change nature:** Documentation hygiene — added the `### Comparison Operator` subsection (lines 14-19 of the current docs) listing the two ChoicesParameter values (`< [Less Than] [0]` and `> [Greater Than] [1]`) with bold-bulleted descriptions of what each operator targets and what the corresponding replacement-direction rule is.
- **V&V content:** Doc currency improvement. The descriptive text is a useful audit artifact because it is the only natural-language record of the *replacement-direction asymmetry* (less-than mode chooses the highest-valued passing neighbor; greater-than mode chooses the lowest-valued passing neighbor). That asymmetry is implemented in the `compare2` method of the comparator subclasses and was previously undocumented.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +48 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/ReplaceElementAttributesWithNeighborValuesFilter.json`
  - `test/simpl_conversion/6_5/ReplaceElementAttributesWithNeighborValuesFilter.json`
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test (`"SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: SIMPL Backwards Compatibility"`) that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Asserts the converted Arguments contain `MinConfidence=2.5f`, `SelectedComparison=0`, `Loop=true`, `ComparisonDataPath=DataContainer/CellData/TestArray`, `SelectedImageGeometryPath=DataContainer`.
- **V&V content:** **Pipeline-conversion correctness only.** The test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values. It does **not** verify that the filter's *output* matches legacy. The fixture also confirms the SIMPL parameter key for the comparison array was `ConfidenceIndexArrayPath` (legacy carried over from the original "Replace Cells with Bad Confidence" intent — visible at `FromSIMPLJson` line 119 / 132 / 134 of the filter .cpp).

### PR #1590 — *"ENH: Standardize 2D Image Handling"* — merged 2026-04-23 *(broad refactor, exception flagged because of the policy's standing rule)*

- **Files in this filter:** algorithm (.cpp) only
- **Diff size:** 1 file, +2 / -1 lines
- **Change nature:** Pure typing cleanup. Replaced the hard-coded `std::array<int64, 6>` with `std::array<int64, k_NumFaceNeighbors>` where `k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount`. Despite the PR title's "2D Image Handling" framing, the change to *this* file is dimensionality-agnostic — the underlying `initializeFaceNeighborOffsets(dims)` utility is what handles the 2D case (a 2D image has `dims[2] == 1`, which means the `plane != 0` and `plane != (dims[2]-1)` guards trivially block the ±plane neighbors and only the four in-plane neighbors are used).
- **V&V content:** None directly — but the typing change is a small correctness ratchet: it now ties the offset-array length to `Image3D::k_FaceNeighborCount` so a hypothetical future jump to 26-neighbor would only need to flip one constant.
- **2D dispatch question:** The current code does *not* branch on dimensionality — the same six guarded neighbor checks run regardless of whether `dims[2]` is 1. This is correct (the `plane != 0` guard prevents indexing into a non-existent slice) but is worth a Deviation entry if the legacy filter explicitly skipped the plane checks for 2D.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1538 | Replace cmake subprocess tar.gz extraction with zlib | Test infrastructure |

### Local / unmerged work flagged

`git log --all` reveals four commits on remote feature branches (`joey/ooc-filter-optimizations`, `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation`) that touch this filter but have **not** been merged to `develop`:

- `d6ec06d5f` *"REV: Improve ReplaceElementAttributesWithNeighborValues algorithm quality"* — 2026-03-21
- `56a253826` *"TEST: Rewrite ReplaceElementAttributesWithNeighborValues unit tests"* — 2026-03-21
- `a49187007` *"TEST: Add IgnoredDataArrayPaths and error condition tests"* — 2026-03-21
- `2cd6f004c` *"ENH: Add throttled progress messaging to all filter algorithms"* — 2026-04-03
- `05bbd3277` *"PERF: Out-of-core (OOC) optimized algorithms ..."* — 2026-04-08
- `6219f32aa` *"DOCS: Add comprehensive Doxygen and inline documentation for OOC-optimized algorithms"* — 2026-04-14
- `f2e56a432` *"ENH: Apply PR #1590 to OOC dispatch variants and fix bool-mask bulk I/O"* — 2026-04-27

These represent in-flight algorithm-quality, test-rewrite, throttled progress messaging, and OOC-optimization work that supersedes much of what is on `develop` today. Coordinate with that branch owner before locking in any V&V baseline — a Class-1 fixture written today against `develop` may need to be rerun once these merge.

## Test coverage detected

`ReplaceElementAttributesWithNeighborValuesTest.cpp` contains 2 `TEST_CASE`s:

1. `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter` — exemplar comparison: loads `6_6_replace_element_attributes_with_neighbor.dream3d` as both the input and the exemplar, runs the filter with `MinConfidence=0.1`, `Comparison=0` (less-than), `Loop=true`, then `CompareExemplarToGeneratedData` over the cell attribute matrix.
2. `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via `DYNAMIC_SECTION` *(added by PR #1588)*.

**Coverage gaps observed in the test file:**

- The greater-than comparison mode (`SelectedComparison=1`) is **not** exercised by any test.
- The single-pass mode (`Loop=false`) is **not** exercised by any test.
- 2D imagery (`dims[2]==1`) is not exercised — the exemplar appears to be a 3D EBSD-style volume.
- Edge-of-volume voxel behavior (where one or more of the six neighbors are off-grid) is not isolated as a dedicated assertion.
- Tie-breaking among multiple equally-best neighbors is not tested.
- A pure no-op input (no voxel passes the threshold) is not tested.
- Loading the exemplar as both input *and* expected output means the test passes only if the input dataset already has been "cleaned" (i.e. no voxel passes the threshold), or the test relies on idempotence after one full pass. **This is unusual** — confirm whether the exemplar is the post-filter state and the input would normally be a *different* file. If so, the test as written may not actually exercise the filter's algorithm.

## Exemplar archive

- **Archive name:** `6_6_replace_element_attributes_with_neighbor.tar.gz`
- **SHA512:** `319ebdf08b83ce5ec915afda8ee2af1e0952a3ce26b3e65a4171eb3125c7bc6613c3994610bf526f70413c00da9061c3e6c2d867643220d62faa8c3cd79a96cd`
- **Referenced in:** `src/Plugins/SimplnxCore/test/CMakeLists.txt` line 230
- **Naming:** Uses the legacy `6_6_` prefix, indicating the data was generated from a DREAM3D 6.6 pipeline and carried forward.
- **Provenance:** *(TBD — engineer must inspect the archive to determine how the exemplar was generated and whether an Oracle Provenance block exists in any ReadMe inside it.)*
- **Action required:** Download the archive locally; inspect the inner `.dream3d` to confirm whether it contains *both* an input cell-data array (with sub-threshold voxels) and an exemplar cell-data array (without them), or whether it contains only one and the test is implicitly an idempotence test. Promote findings into the verification archive ReadMe per Step 0's Oracle Provenance policy.

## Oracle classification (tentative)

- **Recommended class:** **Class 1 (Analytical)** as primary, with **Class 4 (Invariant)** as a companion.
- **Rationale (Class 1):** This filter walks each voxel of a target element-attribute array; if the voxel's value passes a comparison threshold (less-than or greater-than a user value), it is replaced with the value of one of its six face neighbors (chosen as the neighbor with the most-extreme passing value). Pure topology + per-voxel comparison — no orientation math, no random sampling, no iterative solver. For any tiny fixture (e.g. a 4×4×1 image with three failing voxels and two passing neighbors) the expected output is hand-computable and can be hard-coded as a unit-test golden. **Class 1 should be the primary oracle**: at least one Class-1 fixture per (comparison-operator × loop × dimensionality) combination — six cases minimum.
- **Rationale (Class 4):** Several natural invariants are easy to encode and provide cheap regression coverage on real data:
  1. *Monotonicity per iteration.* `count` (number of voxels passing the threshold) is non-increasing across iterations of the outer `while` loop. **Caveat:** this invariant assumes that no replacement value can itself fall back into the failing range — verify by reading the comparator class's `compare1` predicate.
  2. *Idempotence on a clean volume.* Running the filter on a volume where no voxel passes the threshold leaves the data byte-identical to the input.
  3. *Fixed-point convergence.* When `loopUntilDone=true`, the algorithm terminates with `count == 0` after a finite number of iterations *or* with `count > 0` and a `bestNeighbor[i] == -1` for all remaining failing voxels (i.e. all surviving failing voxels are surrounded by other failing voxels).
  4. *Locality.* Every changed voxel's new tuple value is byte-identical to one of its (up to 6) face neighbors' tuple values from the *start of the current iteration*. This invariant matters because the implementation reads from `inputStore` (live) during the mark phase but copies entire tuples in a separate transfer phase — so the value used for the comparison and the value actually written are the same iteration's data.
- **Optional Class 3 (Paper-based):** Unlikely. This is a generalization of "replace bad EBSD voxels with neighbors" — likely no published reference; the algorithm is folklore-level.
- **Action required:** Developer to defend or replace the Class 1 + Class 4 recommendation. Building the Class-1 fixtures is the highest-leverage missing artifact.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review on `develop`. Quality work exists on the unmerged `joey/worktree-...Validation` branch (`d6ec06d5f`). |
| Code path coverage (algorithmic) | **Weak** | Only one combination tested: less-than × Loop=true × 3D. No greater-than coverage, no `Loop=false` coverage, no 2D coverage. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test. |
| Exemplar data in Data_Archive | **Yes** | `6_6_replace_element_attributes_with_neighbor.tar.gz` referenced in test/CMakeLists.txt. |
| Exemplar provenance documented | Unknown | TBD by inspecting archive contents. The `6_6_` legacy prefix suggests it was generated from a DREAM3D 6.6 pipeline. |
| Oracle class recorded | **No** | This document is the first to propose one. |
| Toy data / independent expected output (Step 0 c) | **No** | No script or hand-derivation on file. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. |
| Deviation entries (`ReplaceElementAttributesWithNeighborValues-D<N>`) | None | Not yet written. Several candidates flagged below. |
| Documentation currency | Probably current | Updated by PRs #1571 (ChoicesParameter description) and #1590 (no doc change). The doc does *not* document tie-breaking, edge-voxel handling, or what happens when *all* neighbors fail the threshold. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 (analytical) + Class 4 (invariant) is the recommended starting point. Defend or replace.
2. **Build the Class-1 toy fixtures.** Six minimum: `{less-than, greater-than} × {Loop=false, Loop=true} × {2D, 3D}`. Each fixture should be a 4×4×1 (2D) or 4×4×2 (3D) image with explicit input voxel values and explicit expected output voxel values, hard-coded into the test file. This becomes the oracle of record per Step 0 c.
3. **Add the Class-4 invariant assertions.** In a separate test (against the existing exemplar): `count` non-increasing across iterations; idempotence on a no-passing-voxel input; convergence; locality.
4. **Inspect `6_6_replace_element_attributes_with_neighbor.tar.gz` and document provenance.** Determine whether the archive contains an input dataset *and* a separate expected-output dataset, or only one. Write the Oracle Provenance block.
5. **Run the legacy comparison.** Use `compare-legacy-dream3d` with a small EBSD `.ang` or synthetic input. Verify SIMPLNX output is bit-equal to DREAM3D 6.5.171's `ReplaceElementAttributesWithNeighborValues` filter on the same input. Document any deviations.
6. **Investigate `bestNeighbor` reset (see Bug-pattern hunt below).** Determine whether stale `bestNeighbor[voxelIndex]` values from a prior outer-loop iteration can incorrectly trigger a tuple copy in a later iteration's transfer phase, and either fix or write a test that pins the current behavior.
7. **Investigate `best` typing (see Bug-pattern hunt below).** Determine whether `float32 best` (line 151 of the algorithm) silently truncates `int64`/`uint64`/`float64` array values; either fix or document.
8. **Coordinate with `joey/worktree-ReplaceElementAttrsWithNeighborValuesValidation`.** That branch contains an algorithm-quality pass (`d6ec06d5f`) and rewritten tests (`56a253826`, `a49187007`) that may resolve some of the gaps above; do not duplicate work.
9. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of the SIMPL `ReplaceElementAttributesWithNeighborValues` filter (legacy UUID `17410178-…`); refactored to use shared NeighborUtilities in PR #1523; no algorithmic changes since."*
10. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Bug-pattern hunt

The audit's two-batch pattern of "reassignment-inside-loop clobbering an earlier decrement" was searched for here. **No clean instance of that exact pattern was found** (this filter has no decrement-then-reassign sequence on a counter or divisor). However, three adjacent code-quality concerns surfaced and should be evaluated by the developer:

1. **`bestNeighbor` is not reset between outer-loop iterations.** It is allocated once at line 118 (`std::vector<int64_t> bestNeighbor(totalPoints, -1)`) before the outer `while(keepGoing)` block at line 131. After iteration 1 completes, voxels that *did* get replaced have a non-`-1` entry. In iteration 2, a voxel that used to fail the threshold but now passes (because neighbors changed underneath it during iteration 1's transfer phase) gets re-evaluated, but a voxel that *now* fails but did *not* fail in iteration 1 has a stale `-1` from initialization, which is fine. The risk case is: a voxel that *failed* in iteration 1, *got* a `bestNeighbor` set, was replaced, *now passes* in iteration 2 — but the mark phase at the top of iteration 2 doesn't enter the `if(comparator->compare(...))` block for it, so its old `bestNeighbor` entry is preserved, and the transfer phase at the bottom of iteration 2 will copy from the *now*-stale neighbor again, **overwriting the replacement that was just made in iteration 1**. This may produce an incorrect duplicate copy. **High priority to investigate.**

2. **`float32 best` regardless of array type T.** Line 151: `float32 best = inputStore[voxelIndex];`. If `T` is `int64`, `uint64`, or `float64`, the value silently truncates on the conversion to `float32`. The subsequent `compare2(inputArray[neighbor], best)` then compares `T` to `float32`, triggering implicit conversion in the other direction. For arrays whose values exceed the `float32` precision range (~24 bits of mantissa), neighbors that differ only in low-order bits will compare as equal and the *first* such neighbor in offset order will win. This is a latent precision bug that may not surface on small EBSD-confidence-style data (`float32` natively) but will on larger-range integer arrays.

3. **Tie-breaking is by offset order, not deterministic by content.** `compare2` uses strict comparison (`>` for less-than mode, `<` for greater-than mode). When two neighbors tie on the most-extreme passing value, the *first* one checked wins (offsets tested in order: −plane, −row, −col, +col, +row, +plane). This is implementation-defined behavior that the documentation does not explain. Promote to a Deviation entry if legacy differs.

4. **Asymmetric strict/non-strict comparison.** The outer threshold check uses strict `<` (or `>`). The neighbor-passing check uses non-strict `>=` (or `<=`). Consequence: a voxel whose value *equals* the threshold is **not** targeted for replacement, but **is** an eligible *replacement source*. This may or may not match legacy.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `ReplaceElementAttributesWithNeighborValues-D1`
> **Filter UUID:** `65128c53-d3be-4a69-a559-32a48d603884`
> **Symptom:** When `Loop=true`, voxels replaced in iteration N may be silently re-replaced in iteration N+1 using a stale `bestNeighbor[voxelIndex]` entry left over from iteration N.
> **Root cause:** `bestNeighbor` vector is allocated once before the outer `while(keepGoing)` loop and never reset between iterations (algorithm .cpp lines 118 and 131). The transfer phase (lines 201-220) blindly trusts whatever entry is in `bestNeighbor[voxelIndex]`, even if the mark phase of the *current* iteration did not write it.
> **Affected users:** Anyone running this filter with `Loop=true` on a dataset where the failing region requires more than one pass (i.e. the typical use case).
> **Recommendation:** Confirm the bug exists with a synthetic test (a 1×1×7 column of failing voxels with passing neighbors only at the ends — should require multiple iterations). If confirmed, add `std::fill(bestNeighbor.begin(), bestNeighbor.end(), -1);` at the top of the outer `while(keepGoing)` body. Compare the legacy SIMPL implementation — if it has the same bug, document and decide on a coordinated fix.
> **Status:** **Proposed — pending verification.** Highest-priority finding from this audit.

> **Deviation ID:** `ReplaceElementAttributesWithNeighborValues-D2`
> **Filter UUID:** `65128c53-d3be-4a69-a559-32a48d603884`
> **Symptom:** For input arrays of type `int64`, `uint64`, or `float64`, neighbor selection silently uses `float32` precision for the running "best" value, which can cause neighbors whose values differ only in low-order bits to be treated as ties.
> **Root cause:** Algorithm .cpp line 151 declares `float32 best = inputStore[voxelIndex];` without templating on `T`.
> **Affected users:** Any user comparing 64-bit numeric data via this filter.
> **Recommendation:** Change `float32 best` to `T best` (or `typename ArrayType::value_type best`).
> **Status:** Proposed — pending confirmation.

> **Deviation ID:** `ReplaceElementAttributesWithNeighborValues-D3`
> **Filter UUID:** `65128c53-d3be-4a69-a559-32a48d603884`
> **Symptom:** Tie-breaking among equally-extreme passing neighbors is by face-offset order (−plane, −row, −col, +col, +row, +plane), not documented, not exposed as a parameter.
> **Root cause:** `compare2` uses strict `>`/`<` (algorithm .cpp lines 51-53 and 76-78). The first neighbor visited that beats the *current* best value wins; subsequent equal-value neighbors are skipped.
> **Affected users:** Reproducibility-sensitive workflows where the input has many near-tied values.
> **Recommendation:** Document the tie-breaking rule in the filter's `.md`. If legacy uses a different tie-breaker, flag as a true deviation.
> **Status:** Proposed — pending legacy comparison.

> **Deviation ID:** `ReplaceElementAttributesWithNeighborValues-D4`
> **Filter UUID:** `65128c53-d3be-4a69-a559-32a48d603884`
> **Symptom:** A voxel whose value is *exactly equal to* the threshold is not targeted for replacement, but is an eligible replacement source.
> **Root cause:** Outer comparison `compare()` is strict (`<` / `>`); inner neighbor-passing comparison `compare1()` is non-strict (`>=` / `<=`) (algorithm .cpp lines 43-54 and 68-79).
> **Affected users:** Workflows that set the threshold to a value that occurs exactly in the data.
> **Recommendation:** Document the asymmetry. Confirm legacy uses the same convention.
> **Status:** Proposed — pending legacy comparison.

> **Deviation ID:** `ReplaceElementAttributesWithNeighborValues-D5`
> **Filter UUID:** `65128c53-d3be-4a69-a559-32a48d603884`
> **Symptom:** 2D image handling (`dims[2] == 1`) goes through the same six-neighbor code path as 3D, relying on the `plane != 0` and `plane != (dims[2]-1)` guards to skip the ±plane neighbors.
> **Root cause:** Implementation choice — no explicit 2D dispatch (algorithm .cpp lines 153-182). PR #1590's "Standardize 2D Image Handling" left this filter on the unified path.
> **Affected users:** Anyone running this filter on 2D imagery.
> **Recommendation:** This is correct as-implemented but should be confirmed against legacy and explicitly tested with a 2D fixture (one of the Class-1 fixtures recommended above).
> **Status:** Proposed — likely a documentation-only deviation rather than a behavioral one, pending legacy comparison.
