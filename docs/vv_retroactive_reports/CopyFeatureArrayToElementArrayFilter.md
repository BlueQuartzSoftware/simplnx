# Retroactive V&V: CopyFeatureArrayToElementArrayFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| SIMPLNX ClassName | `CopyFeatureArrayToElementArrayFilter` |
| SIMPLNX Human Name | Create Element Array from Feature Array |
| SIMPL UUID | `99836b75-144b-5126-b261-b411133b5e8a` |
| SIMPL ClassName | `CopyFeatureArrayToElementArray` (per `Filter_Name` in 6_4/6_5 fixtures) |
| SIMPL Human Name | Copy Feature Array To Element Array |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArray.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/CopyFeatureArrayToElementArrayTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/CopyFeatureArrayToElementArrayFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/CopyFeatureArrayToElementArrayFilter.json`
- `src/Plugins/SimplnxCore/docs/CopyFeatureArrayToElementArrayFilter.md`

Also referenced (consumers, not modified by this audit):
- `src/Plugins/ITKImageProcessing/pipelines/(02) Image Segmentation.d3dpipeline`
- `src/Plugins/ITKImageProcessing/pipelines/(04) Porosity Analysis.d3dpipeline`

## Algorithm Relationship

- **Tentative classification:** **Port** — the SIMPLNX filter is a direct functional translation of the legacy SIMPL `CopyFeatureArrayToElementArray` filter. The SIMPL UUID (`99836b75-…`) and SIMPLNX UUID (`4c8c976a-…`) differ but the `FromSIMPLJson()` converter and the 6.4/6.5 conversion fixtures map cleanly through three parameters (`SelectedFeatureArrayPath`, `FeatureIdsArrayPath`, `CreatedArrayName` → `CreatedArraySuffix`).
- **Behavioral notes:**
  - SIMPL took a *single* feature array per invocation; SIMPLNX accepts a *vector* of feature arrays via `MultiArraySelectionParameter` and runs the copy for each. The `SingleToMultiDataPathSelectionFilterParameterConverter` handles the single→multi promotion when reading legacy pipelines.
  - SIMPL took a `CreatedArrayName`; SIMPLNX took a `CreatedArraySuffix` that is appended to each input feature-array name. This is a parameter-semantics change but is otherwise pure naming.
  - The core algorithm — `output[v] = featureArray[FeatureIds[v]]` — is unchanged and is implemented as a parallel-for over voxels in a single templated functor (`CopyFeatureArrayToElementArrayImpl<T>` in the algorithm `.cpp`).
- **Action required:** Confirm by reading the corresponding SIMPL filter source and running `compare-legacy-dream3d` step (e) against a shared toy dataset.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs from the audit's standard pruning list (#1301, #1439, #1457, #1472, #1476, #1501, #1521, #1523, #1524, #1535, #1538, #1544, #1582) are listed at the bottom of this section but called out individually here only when their scoped diff actually touched this filter. **Two PRs from the pruned list (#1301 and #1521) plus #1544 form a tightly linked story for this filter and are detailed individually because together they performed the issue-#1284 algorithm-extraction work.** PR #1588 is also detailed because it added the SIMPL backwards-compatibility fixtures for this filter.

### PR #1301 — *"ENH: Add missing algorithm classes to some filters"* — merged 2026-01-08

- **Files in this filter:** algorithm (.hpp, .cpp) — both new
- **Diff size:** 2 files, +84 / -0 lines
- **Change nature:** Issue-#1284 step 1 for this filter. Created the empty algorithm-class scaffolding (`CopyFeatureArrayToElementArray.{hpp,cpp}`) under `Filters/Algorithms/`. The filter's `executeImpl()` was *not* yet rewired; this commit only added the placeholder pair.
- **V&V content:** None (scaffolding-only). Listed individually because it is the first half of the algorithm-class extraction story.

### PR #1521 — *"BUG: Move unused algorithm codes to internal directory"* — merged 2026-02-04

- **Files in this filter:** algorithm (.hpp, .cpp) — both moved
- **Diff size:** 2 files renamed (0 byte change), `Algorithms/` → `Algorithms/not_used/`
- **Change nature:** Recognised that the scaffolding from #1301 was still unused (the filter's `executeImpl()` had not been migrated) and parked the empty pair in `not_used/` so it would not skew GCov coverage numbers. This is the issue-#1284 holding pattern documented in `CLAUDE.md`.
- **V&V content:** None.

### PR #1544 — *"ENH: Move non-trivial Filter executeImpl() logic to Algorithm classes"* — merged 2026-02-26

- **Files in this filter:** algorithm (.hpp, .cpp), filter (.cpp)
- **Diff size:** 3 files, +145 / -72 lines
- **Change nature:** **The actual algorithm-class extraction.** Real work this time:
  - The algorithm pair was promoted out of `not_used/` and rewritten with the `CopyFeatureArrayToElementArrayInputValues` struct and an `operator()` that owns the per-feature-array loop, the `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` call, and the `ParallelDataAlgorithm` dispatch via `ExecuteParallelFunction<CopyFeatureArrayToElementArrayImpl>`.
  - The filter's `executeImpl()` shrank to ~7 lines that pack parameters into `InputValues` and call `CopyFeatureArrayToElementArray(...)()`.
  - This is one of 24 filters converted in the same PR — listed by the PR description.
- **V&V content:** **Refactor only — no algorithmic change intended.** Behavior of the filter from the user's perspective should be identical to its pre-#1544 form. Worth re-running the existing unit tests to confirm there was no regression, since no new tests were added for this PR.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +47 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/CopyFeatureArrayToElementArrayFilter.json` (~600 bytes, no `Filter_Uuid` — exercises the Filter_Name lookup path)
  - `test/simpl_conversion/6_5/CopyFeatureArrayToElementArrayFilter.json` (~700 bytes, has `Filter_Uuid` `{99836b75-144b-5126-b261-b411133b5e8a}`)
- **Change nature:** **Test addition.** Added a per-filter SIMPL→SIMPLNX backwards-compatibility test that exercises both SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. Test name: `"SimplnxCore::CopyFeatureArrayToElementArrayFilter: SIMPL Backwards Compatibility"`.
- **V&V content:** **Pipeline-conversion correctness only** — the test verifies that opening a legacy SIMPL pipeline in DREAM3DNX produces a filter instance with the right parameter values (`CellFeatureIdsArrayPath` → `DataContainer/CellData/TestArray`, `CreatedArraySuffix` → `"TestName"`). It does **not** verify that the filter's *output* matches legacy. That latter step is still missing. Note: the fixture's `SelectedFeatureArrayPath` is *not* asserted in the test (comment: "Complex type … verified by successful pipeline loading") — the single-to-multi conversion is exercised but its result is not check-equal'd.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned (and what it touched here) |
|---|---|---|
| #1457 | Clean up 'static inline' from filter headers | Style — 6/3 lines in the filter `.hpp` to remove `static inline` from parameter-key constants. |
| #1438 | (microtexture cleanup — exception PR per audit policy) | **Did NOT touch this filter.** Confirmed via git log; called out for completeness. |

No other audit-list pruned PRs (#1439, #1466, #1472, #1476, #1490, #1501, #1523, #1524, #1535, #1538, #1571, #1582) touched this filter's source files in the inspected window.

## Test coverage detected

`CopyFeatureArrayToElementArrayTest.cpp` contains 3 `TEST_CASE`s (one of which is template-list expanded into 10 instantiations):

1. `SimplnxCore::CopyFeatureArrayToElementArrayFilter: Parameter Check` — constructs the filter with empty selected-array list, confirms preflight returns `k_Validate_Empty_Value`, confirms execute returns the same. Negative path only.
2. `SimplnxCore::CopyFeatureArrayToElementArrayFilter: Valid filter execution` *(TEMPLATE_LIST_TEST_CASE over int8/uint8/int16/uint16/int32/uint32/int64/uint64/float32/float64 → 10 instances)* — builds a 10×3 cell `FeatureIds` array (all zeros except by row → feature ids 0,1,2), creates two 3-tuple feature arrays, runs the filter with two simultaneous targets, then asserts `output[v] == featureArray[FeatureIds[v]]` voxel-by-voxel for both outputs.
3. `SimplnxCore::CopyFeatureArrayToElementArrayFilter: SIMPL Backwards Compatibility` — SIMPL 6.4 + 6.5 conversion paths via DYNAMIC_SECTION *(added by PR #1588)*.

Tests 1 and 2 cover the algorithmic surface (negative + scalar single-component for 10 numeric types). Test 3 is conversion-only.

**Gaps in the algorithmic test surface:**
- Multi-component (vector / tensor) feature arrays are not exercised — yet the algorithm explicitly loops `faComp` from 0 to `getNumberOfComponents()-1` and is intended to support them.
- Bool / string feature arrays are not in the type list (the filter advertises `nx::core::GetAllDataTypes()` so this is a real gap).
- `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` (max-FeatureId-vs-feature-array-tuple-count check inside the algorithm) has no test that triggers its error path.
- The current valid-execution test sets all feature values to zero on the temperature array (`avgTempValue[i] = 0`), so the equality check is trivially satisfied even if the index arithmetic were wrong. The second array (`featureDataValue`) is *uninitialized* — the test compares it to itself and would pass even with totally incorrect indexing. This is a verification weakness worth fixing.

## Exemplar archive

- **No exemplar archive is referenced for this filter.** `src/Plugins/SimplnxCore/test/CMakeLists.txt` lists `CopyFeatureArrayToElementArrayTest.cpp` only as a test source; there is no matching `download_test_data()` call for any `copy_feature_array*` archive.
- The valid-execution test builds its DataStructure procedurally in-test, so no on-disk exemplar is currently needed. For Step 0 / legacy-comparison purposes a small `.dream3d` exemplar (with explicit per-feature values, multi-component arrays, and a non-trivial FeatureIds map) should still be created.
- **Action required:** Generate a tar.gz with (a) a SIMPL 6.5.171 pipeline producing a known output, (b) the matching SIMPLNX pipeline producing the same output, (c) a small `.dream3d` with hand-derived expected outputs. Upload to `Data_Archive` and add a `download_test_data()` entry.

## Oracle classification (tentative)

- **Recommended class:** **1 (Analytical)** with **4 (Invariant-based)** as a companion.
- **Rationale:** This filter is pure indirection. The expected output is a one-line formula:
  ```
  output[v * C + c] = featureArray[FeatureIds[v] * C + c]   for all v, all c
  ```
  where `v` indexes the per-element (cell/vertex) array, `c` indexes the component within a multi-component feature array, and `C = getNumberOfComponents()`. There is no floating-point arithmetic, no neighborhood traversal, no iteration to convergence — the output is bit-exact for any value type. That makes it the canonical Class-1 case.
- **Class-4 invariants that should also be asserted in the test:**
  - For every pair of voxels `v1, v2` with `FeatureIds[v1] == FeatureIds[v2]` and the same `c`, `output[v1*C+c] == output[v2*C+c]` (the "constant within a feature" invariant).
  - For every voxel `v`, `0 <= FeatureIds[v] < featureArray.getNumberOfTuples()` is a precondition; if violated, the algorithm relies on `ValidateFeatureIdsToFeatureAttributeMatrixIndexing` to reject. A negative test should confirm the rejection.
  - The output array's `getNumberOfTuples()` equals the FeatureIds array's `getNumberOfTuples()` and `getComponentShape()` equals the source feature array's shape.
- **Action required:** Developer to confirm Class-1 over Class-4-only. If the user prefers Class 4 (e.g., to match the rest of the audit), Class 4 still works because the invariants above are sufficient — but Class 1 is the more honest description for an indirection-only operation.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | The algorithm is small (≈30 LOC of real work) and was extracted from the filter in PR #1544 without behavioral change. A formal review should still be checked off. |
| Code path coverage (algorithmic) | **Partial** | 10 numeric types covered for single-component arrays; multi-component, bool, and the validation error path are uncovered. The valid-execution test's equality assertion is partly trivial (zero-filled / self-compare) — see Test coverage section. |
| Code path coverage (SIMPL conversion) | Good | PR #1588 added SIMPL 6.4 + 6.5 conversion test, but the `SelectedFeatureArrayPath` value is not asserted (comment in test). |
| Exemplar data in Data_Archive | **No** | No `download_test_data()` for this filter. Test data is built procedurally in-process. |
| Exemplar provenance documented | N/A | No archive yet. |
| Oracle class recorded | **No** | This document is the first to propose one (Class 1, with Class 4 companion). |
| Toy data / independent expected output (Step 0 c) | **Partial** | The valid-execution test's per-voxel equality check *is* an analytic oracle, but it has the trivial-data weakness noted above. A hand-derived multi-component case is missing. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run against SIMPL 6.5.171. Especially relevant given the SIMPL→SIMPLNX `CreatedArrayName` → `CreatedArraySuffix` semantic shift. |
| Deviation entries (`CopyFeatureArrayToElementArray-D<N>`) | None | None recorded. The single→multi parameter promotion and the name→suffix change are documentation-worthy regardless of whether they count as Deviations. |
| Documentation currency | **Stale** | `docs/CopyFeatureArrayToElementArrayFilter.md` is 16 lines and still uses the old "Xmdf" wording (presumably "Xdmf"). It does not describe the multi-array, suffix-vs-name semantics, or the `Cell Feature Ids` parameter. Needs a `review-filter-docs` pass. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 (analytical) is the natural fit because the algorithm is `output[v] = featureArray[FeatureIds[v]]`. Document Class 4 invariants as the assertion mechanism.
2. **Strengthen the existing valid-execution test.** Replace the all-zeros temperature array and the uninitialized feature-data array with hand-chosen distinct values per feature so that any off-by-one or transposition would actually fail an assertion. Add a multi-component (e.g., 3-component) variant. Add a bool variant or explicitly document why it's excluded.
3. **Add a negative test for the FeatureIds → feature-array indexing validator.** Set `FeatureIds[k] = 999` for one voxel where the feature array only has 3 tuples, and confirm the algorithm returns an invalid `Result<>`.
4. **Generate a tar.gz exemplar.** Even though the test currently builds inputs procedurally, a small `.dream3d` exemplar (with the multi-component case above) should be published so the same data is available for legacy comparison and documentation screenshots.
5. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.171 on the same toy data. Pay attention to:
   - The single→multi parameter promotion (loading a SIMPL pipeline with one input and confirming the output array name matches what 6.5.171 produced).
   - The `CreatedArrayName` (legacy) vs. `CreatedArraySuffix` (NX) semantic difference — the converter prepends/appends correctly, but the resulting array path should be confirmed by-the-byte against legacy.
6. **Refresh the documentation.** Fix the "Xmdf" typo (→ "Xdmf"), add a section on the suffix-vs-name semantics, document multi-array support, and call out which numeric types are supported.
7. **Produce the Algorithm Relationship one-liner.** Tentative: *"Port — direct translation of SIMPL `CopyFeatureArrayToElementArray` (UUID `99836b75-…`); promoted single feature-array input to multi-array via `MultiArraySelectionParameter` and replaced `CreatedArrayName` with `CreatedArraySuffix`. Algorithm body unchanged. Algorithm-class extraction performed in PR #1544."*
8. **Archive everything** per `archive-filter-verification` for the OneDrive folder.

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `CopyFeatureArrayToElementArray-D1`
> **Filter UUID:** `4c8c976a-993d-438b-bd8e-99f71114b9a1`
> **Symptom:** SIMPLNX accepts a vector of feature arrays in a single filter invocation; SIMPL 6.5.171 accepts only one. A SIMPLNX pipeline that copies N feature arrays in one step requires N consecutive filter instances under SIMPL.
> **Root cause:** Deliberate API improvement in SIMPLNX. `MultiArraySelectionParameter` replaces SIMPL's `SelectedFeatureArrayPath` single-array parameter. `FromSIMPLJson()` uses `SingleToMultiDataPathSelectionFilterParameterConverter` to handle legacy pipelines transparently.
> **Affected users:** Anyone porting SIMPL pipelines that copy multiple feature arrays should expect N legacy steps to collapse into 1 NX step. Output names are unchanged.
> **Recommendation:** Trust SIMPLNX. Document the conversion in user-facing docs.
> **Status:** Proposed — pending verification that the byte-for-byte output names and values match between the legacy N-step pipeline and the NX 1-step pipeline.

> **Deviation ID:** `CopyFeatureArrayToElementArray-D2`
> **Filter UUID:** `4c8c976a-993d-438b-bd8e-99f71114b9a1`
> **Symptom:** SIMPLNX takes a `CreatedArraySuffix` (string appended to each input array's name); SIMPL 6.5.171 took a `CreatedArrayName` (full output name for the single-array case). A user who set `CreatedArrayName = "MyOutput"` in legacy will see `MyOutput` as the suffix in NX (so the output name becomes `<inputName>MyOutput` rather than `MyOutput`).
> **Root cause:** Necessary API change once the input was promoted from single to multi (a single output name does not work for N inputs). The `LinkedPathCreationFilterParameterConverter` performs the conversion but the resulting *string* is what the legacy user typed, applied as a suffix.
> **Affected users:** Anyone whose downstream pipeline references the post-copy output array by an exact, non-suffixed name. The output array path will differ from legacy unless the user manually renames in the converted pipeline.
> **Recommendation:** Document explicitly. Consider adding a `WARNING` to the `FromSIMPLJson()` converter when the legacy `CreatedArrayName` does not appear to start with an underscore or other conventional suffix marker.
> **Status:** Proposed — pending verification that the converter behavior matches the description above on a real legacy fixture.
