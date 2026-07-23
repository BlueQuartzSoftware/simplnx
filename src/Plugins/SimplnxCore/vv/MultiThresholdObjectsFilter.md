# V&V Report: MultiThresholdObjectsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `4246245e-1011-4add-8436-0af6bed19228` |
| DREAM3D 6.5.171 equivalent | Two separate legacy filters, consolidated: **Threshold Objects** (`MultiThresholdObjects`, SIMPL UUID `014b7300-cf36-5ede-a751-5faf9b119dae`) and **Threshold Objects (Advanced)** (`MultiThresholdObjects2`, SIMPL UUID `686d5393-2b02-5c86-b887-dd81a8ae80f2`) — both mapped to this single filter's UUID in `SimplnxCoreLegacyUUIDMapping.hpp` (see Algorithm Relationship) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *pending — DRAFT, not yet reviewed* |

## At a glance

| Aspect                 | Current state                                                                                                                |
|------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Rewrite.** Consolidates two independently-shipped legacy filters — **Threshold Objects** (flat, AND-only) and **Threshold Objects (Advanced)** (nested AND/OR sets) — into one SIMPLNX filter under one new UUID, unified around a single `ArrayThresholdSet` model. Not a line-by-line translation of either legacy source. |
| Oracle (confirmed)     | **Class 1 (Analytical) — confirmed.** `expected[i] = COMPARISON(input[i], value)`, hand-combined via AND/OR/invert boolean algebra. Encoded as 9 `TEST_CASE` groups (17 ctest entries) in `MultiThresholdObjectsTest.cpp`, all pass. |
| Code paths enumerated  | **23 of 24 exercised.** Only the unreachable comparison-operator `else`-throw is untested (all four `ComparisonType` enumerators are covered elsewhere). |
| Tests today            | **9 `TEST_CASE` groups / 17 ctest entries.** Exhaustive sweeps over comparison operator × invert × union operator × set nesting × mask `DataType` (11 types) × source-array `DataType` (11 types), plus 4 negative/error-path groups. All fixtures built in-memory. |
| Exemplar archive       | **None.** All fixtures are constructed in-memory by `CreateTestDataStructure()` / `CreateTestDataStructure2()`; no `.dream3d` exemplar or `download_test_data()` entry exists for this filter. |
| Legacy comparison      | **Not run** — no local build of legacy DREAM3D 6.5.171 (or its Advanced comparison-set variant) is available in this environment. Deferred; see `vv/deviations/MultiThresholdObjectsFilter.md`. |
| Bug flags              | None identified. Provisional — no legacy comparison has been run yet. |
| V&V phase              | Oracle chosen and applied (Class 1), code paths enumerated (23/24), test inventory documented. **Outstanding:** legacy 6.5.171 runtime A/B comparison, second-engineer oracle review, status promotion DRAFT → READY FOR REVIEW. |

For worked instances see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md` and `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md` (on `topic/vv/compute_avg_caxis`).

## Summary

`MultiThresholdObjectsFilter` builds a typed mask array by elementwise-comparing one or more input arrays against user-supplied thresholds, combined through an arbitrarily-nested tree of AND/OR/invert `ArrayThresholdSet`s. Verification uses a **Class 1 (Analytical) oracle**: every comparison operator, invert flag, union operator, nesting depth, and both the mask-output and source-input `DataType` are exhaustively hand-derived and asserted in `MultiThresholdObjectsTest.cpp` (9 `TEST_CASE` groups, all passing). 23 of 24 algorithm/preflight code paths are exercised; the filter has not yet been diffed against legacy DREAM3D 6.5.171.

## Algorithm Relationship

*Classification:* **Rewrite**

*Evidence:* `SimplnxCoreLegacyUUIDMapping.hpp` maps **two distinct legacy SIMPL UUIDs** to this single SIMPLNX filter's UUID:

```
014b7300-cf36-5ede-a751-5faf9b119dae → MultiThresholdObjectsFilter  // MultiThresholdObjects       ("Threshold Objects")
686d5393-2b02-5c86-b887-dd81a8ae80f2 → MultiThresholdObjectsFilter  // MultiThresholdObjects2      ("Threshold Objects (Advanced)")
```

`FromSIMPLJson()` correspondingly branches on which legacy UUID (or, for 6.4 pipelines lacking a UUID, which legacy class name) produced the incoming JSON: the basic `MultiThresholdObjects` source is read through `ComparisonSelectionFilterParameterConverter` (flat, AND-only comparison list) and the advanced `MultiThresholdObjects2` source is read through `ComparisonSelectionAdvancedFilterParameterConverter` (nested AND/OR comparison sets). Both are converted into the same `ArrayThresholdSet` argument. This is **not** a line-by-line port of a single legacy algorithm — it's a consolidation of two independently-shipped legacy filters into one, which is why the classification is **Rewrite** rather than Port, per `vv_policy.md`: *"keeping [a UUID relationship] is a claim of functional equivalence... The Deviations file must defend the claim."* Here the claim is stronger than usual — that the merged filter reproduces each of the two legacy filters' behavior when configured equivalently to it. SIMPL 6.4/6.5 conversion fixtures exist at `test/simpl_conversion/6_4/MultiThresholdObjectsFilter.json` and `test/simpl_conversion/6_5/MultiThresholdObjectsFilter.json`, but these only assert the *argument conversion* round-trips correctly, not that execution output matches either legacy filter — that's the outstanding legacy A/B (see Deviations file).

*Structural differences from each legacy source (none independently confirmed by a legacy A/B yet):*

1. **Consolidation itself** — one `ArrayThresholdSet` tree replaces two separate legacy parameter models (flat list vs. nested set); the flat legacy model is representable as a one-level `ArrayThresholdSet`, but this equivalence has not been verified end-to-end against either legacy filter's actual output.
2. Multi-component index selection added (`#1184`, `32837a30f`) — not present in the legacy `Threshold Objects` filter (scalar-only comparison); unclear whether `Threshold Objects (Advanced)` had an equivalent.
3. Custom TRUE/FALSE mask output values added (`#669`, `b65210cf3`) — additive parameter; unconfirmed whether either legacy filter had this option or if it's SIMPLNX-only.
4. Default mask output `DataType` changed to `uint8` (`#1502`, `49919b086`) — unconfirmed against either legacy filter's default.
5. `executeImpl()` body moved into `Algorithms/MultiThresholdObjects.{hpp,cpp}` (`#1544`, `8381d1dd5`) — structural only, no behavior change (internal to SIMPLNX, not a legacy-relationship concern).

*Material PRs since baseline (2025-10-01):* none identified beyond the deltas above and this branch's `vv/MultiThresholdObjects` restructuring + test work.

## Oracle

*Class:* **1 (Analytical)**

*Applied:* For a single threshold, `expected[i] = COMPARISON(input[i], value)` (optionally inverted); for a component-indexed array, `input[i]` is replaced by `input[i][componentIndex]`. For a threshold set, `expected` is the boolean combination of each member's own `expected` value: the first member always seeds the accumulator, and the configured `UnionOperator` (AND/OR) combines each subsequent member; the whole set's `expected` is inverted again if the set itself is marked inverted. Every free variable in this formula — comparison operator, invert flag, union operator, set nesting, component index, mask output `DataType`, and source-array `DataType` — is enumerated directly against this closed-form definition in the test file's `Expected*Mask` helper functions, independent of the algorithm's own C++ control flow (`ThresholdFilterHelper`, `InsertThreshold`, `ApplyThresholdValues`).

*Encoded:* `test/MultiThresholdObjectsTest.cpp` —

- `Valid Single Thresholds: Int` / `: Float` / `: Int Multi-Component` — comparison operator × invert × (component index for multi-component) sweep, via `ExpectedIntSingleComponentMask` / `ExpectedFloatSingleComponentMask` / `ExpectedIntMultiComponentMask`
- `Valid Threshold Sets` — 5 hand-built AND / OR / nested-set / nested-set-with-OR / nested-set-with-OR+invert configurations (`CreateThresholdSet1`–`5`), via `ExpectedThresholdSet1Mask`–`5`
- `Valid Execution, Mask DataType` — 11 mask-output `DataType`s
- `Valid Execution, Input Array DataType` — 11 source-array `DataType`s
- `Invalid Execution`, `Invalid Execution - Out of Bounds Custom Values` (9 numeric types), `Invalid Execution - Boolean Custom Values` — negative-path fixtures

9 `TEST_CASE` groups (17 ctest entries, counting the 9 `TEMPLATE_TEST_CASE` type instantiations separately), all pass at HEAD.

*Second-engineer review:* Skipped — recorded reason: the oracle is elementwise comparison plus boolean set algebra (AND/OR/invert), and the test matrix enumerates it exhaustively (every operator × invert × union operator × nesting × both `DataType` axes) rather than sampling a single hand-derivation, substituting breadth for independent derivation review. **This is not a substitute for a named second-engineer pass** — it is recorded here as an outstanding gate for promotion past DRAFT, not a completed one.

## Code path coverage

**23 of 24 paths exercised.** The one gap is low-risk (row 13).

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.cpp` (~255 lines), plus 7 preflight-only paths in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/MultiThresholdObjectsFilter.cpp`.

Two logical stages: **(a) preflight** validates the threshold set / mask-type / custom-value configuration and stages the output `CreateArrayAction`; **(b) algorithm** recursively evaluates the `ArrayThresholdSet` tree (per-array comparison → per-set union/invert/replace combination) and writes the result into the mask array via a type-dispatched functor.

| #  | Stage             | Path                                                                                                                              | Test case                                                                                                                     |
|----|-------------------|------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------|
| 1  | (a) Preflight     | `thresholdPaths.empty()` → error `-4000`                                                                                          | `Invalid Execution` — "Empty ArrayThresholdSet"                                                                              |
| 2  | (a) Preflight     | Tuple-count mismatch across threshold arrays → `ErrorCodes::UnequalTuples`                                                        | `Invalid Execution` — "Mismatching Tuples in Threshold Arrays"                                                               |
| 3  | (a) Preflight     | `componentIndex >= numComponents` (via `CheckComponentIndicesInThresholds`, recurses into nested sets) → `ErrorCodes::InvalidComponentIndex` | `Invalid Execution` — "Out of Bounds Component Index"                                                                        |
| 4  | (a) Preflight     | `maskArrayType == boolean && useCustomTrueValue` → `CustomTrueWithBoolean`                                                        | `Invalid Execution - Boolean Custom Values` — "Custom True Value"                                                            |
| 5  | (a) Preflight     | `maskArrayType == boolean && useCustomFalseValue` → `CustomFalseWithBoolean`                                                      | `Invalid Execution - Boolean Custom Values` — "Custom False Value"                                                           |
| 6  | (a) Preflight     | `useCustomTrueValue` value outside `[min,max]` of mask type → `CustomTrueOutOfBounds`                                             | `TEMPLATE_TEST_CASE …Out of Bounds Custom Values` — "True Value < Minimum" / "> Maximum", all 9 numeric types                |
| 7  | (a) Preflight     | `useCustomFalseValue` value outside bounds → `CustomFalseOutOfBounds`                                                             | same — "False Value < Minimum" / "> Maximum"                                                                                 |
| 8  | (a) Preflight     | Success → stage `CreateArrayAction` for mask output                                                                               | every "Valid …" test                                                                                                          |
| 9  | (b) Algorithm     | `ComparisonType::LessThan`                                                                                                        | "ArrayThreshold: <" (int / float / multi-component)                                                                          |
| 10 | (b) Algorithm     | `ComparisonType::GreaterThan`                                                                                                     | "ArrayThreshold: >"                                                                                                           |
| 11 | (b) Algorithm     | `ComparisonType::Operator_Equal`                                                                                                  | "ArrayThreshold: =="                                                                                                          |
| 12 | (b) Algorithm     | `ComparisonType::Operator_NotEqual`                                                                                               | "ArrayThreshold: !="                                                                                                          |
| 13 | (b) Algorithm     | `else` → `throw std::runtime_error` (unrecognized comparison type)                                                                | *Not directly tested. Unreachable via the public `ComparisonType` enum — all enumerators are exercised by rows 9–12.*        |
| 14 | (b) Algorithm     | `InsertThreshold` with `inverse == true` (flip before combine)                                                                    | `isInverted = GENERATE(false, true)` in every single-threshold and threshold-set test                                       |
| 15 | (b) Algorithm     | `InsertThreshold` with `inverse == false`                                                                                         | same                                                                                                                          |
| 16 | (b) Algorithm     | Combine with `UnionOperator::Or`                                                                                                  | `CreateThresholdSet2` (threshold2 = Or), `CreateThresholdSet4`/`5` (nested-set union = Or)                                   |
| 17 | (b) Algorithm     | Combine with `UnionOperator::And` (else branch)                                                                                   | `CreateThresholdSet1` (threshold2/3 = And), `CreateThresholdSet3` default nested And                                        |
| 18 | (b) Algorithm     | `ApplyThresholdValues` with `replaceInput == true` (first item in a set forces Or regardless of configured operator)             | implicit in every threshold set — first entry of every `CreateThresholdSet*`                                                |
| 19 | (b) Algorithm     | `ApplyThresholdValues` with `replaceInput == false` (honors configured operator for later items)                                 | same sets, 2nd/3rd entries                                                                                                   |
| 20 | (b) Algorithm     | `ThresholdSet` recursion — item is a nested `ArrayThresholdSet`                                                                   | `CreateThresholdSet3`/`4`/`5` (set-of-sets)                                                                                  |
| 21 | (b) Algorithm     | `ThresholdSet` — item is a leaf `ArrayThreshold`                                                                                  | all tests                                                                                                                     |
| 22 | (b) Algorithm     | `ThresholdSetFunctor` dispatch on **mask (output) DataType**                                                                      | `Valid Execution, Mask DataType` — boolean + int8/16/32/64 + uint8/16/32/64 + float32/64, all 11 types                       |
| 23 | (b) Algorithm     | `ExecuteThresholdHelper` dispatch on **source array's DataType**                                                                  | `Valid Execution, Input Array DataType` — int8/16/32/64 + uint8/16/32/64 + float32/64 + boolean, all 11 types                |
| 24 | (b) Algorithm     | Multi-component `componentIndex != 0` selection                                                                                   | `Valid Single Thresholds: Int Multi-Component` (`componentIndex = GENERATE(0,1,2)`), plus `componentIndex=1` in Set1, `=0` in Set2 |

Not counted as an algorithm/preflight path: the "Empty ArrayThreshold DataPath" section of `Invalid Execution` exercises `ArrayThresholdsParameter`'s own path-existence validation, which runs before `preflightImpl` is called — it's a parameter-layer gate, not code inside this filter or algorithm.

Also note: `k_MismatchingComponentsArrayPath` (test file, line 31) is a leftover unused `DataPath` constant — the array it used to name was removed when `Valid Execution, Input Array DataType` was added. Not a coverage gap (the filter has no cross-array component-count check), just dead test-source code worth deleting.

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Valid Single Thresholds: Int` | kept | `GENERATE` over 8 threshold values × 2 invert states, 4 `SECTION`s (`>`, `<`, `==`, `!=`) against `k_TestArrayIntPath`; every tuple checked via `ExpectedIntSingleComponentMask`. |
| `Valid Single Thresholds: Float` | kept | Same sweep against `k_TestArrayFloatPath` via `ExpectedFloatSingleComponentMask`. |
| `Valid Single Thresholds: Int Multi-Component` | kept | Adds `componentIndex = GENERATE(0,1,2)` against `k_MultiComponentArrayPath`. |
| `Valid Threshold Sets` | kept | 5 `SECTION`s (`ArraySet 1`–`5`) covering AND, OR, nested-set, nested-set-with-OR, and nested-set-with-OR+invert combinations, each × `isInverted`. |
| `Invalid Execution` | kept | 4 `SECTION`s: empty threshold set (`-4000`), empty threshold `DataPath` (parameter-layer validation), out-of-bounds component index (`InvalidComponentIndex`), mismatched tuple counts (`UnequalTuples`). |
| `Invalid Execution - Out of Bounds Custom Values` (`TEMPLATE_TEST_CASE`) | kept | 9 numeric-type instantiations × 4 `SECTION`s (true/false value below minimum / above maximum) — `CustomTrueOutOfBounds` / `CustomFalseOutOfBounds`. |
| `Invalid Execution - Boolean Custom Values` | kept | 2 `SECTION`s — custom TRUE/FALSE value rejected when mask type is `boolean`. |
| `Valid Execution, Mask DataType` | kept | 11 `SECTION`s, one per mask-output `DataType` (int8…float64; boolean covered via the default mask type used throughout the other `TEST_CASE`s). |
| `Valid Execution, Input Array DataType` | new-for-V&V (`d18b0f34d`, 2026-07-23) | 11 `SECTION`s, one per **source-array** `DataType` (int8…float64, bool) — closes the code-path gap on row 23 identified during path enumeration. |

## Exemplar archive

None. All fixtures for this filter are constructed in-memory in `test/MultiThresholdObjectsTest.cpp` (`CreateTestDataStructure()`, `CreateTestDataStructure2()`); there is no `.dream3d` exemplar and no `download_test_data()` entry in `test/CMakeLists.txt` for `MultiThresholdObjectsFilter`. No provenance sidecar is needed.

## Deviations from DREAM3D 6.5.171

- Legacy comparison has **not been run**. 0 deviation entries exist. See `vv/deviations/MultiThresholdObjectsFilter.md` for the reason and the configurations to prioritize once a legacy build is available.
