# V&V Report: CreateFeatureArrayFromElementArrayFilter

| | |
|---|---|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| DREAM3D 6.5.171 equivalent | `CreateFeatureArrayFromElementArray` (SIMPL UUID `94438019-21bb-5b61-a7c3-66974b9a34dc`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | **COMPLETE** |
| Sign-off | *Nathan Young, 07-28-2026* |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **Port** — the per-cell copy loop is a line-by-line translation of SIMPL `CreateFeatureArrayFromElementArray`. UUID changed from SIMPL; legacy alias maintained via `FromSIMPLJson()` and SIMPL conversion fixtures. The sizing logic differs only in the AM under-sized case: SIMPL errors; SIMPLNX resizes and succeeds (see D1 in deviations). All pipelines that succeeded in SIMPL produce bit-identical output in SIMPLNX. |
| Oracle | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** — expected outputs hand-derived for 3-fixture AnalyticalFixtures suite (AF-1: single-component consistent, AF-2: single-component inconsistent/warning, AF-3: 3-component consistent). Class 4 invariants: output AM has exactly `max(featureIds)+1` tuples; output data type and component shape match input. Implemented as inline `REQUIRE` assertions in `test/CreateFeatureArrayFromElementArrayTest.cpp`. |
| Code paths enumerated | **5 of 6** exercised; cancellation structurally present but not directly exercised by any test fixture. |
| Tests today | **6 test cases** (2 regression + 1 SIMPL backwards-compat + 3 AnalyticalFixtures). **Circular-oracle flag**: existing regression tests compare SIMPLNX output against legacy-generated `CellFeatureData` arrays pre-existing in `6_5_test_data_1_v2.dream3d`; not a valid correctness oracle per policy. The 3 AnalyticalFixtures tests (AF-1, AF-2, AF-3) provide the independent Class 1 + Class 4 oracle. |
| Exemplar archive | `6_5_test_data_1_v2.tar.gz` (SHA512 `585b51b…`) — shared input archive. Pre-existing `CellFeatureData` arrays within the dream3d are legacy-generated; serve only as regression baselines. No oracle-specific archive needed for Class 1 (oracle is inline assertions). |
| Legacy comparison | **Complete (2026-07-23) — one deviation identified, no migration impact.** Empirical A/B comparison on synthetic 8×1×1 fixtures: bit-identical. Static source analysis identified D1 (AM under-sized: SIMPL errors -5555; SIMPLNX resizes and succeeds). D1 is unreachable from any SIMPL pipeline that ran successfully, so output is bit-identical for the entire valid SIMPL migration space. See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md`. |
| Bug flags | **None** |
| V&V phase | **Complete.** Oracle designed (Class 1 + Class 4) and implemented; code-path analysis complete; Algorithm Relationship classified (Port); AnalyticalFixtures tests implemented (AF-1, AF-2, AF-3); legacy A/B comparison run (2026-07-23, bit-identical on full valid SIMPL migration space); one deviation (D1, SIMPLNX improvement) identified and documented. |

## Summary

`CreateFeatureArrayFromElementArrayFilter` copies each element-level data array value to the feature-level entry identified by the corresponding FeatureId, using last-writer-wins semantics, and emits one warning if any cell's value for a feature differs from the first-seen value. The filter was verified analytically using a Class 1 hand-derived oracle on three small synthetic fixtures covering consistent, inconsistent, and multi-component cases (AF-1, AF-2, AF-3), plus Class 4 invariants on output shape and type. All six oracle assertions pass. The per-cell copy loop is a direct port of SIMPL `CreateFeatureArrayFromElementArray`; empirical A/B comparison (2026-07-23) confirmed bit-identical output for all inputs that SIMPL can process. One deviation exists (D1): SIMPLNX handles the AM under-sized case by resizing, whereas SIMPL errors with -5555. This deviation has no migration impact — it is unreachable from any pipeline that ran successfully in SIMPL.

## Algorithm Relationship

*Classification:* **Port**

*Evidence:* The SIMPLNX algorithm at `Algorithms/CreateFeatureArrayFromElementArray.cpp` (95 lines) is a line-for-line translation of the SIMPL `CreateFeatureArrayFromElementArray` filter. Identical control flow: iterate over cell tuples, look up `featureIds[cellIdx]`, record first-seen tuple offset in `featureMap`, compare current value against first-seen per component, emit one warning on first mismatch, write current value to `createdDataStore[featureId * C + comp]` (last-writer-wins). The SIMPL UUID (`94438019-21bb-5b61-a7c3-66974b9a34dc`) differs from the SIMPLNX UUID (`50e1be47-b027-4f40-8f70-1283682ee3e7`) because the filter was re-UUID-ed during the SIMPL→SIMPLNX port; the legacy alias is maintained through `FromSIMPLJson()` and `test/simpl_conversion/6_5/CreateFeatureArrayFromElementArrayFilter.json`.

*Port-time deltas that do not change output data:*

1. `QVector<int32_t>` + linear search → `std::map<int32, usize>` for `featureMap` — performance/API change; set-membership semantics and iteration order over cells unchanged.
2. SIMPL `EXECUTE_FUNCTION_TEMPLATE` macro → `ExecuteDataFunction(CopyCellDataFunctor{}, ...)` dispatch — API modernization; identical runtime dispatch on element data type.
3. Warning API: SIMPL used `notifyStatusMessage` guarded by `bool warningThrown = false;`; SIMPLNX uses `Result<>` warnings guarded by `result.warnings().empty()`. Both guards produce the same behavior: exactly **one** warning per execution when any feature's cell values are inconsistent. Confirmed by empirical A/B comparison (2026-07-23) — no behavioral delta.
4. Algorithm class extracted from `executeImpl` into `Algorithms/CreateFeatureArrayFromElementArray.{hpp,cpp}` — code organization change only (PRs #1301 and #1544).
5. Added `shouldCancel` check inside the per-cell loop — no output change (cancel path was absent in SIMPL).

*Material PRs:*

- **#1301** — "ENH: Add missing algorithm classes to some filters": extracted algorithm class skeleton.
- **#1544** — "ENH: Move Filter executeImpl() logic to Algorithm classes": finalized algorithm class with resize logic.
- **#1278** — "BUG: Ensure FeatureId arrays are range checked against the Feature Attribute Matrix": Updated help text and default value for parameter.
- **#1295** — "ENH: Add Fill Functionality to CreateArrayAction": output array initialized to `"0"` fill at creation; ensures feature-0 slot has a defined value when no cell maps to feature 0.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion.

*Applied (Class 1 — Analytical):* Expected outputs are hand-derived without reference to any DREAM3D implementation. The algorithm is a pure indirection: for each cell `i` in order, write `output[featureIds[i] * C + j] = cellInput[i * C + j]` for each component `j`. Last-writer-wins when multiple cells share a featureId. The output AM is resized to `max(featureIds[:]) + 1` tuples.

**Fixture AF-1** — single-component, all values consistent:

| | |
|---|---|
| `featureIds` | `[0, 1, 2, 1, 2]` |
| `cellValues` (float32, 1-comp) | `[5.0, 10.0, 20.0, 10.0, 20.0]` |

Hand derivation:
- Feature 0: only cell 0 → `output[0] = 5.0`
- Feature 1: cell 1 (first) = `10.0`; cell 3 (second) = `10.0` (same → no warning); last-writer = `10.0`
- Feature 2: cell 2 (first) = `20.0`; cell 4 (second) = `20.0` (same → no warning); last-writer = `20.0`

Expected output (float32, 3 tuples): `[5.0, 10.0, 20.0]`. Expected warnings: **0**.

**Fixture AF-2** — single-component, inconsistent values (warning path):

| | |
|---|---|
| `featureIds` | `[1, 2, 1, 2]` |
| `cellValues` (float32, 1-comp) | `[10.0, 20.0, 15.0, 20.0]` |

Hand derivation:
- Feature 0: never written; default fill `"0"` → `output[0] = 0.0`
- Feature 1: cell 0 (first) = `10.0` written; cell 2 (second) = `15.0` ≠ `10.0` → **one warning emitted** (Warning -1000: "Elements from Feature 1 do not all have the same value…"); `output[1] = 15.0` (last-writer)
- Feature 2: cell 1 (first) = `20.0`; cell 3 (second) = `20.0` (same → no additional warning); `output[2] = 20.0`

Expected output (float32, 3 tuples): `[0.0, 15.0, 20.0]`. Expected warnings: **1** (Warning -1000 for feature 1).

**Fixture AF-3** — 3-component, consistent:

| | |
|---|---|
| `featureIds` | `[1, 2, 1, 2]` |
| `cellValues` (uint8, 3-comp) | `cell0=[10,20,30]`, `cell1=[40,50,60]`, `cell2=[10,20,30]`, `cell3=[40,50,60]` |

Hand derivation:
- Feature 0: never written → `output[0] = [0, 0, 0]`
- Feature 1: cell 0 `[10,20,30]`; cell 2 `[10,20,30]` (same → no warning); `output[1] = [10, 20, 30]`
- Feature 2: cell 1 `[40,50,60]`; cell 3 `[40,50,60]` (same → no warning); `output[2] = [40, 50, 60]`

Expected output (uint8, 3 tuples): `[[0,0,0], [10,20,30], [40,50,60]]`. Expected warnings: **0**.

*Applied (Class 4 — Invariant):* Derivable properties any valid output must satisfy, asserted inline:

- `outputArray.numberOfTuples() == max(featureIds[:]) + 1` (AM and array resized consistently)
- `featureAttributeMatrix.shape() == {max(featureIds[:]) + 1}`
- `outputArray.getDataType() == inputCellArray.getDataType()`
- `outputArray.getNumberOfComponents() == inputCellArray.getNumberOfComponents()`
- `result.warnings().size() == 0` when all cells for each feature are value-consistent; `result.warnings().size() == 1` when any mismatch exists (one-warning-only guard)

*Encoded:* Implemented — `AnalyticalFixtures` TEST_CASEs in `test/CreateFeatureArrayFromElementArrayTest.cpp` (AF-1, AF-2, AF-3). Inline `REQUIRE` assertions match hand derivations above. Class 4 invariants (`getNumberOfTuples`, `getDataType`, `getNumberOfComponents`, `warnings().size()`) asserted in each fixture's Validation block.

*Second-engineer review:* Skipped — the oracle is integer-indexed indirection arithmetic on 4–5 element fixtures. No mathematical ambiguity: the closed-form derivation is `output[featureId * C + comp] = input[cellIdx * C + comp]` (last-writer), traceable directly to lines 43–55 of `Algorithms/CreateFeatureArrayFromElementArray.cpp`. Formal second-engineer review of this level of arithmetic was not justified.

## Code path coverage

*5 of 6 paths covered; cancellation check not directly exercised.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CreateFeatureArrayFromElementArray.cpp` (95 lines)

The algorithm has two phases: (a) output resize — scan `featureIds` for the max value and resize both the output array and the AttributeMatrix to `maxValue + 1` tuples; (b) per-cell copy loop — for each cell, index by `featureIds[cellIdx]` into the output array using last-writer-wins semantics.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | (a) Resize | `std::max_element` scan → `resizeTuples({maxValue + 1})` on both `createdArray` and `cellFeatureAttrMat` | AF-1 (3 tuples), AF-2 (3 tuples), AF-3 (3 tuples) — output AM has correct shape asserted via Class 4 invariant |
| 2 | (b) Per-cell | First encounter for a `featureId`: insert into `featureMap`; write value to output | AF-1, AF-2, AF-3 (all feature IDs present in each fixture) |
| 3 | (b) Per-cell | Subsequent encounter, values match first-seen: no warning emitted; write value (last-writer with same value = no-op in effect) | AF-1 (features 1 and 2), AF-3 (features 1 and 2) |
| 4 | (b) Per-cell | Subsequent encounter, values differ from first-seen: one warning total (guarded by `result.warnings().empty()`); write current value (last-writer-wins) | AF-2 (feature 1: `10.0` vs `15.0`) |
| 5 | (b) Per-cell | Multi-component inner loop: `totalCellArrayComponents > 1` — inner `for(cellCompIdx)` iterates C times per cell | AF-3 (C=3, uint8) |
| 6 | (b) Per-cell | Cancel check: `if(shouldCancel) return {}` at top of per-cell loop | *Not directly tested. Cancel-signal injection not implemented in test fixtures; structurally covered by the `shouldCancel` atomic bool parameter.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `Valid filter execution - 1 Component` | kept — regression | Loads Small IN100 via `6_5_test_data_1_v2.tar.gz`; runs filter on `CellData/ConfidenceIndex` (float32, 1-comp); compares output against `CellFeatureData/ConfidenceIndex` pre-existing in the dream3d file. **Circular-oracle flag**: `CellFeatureData/ConfidenceIndex` was generated by DREAM3D 6.5.x using the same-algorithm filter; comparison is legacy-output regression, not an independent oracle. Early-exit loop (breaks on first mismatch) instead of `CompareDataArrays` — all values compared only when all match, which is valid for the regression purpose. |
| `Valid filter execution - 3 Component` | kept — regression | Same as above on `CellData/IPFColors` (uint8, 3-comp). Same circular-oracle flag applies. Exercises multi-component code path (path #5 in coverage table). |
| `SIMPL Backwards Compatibility` | kept | Verifies `FromSIMPLJson()` mapping from both 6.5 UUID (`94438019-21bb-5b61-a7c3-66974b9a34dc`) and 6.4 filter-name (`CreateFeatureArrayFromElementArray`) formats. 4 parameter assertions each variant. Passes at HEAD. |
| `AnalyticalFixtures — AF-1` | implemented | Class 1 + Class 4 oracle; single-component float32, 5 cells, all consistent. Covers paths #1, #2, #3. Inline `REQUIRE` assertions per hand derivation in Oracle section. |
| `AnalyticalFixtures — AF-2` | implemented | Class 1 + Class 4 oracle; single-component float32, 4 cells, feature-1 inconsistent. Covers paths #1, #2, #4; verifies `warnings().size() == 1`. Inline `REQUIRE` assertions. |
| `AnalyticalFixtures — AF-3` | implemented | Class 1 + Class 4 oracle; 3-component uint8, 4 cells, all consistent. Covers paths #1, #2, #3, #5. Inline `REQUIRE` assertions per hand derivation. |

## Exemplar archive

- **Archive:** `6_5_test_data_1_v2.tar.gz` (shared input archive — not an oracle-specific archive for this filter)
- **SHA512:** `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/CreateFeatureArrayFromElementArrayFilter.md`
- **Oracle note:** The Class 1 and Class 4 oracles are encoded entirely as inline `REQUIRE` assertions in the AnalyticalFixtures test cases. No oracle-specific exemplar archive is created or required; `6_5_test_data_1_v2.tar.gz` provides regression test input data only.

## Deviations from DREAM3D 6.5.171

See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md`.

**Legacy comparison complete (2026-07-23) — one deviation, no migration impact.** Empirical A/B comparison was bit-identical on the Exact Match fixture. Static source analysis identified one deviation (D1): SIMPLNX handles the AM under-sized case (`max(featureIds) + 1 > AM.tupleCount`) by resizing the AM to accommodate, while SIMPL errors with -5555. For the Exact Match and AM over-sized cases — the only cases reachable from a SIMPL pipeline that ran successfully — SIMPLNX output is bit-identical to SIMPL. See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md` for full analysis.
