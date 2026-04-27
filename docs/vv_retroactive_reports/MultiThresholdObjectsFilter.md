# Retroactive V&V: MultiThresholdObjectsFilter

*Report status:* **DRAFT**. Generated from git-history and source-tree inspection. Developer must confirm or correct the Oracle class, Algorithm Relationship, and the V&V status entries.

## Metadata

| Field | Value |
|---|---|
| SIMPLNX UUID | `4246245e-1011-4add-8436-0af6bed19228` |
| SIMPLNX ClassName | `MultiThresholdObjectsFilter` |
| SIMPLNX Human Name | Multi-Threshold Objects |
| SIMPL UUID (v1) | `{014b7300-cf36-5ede-a751-5faf9b119dae}` (legacy `MultiThresholdObjects`) |
| SIMPL UUID (v2/advanced) | `{686d5393-2b02-5c86-b887-dd81a8ae80f2}` (legacy `MultiThresholdObjects2`) |
| SIMPL ClassName | `MultiThresholdObjects` (v1) / `MultiThresholdObjects2` (v2 advanced) |
| SIMPL Human Name | Multi Threshold Objects |
| Plugin | SimplnxCore |

### Source files scanned

- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/MultiThresholdObjectsFilter.{hpp,cpp}`
- `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.{hpp,cpp}`
- `src/Plugins/SimplnxCore/test/MultiThresholdObjectsTest.cpp`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_4/MultiThresholdObjectsFilter.json`
- `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/MultiThresholdObjectsFilter.json`
- `src/Plugins/SimplnxCore/docs/MultiThresholdObjectsFilter.md`

## Algorithm Relationship

- **Tentative classification:** **Port** of two related SIMPL filters into a single SIMPLNX filter — the legacy `MultiThresholdObjects` (v1) and `MultiThresholdObjects2` (v2 "advanced", with nested `ArrayThresholdSet`) both convert to this single SIMPLNX filter via `FromSIMPLJson` UUID/Filter_Name dispatch.
- **Evidence:** `FromSIMPLJson()` explicitly checks for both legacy UUIDs / class names and routes to either `ComparisonSelectionFilterParameterConverter` (v1) or `ComparisonSelectionAdvancedFilterParameterConverter` (v2 advanced). Algorithm logic in `MultiThresholdObjects.cpp` is straightforward elementwise comparison + per-set boolean reduction (`std::less<>`, `std::greater<>`, `std::equal_to<>`, `std::not_equal_to<>` over `AbstractDataStore::getComponentValue`), recursively walking `ArrayThresholdSet` collections.
- **Action required:** Confirm by reading legacy SIMPL `MultiThresholdObjects` and `MultiThresholdObjects2` source and running `compare-legacy-dream3d` on a shared toy dataset.

## PRs inspected (since 2025-10-01)

> Pruned: pure-style/repo-wide refactor PRs (#1439 NeighborList API change, #1457 static-inline, #1524 test-tag cosmetic) are listed at the bottom and not detailed individually — they did not change behavior of this filter.

### PR #1301 — *"ENH: Add missing algorithm classes to some filters"* — merged 2026-01-08 (broad refactor, exception flagged because it was the Algorithm-class **stub creation** event for THIS filter)

- **Files in this filter:** algorithm (.hpp +67, .cpp +26) — created
- **Diff size:** 2 files, +93 / -0 lines
- **Change nature:** Created the Algorithm-class **skeleton** (`MultiThresholdObjects` class with empty `operator()` returning `Result<>{}`). The filter's `executeImpl()` was *not* yet rewired to call this stub at this point — the real execution logic still lived in `MultiThresholdObjectsFilter.cpp`.
- **V&V content:** None — pure scaffolding.

### PR #1476 — *"BUG/ENH: Fix Backwards Pipeline Compatibility and Add Testing"* — merged 2026-01-06

- **Files in this filter:** filter (.cpp), small diff in `FromSIMPLJson()`
- **Change nature:** **Backwards compatibility fix.** Made `Filter_Uuid` lookup defensive (`json.contains(k_FilterUuidKey)`) instead of raw `operator[]` (which would UB-crash under NDEBUG when the key was missing). Switched ScalarType conversion from `ScalarTypeParameterToNumericTypeConverter` to `ScalarTypeParameterConverter` and made it tolerant of being absent (the legacy 6.5 JSON didn't always emit it).
- **V&V content:** Backwards-compat hardening; not algorithmic.

### PR #1502 — *"Defaulting MultiThresholdObjects output array to uint8"* — merged 2026-01-15

- **Files in this filter:** filter (.cpp) — 1-line change, default `DataType::boolean` -> `DataType::uint8`
- **Diff size:** 2 files, +19 / -2 lines (the +19 is in `UnitTestCommon.hpp` for support helpers)
- **Change nature:** **Behavioral default change.** The default output mask `DataType` flipped from `boolean` to `uint8`. This is a **user-visible** default and is the kind of thing that needs a Deviation entry vs. legacy SIMPL — legacy `MultiThresholdObjects` emits a uint8 mask too historically, so this brings the SIMPLNX default *back* in line with legacy behavior. Pipelines that explicitly set the parameter are unaffected.
- **V&V content:** Default change documented in PR title only; no test added that pins the new default.

### PR #1521 — *"BUG: Move unused algorithm codes to internal directory"* — merged 2026-02-04 (broad refactor, exception flagged because the algorithm stub was moved INTO `not_used/`)

- **Files in this filter:** algorithm (.hpp -67, .cpp -26)
- **Diff size:** 2 files, +0 / -93 lines (pure file rename `Algorithms/ -> Algorithms/not_used/`)
- **Change nature:** The empty Algorithm stub created by #1301 was identified as unused by the filter (the filter's `executeImpl` still ran inline) and was relocated to `Algorithms/not_used/` for gcov-coverage hygiene. **Note:** this is the *opposite* direction the project would later need — see #1544.
- **V&V content:** None.

### PR #1544 — *"ENH: Move Filter executeImpl() logic to Algorithm classes"* — merged 2026-02-26 (broad refactor, exception flagged because executeImpl was actually moved to Algorithm for THIS filter)

- **Files in this filter:** algorithm (.cpp +271, .hpp +67) — **re-created and populated**, filter (.cpp -242 / +12)
- **Diff size:** 3 files, +350 / -242 lines
- **Change nature:** **Major refactor.** The filter's full `executeImpl()` body — `ThresholdFilterHelper`, `InsertThreshold`, `ThresholdValue`, `ThresholdSet`, the `ExecuteThresholdHelper` / `ThresholdValueFunctor` / `ThresholdSetFunctor` type-dispatch wrappers, and the top-level `operator()` driver — was lifted out of `MultiThresholdObjectsFilter.cpp` and moved into a real (no longer empty) `Algorithms/MultiThresholdObjects.{hpp,cpp}`. Filter `executeImpl` shrank to the standard pattern: pack `MultiThresholdObjectsInputValues`, call `MultiThresholdObjects(...)()`. Behavior is intended to be identical.
- **V&V content:** No new tests; existing test suite is the regression net for this refactor. Worth noting since the move was substantive (271 lines of moved logic).

### PR #1582 — *"ENH: Add missing cancel checks to lots of filters"* — merged 2026-04-08 (broad refactor, exception flagged because cancel-check was added inside this filter's algorithm loop)

- **Files in this filter:** algorithm (.cpp), +4 lines
- **Change nature:** Added an `if(m_ShouldCancel) { return {}; }` guard inside the per-threshold loop in `MultiThresholdObjects::operator()()`. Allows long-running thresholds (large arrays × many threshold rows) to honor user cancel.
- **V&V content:** Cancel-handling correctness; not algorithmic. Returning `Result<>{}` on cancel (rather than an error) silently produces a partially-populated mask; downstream filters will see the partial mask. This is consistent with the rest of the codebase but is worth flagging as a Deviation candidate vs. legacy if SIMPL behaves differently on cancel.

### PR #1588 — *"ENH: SIMPL Backwards Compatibility Test Redesign"* — merged 2026-04-22

- **Files in this filter:** test (.cpp) +49 lines, plus two new fixture files
  - `test/simpl_conversion/6_4/MultiThresholdObjectsFilter.json` (636 bytes, no `Filter_Uuid`)
  - `test/simpl_conversion/6_5/MultiThresholdObjectsFilter.json` (693 bytes, with `Filter_Uuid`)
- **Change nature:** **Test addition.** Added `TEST_CASE("SimplnxCore::MultiThresholdObjectsFilter: SIMPL Backwards Compatibility")` exercising both 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) pipeline conversion paths via `DYNAMIC_SECTION`. At merge time, the only assertion was that the converted pipeline contained one `PipelineFilter` with the right UUID (the value-checks were stubbed with comments).
- **V&V content:** Pipeline-conversion correctness only — does NOT verify that filter *output* matches legacy. Legacy comparison is still pending.

### PR #1605 — *"BUG: Fix SIMPL JSON conversion segfault and re-enable backwards-compatibility checks"* — merged 2026-04-23

- **Files in this filter:** filter (.cpp +7), test (.cpp): un-stubbed CHECK statement, fixture files: repaired
- **Change nature:** Two parts.
  1. **Bug fix in `FromSIMPLJson()`**: added a `Filter_Name` fallback so SIMPL 6.4 pipelines (which omit `Filter_Uuid`) correctly route to the v1 vs. advanced converter based on the legacy class name string. Without this, all 6.4 conversions silently took the v1 path even for `MultiThresholdObjects2` content.
  2. **Test hardening**: replaced the stubbed-out `// CHECK(...)` comment in the SIMPL backwards-compat test with a real `CHECK(args.value<std::string>(MultiThresholdObjectsFilter::k_CreatedDataName_Key) == "TestName")`, which now actually validates that the converted Arguments carry the right destination-array name. Also repaired the fixture JSON files (per the omnibus PR description).
- **V&V content:** **Material** — this is a real conversion-path bug fix plus test enabling. The off-by-converter-route bug it fixes is a Deviation candidate vs. SIMPL 6.4 if the user has 6.4 pipelines stored with `MultiThresholdObjects2`.

### Pruned PRs (touched the file but not behaviorally relevant to this filter)

| PR | Subject | Why pruned |
|---|---|---|
| #1439 | Multi-Dimensional Tuple Support for StringArray and NeighborList | API change, no per-filter behavior change |
| #1457 | Clean up 'static inline' from filter headers | Style |
| #1524 | Fixed filter tags to consistently use the full filter name | Test cosmetic |

## Test coverage detected

`MultiThresholdObjectsTest.cpp` contains the following `TEST_CASE`s:

1. `SimplnxCore::MultiThresholdObjects: Valid Execution` — `[SimplnxCore][MultiThresholdObjectsFilter]` with two `SECTION`s (Float Array Threshold, Int Array Threshold).
2. `SimplnxCore::MultiThresholdObjects: Valid Execution - Custom Values` — TEMPLATE_TEST_CASE over `int8, uint8, int16, uint16, int32, uint32, int64, uint64, float32, float64` exercising custom TRUE/FALSE value paths.
3. `SimplnxCore::MultiThresholdObjects: Invalid Execution` — `SECTION`s for: Empty ArrayThresholdSet, Empty ArrayThreshold DataPath, Mismatching Components in Threshold Arrays, Out of Bounds Component Index, Mismatching Tuples in Threshold Arrays.
4. `SimplnxCore::MultiThresholdObjects: Invalid Execution - Out of Bounds Custom Values` — TEMPLATE_TEST_CASE over the integer types with `SECTION`s True Value < Min, False Value < Min, True Value > Max, False Value > Max.
5. `SimplnxCore::MultiThresholdObjects: Invalid Execution - Boolean Custom Values` — `SECTION`s Custom True Value, Custom False Value (both must error when `DataType::boolean` is selected).
6. `SimplnxCore::MultiThresholdObjects: Valid Execution, DataType` — `SECTION` for every output `DataType` (Int8, Int16, Int32, Int64, UInt8, UInt16, UInt32, UInt64, Float32, Float64).
7. `SimplnxCore::MultiThresholdObjects: Valid Execution - Multicomponent` — exercises the per-component-index code path.
8. `SimplnxCore::MultiThresholdObjectsFilter: SIMPL Backwards Compatibility` — `[BackwardsCompatibility]` tag, two `DYNAMIC_SECTION`s (SIMPL 6.5 UUID, SIMPL 6.4 Filter_Name fallback) *(added by PR #1588, hardened by PR #1605)*.

Tests 1–7 are the algorithmic suite; test 8 is conversion-only. The TEMPLATE_TEST_CASE expansions (tests 2 and 4) materially multiply coverage across numeric output types.

## Exemplar archive

- **Archive name:** *(none — this filter does not download a `.tar.gz` exemplar archive)*
- **Referenced in:** N/A
- **Provenance:** Tests construct input arrays in-memory in the test code (no external `.dream3d` exemplar files). Expected outputs are likewise computed in-test. This is appropriate for a Class 1 (analytical) oracle but means there is no archived golden reference data.
- **Action required:** None for archive download. For the V&V archive, capture the test source itself and any reference-mask hand calculations as the oracle artifact.

## Oracle classification (tentative)

- **Recommended class:** **Class 1 (Analytical)** with **Class 4 (Invariant) companion**.
- **Class 1 rationale:** This filter is pure boolean algebra over per-element comparisons. For any toy input array and threshold spec, the expected output mask is computable exactly by direct elementwise evaluation: `mask[i] = combinator(op_1(input_1[i, c_1], val_1), op_2(input_2[i, c_2], val_2), ...)`. No floating-point reductions, no iterative convergence, no random sampling — the answer is deterministic and bit-exact computable in any other language (numpy `np.where`, hand-pencil for tiny inputs).
- **Class 4 invariants** (for property-based assertions on top of Class 1):
  - **Cardinality bound:** `count(mask == TRUE) + count(mask == FALSE) == numTuples`; `count(mask == TRUE) <= numTuples`.
  - **Double-negation idempotence:** Inverting the `ArrayThresholdSet` twice produces the same mask. (Note: `isInverted()` is honored by the algorithm.)
  - **De Morgan:** For two single-array thresholds `A` and `B`, `NOT(A AND B) == (NOT A) OR (NOT B)` voxel-for-voxel.
  - **Custom value preservation:** When `useCustomTrueValue` is on with value `T_c`, `forall i: mask[i] in {T_c, F_c}` (no leaked default 0/1 values).
  - **Output dtype invariant:** Mask `DataType` matches `CreatedMaskType` parameter exactly.
- **Action required:** Defend or replace. Class 1 is the natural fit; Class 4 is cheap to add as additional `REQUIRE` assertions on top of the existing exemplar-style checks.

## V&V status so far

| Item | Status | Notes |
|---|---|---|
| Algorithm review (`review-algorithm` skill) | Not visible from PR history | No PR explicitly performs the line-by-line review of `MultiThresholdObjects.cpp`. The `InsertThreshold` / `ThresholdSet` recursion deserves a focused review for the `replaceInput` + `inverse` interaction (see Gap 5). |
| Code path coverage (algorithmic) | **Strong** | TEMPLATE_TEST_CASEs over 10 numeric types × custom-value cross-product, plus dedicated multicomponent and DataType section tests. |
| Code path coverage (SIMPL conversion) | **Good** | PR #1588 + #1605 added SIMPL 6.4 (Filter_Name fallback) and 6.5 (UUID-mapped) tests. |
| Exemplar data in Data_Archive | **N/A** | No `.tar.gz` archive — tests use in-memory inputs/outputs. |
| Exemplar provenance documented | N/A | See above. |
| Oracle class recorded | **No** | This document is the first to propose one (Class 1 + Class 4). |
| Toy data / independent expected output (Step 0 c) | **Implicitly yes** | The in-test hand-computed expected masks effectively serve as Class-1 oracle answers. They are not, however, separately documented or cross-checked in numpy. |
| Legacy comparison report (Step 0 e) | **No** | `compare-legacy-dream3d` has not been run. SIMPL `MultiThresholdObjects` v1 vs. v2-advanced both need comparison. |
| Deviation entries (`MultiThresholdObjects-D<N>`) | None | Not yet written. PR #1502 (default DataType change) and PR #1605 (6.4 Filter_Name routing fix) are both Deviation candidates. |
| Documentation currency | **Stale-ish** | Doc still says "boolean threshold array" in the example text but the default output type is now `uint8` (changed by #1502). The custom-value paragraph and the warning about downstream-filter compatibility are accurate. Needs `review-filter-docs` pass. |
| Verification archive (OneDrive) | No | Not yet created. |

## Gaps to close (to meet Step 0 / Legacy Comparison policy)

1. **Confirm the oracle.** Class 1 (analytical) is the recommended starting point with Class 4 invariant companions. The existing test code already encodes the Class-1 oracle inline; promote those into a documented oracle artifact for the verification archive.
2. **Run the legacy comparison.** Use `compare-legacy-dream3d` to diff SIMPLNX vs. DREAM3D 6.5.171 on toy data covering: (a) v1 `MultiThresholdObjects` single-condition pipelines and (b) v2 `MultiThresholdObjects2` nested-set pipelines. Expected outcomes: at least one Deviation entry for the default-mask-type change (PR #1502) if any user pipeline relied on the SIMPLNX-side default before that fix.
3. **Document the dual-source-filter port.** This SIMPLNX filter consolidates two SIMPL filters (v1 + v2-advanced). The Algorithm Relationship one-liner should make this explicit so anyone reading the V&V record understands that two legacy UUIDs map here.
4. **Add Class-4 invariant assertions.** The existing tests are exemplar-style (compute expected, compare). Add cheap `REQUIRE` invariants on top: cardinality, double-negation, custom-value-preservation, output-dtype. These are property-based safety nets that catch future regressions even if the exemplar arithmetic gets edited along with the bug.
5. **Audit the `replaceInput` + `inverse` + `firstValueFound` recursion in `ThresholdSet`/`InsertThreshold`.** The way `firstValueFound` flips `replaceInput` true on the first sub-threshold and then `InsertThreshold` is invoked recursively with the parent's union operator and inversion flag is non-obvious. There is also a `std::reverse(tempResultVector...)` branch on `replaceInput && inverse` (lines ~138 and ~194 of `MultiThresholdObjects.cpp`) that *reverses the per-tuple ordering of the temp vector* — this looks suspicious for a boolean-mask inversion (would normally be elementwise NOT, not array reverse). Worth a focused algorithm review and a unit test that pins behavior on a known nested + inverted case.
6. **Update documentation.** Default DataType changed from boolean to uint8 (PR #1502); the description still talks about "the boolean threshold array produced will contain *false*, *false*, ...". Update example text to say "for the default uint8 output type, 0s and 1s; for boolean output, false/true."
7. **Archive everything** per `archive-filter-verification` for the OneDrive folder. Include the SIMPL `.d3dpipeline` files used for legacy comparison (both v1 and v2-advanced shapes).

## Recommended Deviation entries (proposed, pending legacy comparison)

> **Deviation ID:** `MultiThresholdObjects-D1`
> **Filter UUID:** `4246245e-1011-4add-8436-0af6bed19228`
> **Symptom:** Default output mask `DataType` differs across SIMPLNX versions and from one historical SIMPLNX baseline. Pre-PR-#1502 SIMPLNX defaulted to `DataType::boolean`; post-PR-#1502 (merged 2026-01-15) SIMPLNX defaults to `DataType::uint8`. Legacy SIMPL `MultiThresholdObjects` historically emits a `uint8` mask.
> **Root cause:** The SIMPLNX default was changed to better match legacy and to avoid downstream filters that assume 0/1 numeric mask values choking on the boolean dtype.
> **Affected users:** Pipelines authored against pre-#1502 SIMPLNX that *relied on* the `boolean` default (rare) will now produce `uint8` masks. Pipelines that explicitly set `created_mask_type` are unaffected.
> **Recommendation:** Trust the new SIMPLNX default (uint8) — it matches legacy SIMPL output and downstream-filter expectations.
> **Status:** Proposed — confirm by running both pre-#1502 and post-#1502 SIMPLNX against legacy on a pipeline that omits the parameter.

> **Deviation ID:** `MultiThresholdObjects-D2`
> **Filter UUID:** `4246245e-1011-4add-8436-0af6bed19228`
> **Symptom:** SIMPL 6.4 pipelines stored with `Filter_Name = "MultiThresholdObjects2"` (no `Filter_Uuid` field) were silently routed through the v1 `ComparisonSelectionFilterParameterConverter` instead of the v2-advanced one. This produced converted Arguments missing nested-set comparison structure.
> **Root cause:** Pre-PR-#1605 `FromSIMPLJson()` only checked `Filter_Uuid`; if absent (as in 6.4 JSON), `isAdvanced` defaulted to false and v2-advanced content was lost. Fixed in PR #1605 by adding a `Filter_Name` fallback that detects the legacy v2 class name.
> **Affected users:** Anyone with SIMPL 6.4-era pipeline JSON that uses the advanced (nested-set) thresholding form.
> **Recommendation:** Trust SIMPLNX post-#1605. Pre-#1605 SIMPLNX was wrong on this conversion; legacy SIMPL was correct. No legacy patch needed.
> **Status:** Proposed — confirmed by the test added in PR #1605 (the un-stubbed `CHECK` on `k_CreatedDataName_Key`).

> **Deviation ID:** `MultiThresholdObjects-D3` *(speculative — pending algorithm review)*
> **Filter UUID:** `4246245e-1011-4add-8436-0af6bed19228`
> **Symptom:** Possible incorrect handling of `inverse` (i.e. `ArrayThresholdSet::isInverted()`) on nested threshold sets. The implementation in `ThresholdSet`/`ThresholdValue` performs `std::reverse(tempResultVector.begin(), tempResultVector.end())` when `replaceInput && inverse` is true (lines ~138 and ~194 of `MultiThresholdObjects.cpp`), which reverses tuple ordering rather than performing a boolean NOT. If a user has a top-level inverted set with a single child threshold, the resulting mask may be the *reversed* mask rather than the *inverted* mask.
> **Root cause:** Suspected port artifact — the legacy SIMPL code may have used `std::reverse` to invert a `std::vector<bool>` representation that this port semantics doesn't preserve.
> **Affected users:** Anyone using inverted top-level `ArrayThresholdSet`s.
> **Recommendation:** Pending algorithm review (see Gap 5). If confirmed, this is a SIMPLNX-side bug to fix. Tests today do not appear to exercise `isInverted()` with `firstValueFound == false`.
> **Status:** Speculative — must be confirmed by reading `ArrayThreshold.hpp` semantics, hand-walking the recursion, and adding a targeted test before promoting to a real Deviation.

> **Deviation ID:** `MultiThresholdObjects-D4` *(NaN/inf handling)*
> **Filter UUID:** `4246245e-1011-4add-8436-0af6bed19228`
> **Symptom:** `std::less<>`, `std::greater<>`, `std::equal_to<>` against IEEE-754 NaN values always return false (per the C++ standard), so any voxel where the input array contains NaN will always be classified FALSE regardless of which comparison operator is selected — including `Operator_NotEqual`. (NaN != NaN evaluates true, so `not_equal_to` would actually return true for NaN; the asymmetry between operators on NaN is itself a Deviation surface.)
> **Root cause:** Inherent to the C++ STL functor approach. Legacy SIMPL likely has the same behavior, but worth pinning.
> **Affected users:** Anyone thresholding `float32`/`float64` arrays that may contain NaN (uninitialized data, masked-out voxels marked NaN, etc.).
> **Recommendation:** Confirm legacy SIMPL behavior on NaN matches; document the asymmetric treatment in the filter documentation. No code change recommended.
> **Status:** Proposed — pending NaN test and legacy comparison.
