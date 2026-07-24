# V&V Report: CopyFeatureArrayToElementArrayFilter

|                             |                                                                                                      |
|-----------------------------|------------------------------------------------------------------------------------------------------|
| Plugin                      | SimplnxCore                                                                                          |
| SIMPLNX UUID                | `4c8c976a-993d-438b-bd8e-99f71114b9a1`                                                               |
| SIMPLNX Human Name          | Create Element Array from Feature Array                                                              |
| DREAM3D 6.5.171 equivalent  | `CopyFeatureArrayToElementArray` (SIMPLib CoreFilters) — SIMPL UUID `99836b75-144b-5126-b261-b411133b5e8a` |
| Verified commit             | *<filled at SBIR deliverable assembly>*                                                              |
| Status                      | COMPLETE |
| Sign-off                    | *pending second-engineer review* |

## At a glance

| Aspect                | Current state |
|-----------------------|---------------|
| Algorithm Relationship | **Minor changes** — same indirection-copy kernel as SIMPL `CopyFeatureArrayToElementArray`; NX deliberately adds multi-array selection, suffix-based output naming, TBB parallelization, and different feature-count validation semantics. |
| Oracle (confirmed)    | **Class 1 (Analytical)** — pure indirection lookup `out[i*C+c] = feature[featureIds[i]*C+c]`; hand-derived expected values on a 4×3×1 fixture (float32/1-comp, int32/3-comp, bool). **Class 4 (Invariant)** companion — piecewise constancy within each feature. Encoded as `CopyFeatureArrayToElementArrayTest.cpp::"Analytical Oracle (Class 1)"`; all pass. |
| Code paths enumerated | 13 of 14 exercised; the uncovered path is the cancel check (excluded by engineer instruction — requires cancel-signal injection). Path 14 (virtual-store kernel fallback) is only partially covered pending the OOC-backend gap noted in *Tests today*. |
| Tests today           | 21 ctest entries (12 test cases, one a 10-type `TEMPLATE_LIST`): 1 analytical-oracle, 7 error/negative, 1 degenerate no-op, 1 deviation-pin, 10 type-dispatch instantiations, 1 SIMPL backwards-compat (2 `DYNAMIC_SECTION`s). All pass in both `simplnx-Rel` and `simplnx-ooc-Rel`, 2026-07-23. **OOC caveat:** the `simplnx-ooc-Rel` build sets `SIMPLNX_FORCE_OUT_OF_CORE_DATA=ON` but registers no OOC backend (`SIMPLNX_EXTRA_PLUGINS=FileStore` with an empty `SIMPLNX_FileStore_SOURCE_DIR`), so `useOocData()` is false and those runs execute in-core; that pass certifies compile + run under the OOC configuration, **not** OOC data-path behavior. Applies to every filter tested from this build dir. |
| Exemplar archive      | None — all fixtures are in-memory `AnalyticalFixtures` built in test code; no `download_test_data()` archive required. |
| Legacy comparison     | **Run 2026-07-23** (re-run after the kernel fast-path change). Bit-identical numeric output on the main fixture (float32, int32×3, bool). 3 deviations, all naming/validation semantics: D1 (output naming for converted pipelines), D2 (over-provisioned feature array accepted in NX, error -5555 in legacy), D3 (negative ids: silent out-of-bounds garbage in legacy, hard error -5355 in NX). |
| Bug flags             | `CopyFeatureArrayToElementArrayFilter-D3` — legacy 6.5.171 silently produces undefined values for negative feature ids (unchecked out-of-bounds read). SIMPLNX behavior is correct. |
| V&V phase             | Discovery, algorithm relationship, oracle design + reconciliation, algorithm review (fixes applied and re-verified), unit tests, legacy comparison, deviations, documentation — **complete**. Outstanding: second-engineer review of the oracle design and this report (requested at PR review). The OOC-build backend gap noted in *Tests today* needs a build-infrastructure decision and is tracked outside this report. |

## Summary

`CopyFeatureArrayToElementArrayFilter` copies each selected Feature-level array down to the Element (cell) level: for every cell `i`, the output tuple is the source array's tuple at index `featureIds[i]`. Verification used a Class 1 analytical oracle (hand-derived expected values for a 12-cell, 4-feature fixture across single- and multi-component arrays and three data types) plus Class 4 piecewise-constancy invariants; SIMPLNX matched the oracle exactly with zero discrepancies. A/B comparison against DREAM3D 6.5.171 was bit-identical on valid input, with 3 documented deviations in naming/validation semantics — including one legacy bug (silent undefined output for negative feature ids, D3).

## Algorithm Relationship

*Classification:* **Minor changes**

*Evidence:* SIMPL UUID `99836b75-144b-5126-b261-b411133b5e8a` is registered for conversion via `FromSIMPLJson()` (fixtures at `test/simpl_conversion/6_5/` and `6_4/`). The copy kernel is semantically identical to legacy `copyData<T>()` (SIMPL `CopyFeatureArrayToElementArray.cpp:159-191`) — a per-tuple indirection copy. The surrounding filter deliberately changed.

*Port-time deltas:*

1. **Multi-array selection** — legacy selects ONE feature array per filter instance; NX takes a `MultiArraySelectionParameter` and loops. Does not change per-array output values.
2. **Output naming** — legacy takes an explicit created-array *name*; NX builds the name as `<sourceArrayName><suffix>`. Same numeric output, different output DataPath for converted pipelines (see Deviations D1).
3. **Feature-count validation relaxed** — legacy `execute()` errors (-5555) BOTH when `maxFeatureId >= numFeatures` AND when the feature array is over-provisioned (`maxFeatureId != numFeatures-1`). NX (`ValidateFeatureIdsToFeatureAttributeMatrixIndexing`, `ignoreNegativeValues=false`) errors only when `maxFeatureId >= numFeatures` (-5351); an over-sized feature array is accepted (see Deviations D2).
4. **Negative feature ids** — legacy performs an unchecked negative index into the feature array (undefined behavior / garbage read); NX errors with -5355 (see Deviations D3).
5. **Parallelization and kernel form** — legacy is a serial per-tuple `memcpy`; NX runs `ParallelDataAlgorithm` over cell tuples with two kernel forms: a raw-pointer `std::copy_n` path taken when all three stores are concrete in-core `DataStore<T>` (each thread writes a disjoint index range of a plain buffer), and a virtual `AbstractDataStore` per-component fallback for any other store type — including out-of-core, where `IParallelAlgorithm`/`requireArraysInMemory()` runs the range serially. Writes are element-wise independent with no accumulation, so neither form has an order-of-operations effect on output.
6. **Type dispatch** — legacy if/else `CanDynamicCast` chain over 11 types (bool + 8 int + 2 float); NX `ExecuteParallelFunction` with `ArrayUseAllTypes` over the same 11 types. No behavioral difference.
7. **Tuple-count precheck (new in NX)** — preflight requires all selected feature arrays to share a tuple count (error -3020). Legacy has no equivalent because it only ever operates on one array.

*Material PRs since baseline:* #1644 (added -3020 preflight test), #1588 (SIMPL backwards-compat test redesign), #1544 (executeImpl → Algorithm class extraction). None changed the copy kernel.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion. Classes 2/3/5 N/A — no external library or paper needed for an indirection lookup (this filter is the literal "Indirection lookups" example in `oracle_classes.md`).

*Applied:* Hand-built 4×3×1 `ImageGeom` fixture, 12 cells, 4 features. `FeatureIds = [0,1,1,2, 2,0,3,1, 3,3,0,2]`. Feature arrays: `AvgTemp` (float32, 1 comp) `= [10.5, 20.25, -30.75, 40.125]` and `RGB` (int32, 3 comp) `= [(1,2,3), (40,50,60), (-7,8,-9), (100,200,127)]`. Expected cell outputs are derived by hand (spreadsheet-free — 12 lookups) and embedded as inline constants with derivation comments:

- `AvgTemp_Cell = [10.5, 20.25, 20.25, -30.75, -30.75, 10.5, 40.125, 20.25, 40.125, 40.125, 10.5, -30.75]`
- `RGB_Cell` tuple `i` = RGB tuple `FeatureIds[i]`, e.g. cell 3 (fid 2) = `(-7,8,-9)`; cell 6 (fid 3) = `(100,200,127)`.

Class 4 companion invariants: (a) every pair of cells with the same feature id has identical output tuples; (b) each output tuple equals the source feature tuple exactly (bit-identical, no arithmetic performed).

*Encoded:* `test/CopyFeatureArrayToElementArrayTest.cpp::"Analytical Oracle (Class 1)"` — 12+36+12 element-wise Class 1 assertions (AvgTemp_Cell, RGB_Cell, Active_Cell) with the derivation embedded as comments, plus the Class 4 piecewise-constancy invariant loop. All pass in both builds. A secondary hand-derived fixture lives in `"Over-provisioned Feature array accepted"` (6 assertions).

*Second-engineer review:* pending — requested as part of PR review of the test changes.

## Code path coverage

*13 of 14 paths exercised; cancel path excluded by engineer instruction.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArray.cpp` (138 lines) + preflight in `Filters/CopyFeatureArrayToElementArrayFilter.cpp` (165 lines).

Logical phases: (a) parameter/preflight validation + output-array creation, (b) execute-time validation, (c) type-dispatched copy kernel (raw-pointer fast path for in-core `DataStore<T>`, virtual `AbstractDataStore` fallback otherwise).

| #  | Phase         | Path                                                                                     | Test case |
|----|---------------|-------------------------------------------------------------------------------------------|-----------|
| 1  | (a) Preflight | empty selection list → error `k_Validate_Empty_Value` (filter's own guard, preflight AND execute) | `Preflight Error - Empty selection (filter guard)` — FeatureIds path is valid so parameter validation passes and the filter guard is what fires |
| 2  | (a) Preflight | non-`IDataArray` selection (NeighborList/StringArray) → rejected by parameter validation. The `ArrayType::DataArray` constraint is what rejects it; `MultiArraySelectionParameter::AllowedDataTypes` is stored but not enforced by the framework | `Preflight Error - Non-DataArray selection rejected` |
| 3  | (a) Preflight | suffix contains `/` → error -3021                                                         | `Preflight Error - Suffix contains '/' (-3021)` |
| 4  | (a) Preflight | selected feature arrays disagree on tuple count → error -3020                             | `Preflight Error - Feature array tuple count mismatch (-3020)` |
| 5  | (a) Preflight | valid → one `CreateArrayAction` per selected array (tuple shape from FeatureIds, component shape from source, name `<source><suffix>`) | `Analytical Oracle (Class 1)` — 3 arrays created in the Cell AM |
| 6  | (a) Actions   | created path collides with an existing object → error -266 at action application (execute in unit tests; pipeline preflight in the GUI); original array untouched | `Execute Error - Created name collides with existing array (-266)` |
| 7  | (b) Execute   | negative feature id → error -5355 (`ValidateFeatureIdsToFeatureAttributeMatrixIndexing`, called once — preflight -3020 guarantees equal tuple counts across selected arrays) | `Execute Error - Negative FeatureIds (-5355)` |
| 8  | (b) Execute   | `maxFeatureId >= numFeatures` → error -5351                                               | `Execute Error - FeatureId exceeds Feature tuple count (-5351)` |
| 9  | (b) Execute   | over-provisioned feature array (`numFeatures > maxId+1`) → accepted (deviation D2 pin)    | `Over-provisioned Feature array accepted` |
| 10 | (b) Execute   | zero-tuple FeatureIds → valid no-op (validator's empty guard), empty outputs              | `Zero-tuple FeatureIds accepted` |
| 11 | (b) Execute   | cancel check (per-array loop + inside both kernel paths)                                  | *Not directly tested. Excluded by engineer instruction — requires cancel-signal injection.* |
| 12 | (c) Kernel    | 11-way type dispatch (bool + 8 int + 2 float)                                             | TEMPLATE_LIST `Valid filter execution` (10 numeric); `Analytical Oracle (Class 1)` (bool, float32, int32) |
| 13 | (c) Kernel    | raw-pointer fast path (all three stores are in-core `DataStore<T>`) + multi-component copy (`C > 1`) | `Analytical Oracle (Class 1)` (in-core build) — RGB 3-comp; all in-core tests take this path |
| 14 | (c) Kernel    | virtual `AbstractDataStore` fallback (non-`DataStore<T>` stores, e.g. out-of-core)        | *Partially covered.* The fallback is the same per-component indirection copy as the fast path and is compiled and instantiated by every test, but no test currently supplies a non-`DataStore<T>` store — that requires a build with an OOC backend registered (see the OOC caveat in At a glance). |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `Preflight Error - Empty selection (filter guard)` | new-for-V&V | Supplies a valid FeatureIds array so the filter's own empty-selection guard is what fires, in both preflight and execute. Replaces `Parameter Check` (see the retired row below). |
| `Preflight Error - Feature array tuple count mismatch (-3020)` | kept | Two feature arrays with 3 vs 4 tuples → -3020. |
| `Preflight Error - Non-DataArray selection rejected` | new-for-V&V | Pins the `ArrayType::DataArray` parameter constraint: selecting a `NeighborList` previously threw an uncaught `std::bad_cast` out of preflight. |
| `Preflight Error - Suffix contains '/' (-3021)` | new-for-V&V | Pins the -3021 suffix validation: a suffix containing `/` previously threw an uncaught `std::invalid_argument` from `DataObject` name validation. |
| `Execute Error - Created name collides with existing array (-266)` | new-for-V&V | Empty suffix + self-selection → -266 when the output actions are applied (execute in unit tests, pipeline preflight in the GUI, since `IFilter::preflight` does not apply actions); asserts the source array is untouched (no silent overwrite). |
| `Analytical Oracle (Class 1)` | new-for-V&V | ImageGeom fixture; 60 element-wise Class 1 assertions vs hand-derived constants (float32/1, int32/3, bool/1) + Class 4 piecewise-constancy loop + `CheckArraysInheritTupleDims`. |
| `Execute Error - Negative FeatureIds (-5355)` | new-for-V&V | Passes preflight, fails execute; pins deviation D3's SIMPLNX side. |
| `Execute Error - FeatureId exceeds Feature tuple count (-5351)` | new-for-V&V | id 4 vs 4-tuple feature array. |
| `Over-provisioned Feature array accepted` | new-for-V&V | 8-tuple feature array, max id 2 → success + 6 hand-derived value assertions; pins deviation D2. |
| `Zero-tuple FeatureIds accepted` | new-for-V&V | Degenerate no-op case; also pins the empty-store guard in `ValidateFeatureIdsToFeatureAttributeMatrixIndexing`, which previously dereferenced `end()` on a zero-tuple FeatureIds array (UB). |
| `Valid filter execution` (TEMPLATE_LIST ×10 types) | kept (modified) | Was comparing never-initialized feature data (indeterminate values) and all-zero temperature data — an indexing bug could not have been detected. Now initialized with distinct per-feature values `[5,15,25]` / `[1,4,7]`. Added `REQUIRE_NOTHROW`, `CAPTURE(i)`, `CheckArraysInheritTupleDims`. |
| `SIMPL Backwards Compatibility` (2 DYNAMIC_SECTIONs) | kept (modified) | UUID + argument conversion round-trip for 6.4 and 6.5 fixtures; now also asserts the converted multi-path selection value (was previously unasserted). |
| `Parameter Check` | retired | Its empty-selection assertion supplied an empty FeatureIds path, which failed *parameter* validation before `preflightImpl()` ran, so the filter's own empty-selection guard was never reached. Replaced by `Preflight Error - Empty selection (filter guard)`. |

All 21 ctest entries pass in both `simplnx-Rel` (in-core) and `simplnx-ooc-Rel` builds, 2026-07-23. Per the OOC caveat in *At a glance → Tests today*, the `simplnx-ooc-Rel` pass certifies compile + run under the OOC configuration, not OOC data-path behavior.

## Exemplar archive

- **Archive:** None. All fixtures are built in-memory in the test file (`AnalyticalFixtures`); no exemplar `.dream3d`/`download_test_data()` entry exists or is needed for this filter.
- **Provenance:** N/A (no archive). The A/B comparison inputs are minted by `make_input.py` in the comparison bundle (OneDrive archive), reproducible from the checked-in oracle constants.

## Deviations from DREAM3D 6.5.171

Comparison run 2026-07-23 on the analytical fixture (main case) plus two targeted probes. **Numeric output bit-identical on valid input** (float32/1-comp, int32/3-comp, bool arrays).

- `CopyFeatureArrayToElementArrayFilter-D1` — converted legacy pipelines produce differently *named* output arrays (`<source><suffix>` vs explicit legacy name) — see `vv/deviations/CopyFeatureArrayToElementArrayFilter.md`
- `CopyFeatureArrayToElementArrayFilter-D2` — over-provisioned feature array errors in 6.5.171 (-5555) but succeeds in SIMPLNX — see `vv/deviations/CopyFeatureArrayToElementArrayFilter.md`
- `CopyFeatureArrayToElementArrayFilter-D3` — **legacy bug**: negative feature ids silently produce undefined values in 6.5.171 (unchecked out-of-bounds read); SIMPLNX errors -5355 — see `vv/deviations/CopyFeatureArrayToElementArrayFilter.md`
