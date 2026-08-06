# V&V Report: CreateFeatureArrayFromElementArrayFilter

| | |
|---|---|
| Plugin | SimplnxCore |
| SIMPLNX Human Name | Create Feature Array from Element Array |
| SIMPLNX UUID | `50e1be47-b027-4f40-8f70-1283682ee3e7` |
| DREAM3D 6.5.171 equivalent | `CreateFeatureArrayFromElementArray` (SIMPL UUID `94438019-21bb-5b61-a7c3-66974b9a34dc`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | **COMPLETE** |
| Sign-off | *Nathan Young, 07-28-2026* |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **Port** — the per-cell copy loop is a line-by-line translation of SIMPL `CreateFeatureArrayFromElementArray`. UUID changed from SIMPL; legacy alias maintained via `FromSIMPLJson()` and SIMPL conversion fixtures. The sizing logic differs only in the AM under-sized case: SIMPL errors; SIMPLNX resizes and succeeds (see D1 in deviations). All pipelines that succeeded in SIMPL produce bit-identical output in SIMPLNX. |
| Oracle | **Class 1 (Analytical) primary + Class 4 (Invariant) companion** — expected outputs hand-derived for 9-fixture AnalyticalFixtures suite (AF-1: single-component consistent, AF-2: single-component inconsistent/warning, AF-3: three-component consistent, AF-4: error -81880 all-negative featureIds, AF-5: error -81881 shrink-protection guard, AF-6: gap in FeatureIds range resize-grown tuple, AF-7: error -81882 empty featureIds, AF-8: error -81880 mixed-negative featureIds, AF-9: error -81883 preflight tuple count mismatch). Class 4 invariants: output array has `max(featureIds)+1` tuples; output data type and component shape match input; error codes for boundary inputs. Implemented as inline `REQUIRE` assertions in `test/CreateFeatureArrayFromElementArrayTest.cpp`. |
| Code paths enumerated | **10 of 11 paths exercised; 1 uncovered:** cancel check. Error paths -81880 and -81881 covered by AF-4/AF-8 and AF-5; -81882 by AF-7; -81883 (preflight) by AF-9. |
| Tests today | **12 test cases** (2 regression + 1 SIMPL backwards-compat + 9 AnalyticalFixtures). **Circular-oracle flag**: existing regression tests compare SIMPLNX output against legacy-generated `CellFeatureData` arrays pre-existing in `6_5_test_data_1_v2.dream3d`; not a valid correctness oracle per policy. AF-1 through AF-3 provide the independent Class 1 + Class 4 output oracle; AF-4 through AF-9 cover error-path and boundary-condition Class 4 invariants. |
| Exemplar archive | `6_5_test_data_1_v2.tar.gz` (SHA512 `585b51b…`) — shared input archive. Pre-existing `CellFeatureData` arrays within the dream3d are legacy-generated; serve only as regression baselines. No oracle-specific archive needed for Class 1 (oracle is inline assertions). |
| Legacy comparison | **Complete (2026-07-23) — one deviation identified, no migration impact.** Empirical A/B comparison on synthetic 8×1×1 fixtures: bit-identical. Static source analysis identified D1 (AM under-sized: SIMPL errors -5555; SIMPLNX resizes and succeeds). D1 is unreachable from any SIMPL pipeline that ran successfully, so output is bit-identical for the entire valid SIMPL migration space. See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md`. |
| Bug flags | **Three bugs fixed during review:** (1) UB on empty featureIds: `std::minmax_element` on empty range is UB; fixed by empty-array guard → error -81882. (2) UB on negative featureIds: old `maxValue < 0` guard missed mixed-sign inputs; negative featureIdx in copy loop converts to `usize(UINT64_MAX)` → OOB write; fixed by `minValue < 0` guard → error -81880. (3) Unchecked tuple-count precondition: cell array and featureIds tuple counts were never compared; OOB featureIds access when counts differ; fixed by preflight check → error -81883. |
| V&V phase | **Complete.** Oracle designed (Class 1 + Class 4) and implemented; code-path analysis complete; Algorithm Relationship classified (Port); AnalyticalFixtures tests implemented (AF-1 through AF-9); legacy A/B comparison run (2026-07-23, bit-identical on full valid SIMPL migration space); one deviation (D1, SIMPLNX improvement) identified and documented. |

## Summary

`CreateFeatureArrayFromElementArrayFilter` copies each element-level data array value to the feature-level entry identified by the corresponding FeatureId, using last-writer-wins semantics, and emits one warning if any cell's value for a feature differs from the first-seen value. The filter was verified analytically using a Class 1 hand-derived oracle on three small synthetic fixtures covering consistent, inconsistent, and multi-component cases (AF-1, AF-2, AF-3), plus Class 4 invariants on output shape and type, and six error-path and boundary-condition fixtures: the all-negative-featureIds guard (AF-4, error -81880), the shrink-protection guard (AF-5, error -81881), a gap in FeatureIds range with a resize-grown never-written tuple (AF-6), the empty-featureIds guard (AF-7, error -81882), the mixed-negative-featureIds guard (AF-8, error -81880), and the preflight tuple-count-mismatch guard (AF-9, error -81883). All oracle assertions pass. The per-cell copy loop is a direct port of SIMPL `CreateFeatureArrayFromElementArray`; empirical A/B comparison (2026-07-23) confirmed bit-identical output for all inputs that SIMPL can process. One deviation exists (D1): SIMPLNX handles the AM under-sized case by resizing, whereas SIMPL errors with -5555. This deviation has no migration impact — it is unreachable from any pipeline that ran successfully in SIMPL.

## Algorithm Relationship

*Classification:* **Port**

*Evidence:* The SIMPLNX algorithm at `Algorithms/CreateFeatureArrayFromElementArray.cpp` (121 lines) is a line-for-line translation of the SIMPL `CreateFeatureArrayFromElementArray` filter. Identical control flow: iterate over cell tuples, look up `featureIds[cellIdx]`, record first-seen tuple offset in `featureFirstCellOffset`, compare current value against first-seen per component, emit one warning on first mismatch, write current value to `createdDataStore[featureId * C + comp]` (last-writer-wins). The SIMPL UUID (`94438019-21bb-5b61-a7c3-66974b9a34dc`) differs from the SIMPLNX UUID (`50e1be47-b027-4f40-8f70-1283682ee3e7`) because the filter was re-UUID-ed during the SIMPL→SIMPLNX port; the legacy alias is maintained through `FromSIMPLJson()` and `test/simpl_conversion/6_5/CreateFeatureArrayFromElementArrayFilter.json`.

*Port-time deltas that do not change output data:*

1. `QMap<int32_t, T*>` → `std::vector<usize>` for `featureFirstCellOffset` — SIMPL used an O(log n) sorted map from feature ID to raw source pointer; SIMPLNX uses an O(1) flat vector pre-allocated to `maxValue + 1` elements (initialized to `k_NotSeen = std::numeric_limits<usize>::max()`), indexed directly by feature ID, storing the first-seen flat element offset. Lookup semantics and iteration order over cells are unchanged.
2. SIMPL `EXECUTE_FUNCTION_TEMPLATE` macro → `ExecuteDataFunction(CopyCellDataFunctor{}, ...)` dispatch — API modernization; identical runtime dispatch on element data type.
3. Warning API: SIMPL used `filter->setWarningCondition(-1000, ss)` guarded by `bool warningThrown = false;`; SIMPLNX uses `result.warnings().push_back(Warning{-1000, ...})` guarded by `result.warnings().empty()`. Both guards produce the same behavior: exactly **one** warning per execution when any feature's cell values are inconsistent. Confirmed by empirical A/B comparison (2026-07-23) — no behavioral delta.
4. Algorithm class extracted from `executeImpl` into `Algorithms/CreateFeatureArrayFromElementArray.{hpp,cpp}` — code organization change only (PRs #1301 and #1544).
5. Added `shouldCancel` check inside the per-cell loop — no output change (cancel path was absent in SIMPL).

*Material PRs:*

- **#1301** — "ENH: Move Execution to Algorithm Classes": extracted algorithm class skeleton.
- **#1544** — "ENH: Move non-trivial Filter executeImpl() logic to Algorithm classes": finalized algorithm class with resize logic.
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

- `outputArray.getNumberOfTuples() == max(featureIds[:]) + 1` (output array resized consistently with AM)
- `outputArray.getDataType() == inputCellArray.getDataType()`
- `outputArray.getNumberOfComponents() == inputCellArray.getNumberOfComponents()`
- `result.warnings().size() == 0` when all cells for each feature are value-consistent; `result.warnings().size() == 1` when any mismatch exists (one-warning-only guard)
- execute returns error -81882 when the featureIds array is empty; execute returns error -81880 when any feature ID is negative (`minValue < 0`; covers all-negative and mixed cases); execute returns error -81881 when an AM child array has more tuples than the resize target; preflight returns error -81883 when cell array and featureIds tuple counts differ

*Encoded:* Implemented — `AnalyticalFixtures` TEST_CASEs in `test/CreateFeatureArrayFromElementArrayTest.cpp` (AF-1 through AF-9). `getNumberOfTuples`, `getDataType`, `getNumberOfComponents` asserted in the Validation block of AF-1/AF-2/AF-3/AF-6; `warnings().size()` asserted in the Execution block immediately after `filter.execute()`; execute error code invariants asserted in AF-4, AF-5, AF-7, and AF-8; preflight error code invariant asserted in AF-9.

*Second-engineer review:* Skipped — the Class 1 oracle (AF-1/AF-2/AF-3) is integer-indexed indirection arithmetic on 4–5 element fixtures. No mathematical ambiguity: the closed-form derivation is `output[featureId * C + comp] = input[cellIdx * C + comp]` (last-writer), traceable directly to lines 25–56 of `Algorithms/CreateFeatureArrayFromElementArray.cpp`. The Class 4 error-path invariants (AF-4/AF-5/AF-7/AF-8/AF-9) assert single integer error codes against structurally constructed fixtures; no arithmetic review is required. Formal second-engineer review was not justified for either oracle type.

## Code path coverage

*10 of 11 paths exercised; 1 uncovered (cancel check).*

Sources: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CreateFeatureArrayFromElementArray.cpp` (121 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CreateFeatureArrayFromElementArrayFilter.cpp` (preflightImpl, path #11)

The algorithm has two phases: **(a) `operator()()` sizing and guard logic** — guard empty featureIds; scan `featureIds` for `minValue` and `maxValue` via `std::minmax_element`; error when `minValue < 0`; when `maxValue + 1 > AM.tupleCount`, enter the grow block — inner shrink-protection loop iterates all AM children and returns error -81881 if any child's `getNumberOfTuples() > (maxValue + 1)`; if all pass, calls `cellFeatureAttrMat.resizeTuples({maxValue + 1})` which cascades to all AM children via `AttributeMatrix::resizeTuples`; otherwise skip the grow block entirely; **(b) `CopyCellDataFunctor` per-cell copy loop** — cancel check, `featureFirstCellOffset` flat-vector first-encounter recording, per-component value comparison with one-warning-only guard, last-writer-wins write.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | (a) Guard | `featureIdsRef.getNumberOfTuples() == 0` → `MakeErrorResult(-81882, ...)` | AF-7: featureIds array with 0 tuples; preflight succeeds; execute returns error -81882 |
| 2 | (a) Guard | `minValue < 0` (any feature ID negative — covers all-negative and mixed) → `MakeErrorResult(-81880, ...)` | AF-4: `featureIds = [-1, -2, -1]`; AF-8: `featureIds = [-1, 1, 2, -1]`; preflight succeeds; execute returns error -81880 |
| 3 | (a) Grow — shrink guard | `maxValue + 1 > AM.tupleCount` AND some AM child array has `getNumberOfTuples() > (maxValue + 1)` → `MakeErrorResult(-81881, ...)` — refuses to grow AM if doing so would shrink an independently oversized child | AF-5: Feature AM with 2 tuples; sibling child created directly with 5 tuples; `featureIds` max=3 → `maxValue+1=4`; outer fires (4>2), sibling check fires (5>4); execute returns error -81881 |
| 4 | (a) Grow | `maxValue + 1 > AM.tupleCount`, shrink check passes → `cellFeatureAttrMat.resizeTuples({maxValue + 1})` cascades to all AM children including `createdArray` | AF-1 (AM 1→3), AF-2 (AM 1→3), AF-3 (AM 1→3), AF-6 (AM 1→3) — all fixtures start with a 1-tuple AM; output `getNumberOfTuples()` asserted via Class 4 invariant |
| 5 | (a) Skip | `maxValue + 1 <= AM.tupleCount` → grow block skipped | `Valid filter execution - 1 Component`, `Valid filter execution - 3 Component` — Small IN100 AM already correctly sized |
| 6 | (b) Cancel | `if(shouldCancel) return {}` at top of per-cell loop | *Not tested. Cancel-signal injection not implemented in any test fixture.* |
| 7 | (b) Per-cell | First encounter for a `featureId`: record in `featureFirstCellOffset`; write value to output | AF-1, AF-2, AF-3, AF-6 (all feature IDs present) |
| 8 | (b) Per-cell | Subsequent encounter, values match first-seen: no warning emitted; write value | AF-1 (features 1 and 2), AF-3 (features 1 and 2) |
| 9 | (b) Per-cell | Subsequent encounter, values differ: one warning total (guarded by `result.warnings().empty()`); write current value (last-writer-wins) | AF-2 (feature 1: `10.0` vs `15.0`) |
| 10 | (b) Per-cell | Multi-component inner loop: `totalCellArrayComponents > 1` — inner `for(cellCompIdx)` iterates C times per cell | AF-3 (C=3, uint8) |
| 11 | (preflight) Tuple count mismatch | `selectedCellArrayStore.getNumberOfTuples() != featureIdsArray.getNumberOfTuples()` → `MakePreflightErrorResult(-81883, ...)` | AF-9: cellArray 4 tuples, featureIds 2 tuples; preflight returns error -81883 |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `Valid filter execution - 1 Component` | kept — regression | Loads Small IN100 via `6_5_test_data_1_v2.tar.gz`; runs filter on `CellData/ConfidenceIndex` (float32, 1-comp); compares output against `CellFeatureData/ConfidenceIndex` pre-existing in the dream3d file. Exercises path #5 (skip-grow: AM already correctly sized). **Circular-oracle flag**: `CellFeatureData/ConfidenceIndex` was generated by DREAM3D 6.5.x using the same-algorithm filter; comparison is legacy-output regression, not an independent oracle. Early-exit loop (breaks on first mismatch) instead of `CompareDataArrays` — all values compared only when all match, which is valid for the regression purpose. |
| `Valid filter execution - 3 Component` | kept — regression | Same as above on `CellData/IPFColors` (uint8, 3-comp). Same circular-oracle flag applies. Exercises paths #5 and #10 in the coverage table (skip-grow and multi-component). |
| `SIMPL Backwards Compatibility` | kept | Verifies `FromSIMPLJson()` mapping from both 6.5 UUID (`94438019-21bb-5b61-a7c3-66974b9a34dc`) and 6.4 filter-name (`CreateFeatureArrayFromElementArray`) formats. 4 parameter assertions each variant. Passes at HEAD. |
| `AF-1 single-component consistent` | implemented | Class 1 + Class 4 oracle; single-component float32, 5 cells, all consistent. Covers paths #4, #7, #8. Inline `REQUIRE` assertions per hand derivation in Oracle section. |
| `AF-2 single-component inconsistent` | implemented | Class 1 + Class 4 oracle; single-component float32, 4 cells, feature-1 inconsistent. Covers paths #4, #7, #9; verifies `warnings().size() == 1`. Inline `REQUIRE` assertions. |
| `AF-3 three-component consistent` | implemented | Class 1 + Class 4 oracle; 3-component uint8, 4 cells, all consistent. Covers paths #4, #7, #8, #10. Inline `REQUIRE` assertions per hand derivation. |
| `AF-4 error path all-negative featureIds` | implemented | Class 4 oracle (error invariant); `featureIds = [-1, -2, -1]`; preflight succeeds; execute returns error -81880. Covers path #2. |
| `AF-5 error path shrink-protection guard` | implemented | Class 4 oracle (error invariant); Feature AM 2 tuples, sibling child created with 5 tuples, `featureIds` max=3; preflight succeeds; execute returns error -81881. Covers path #3. |
| `AF-6 gap in FeatureIds range (resize-grown tuple never written)` | implemented | Class 1 + Class 4 oracle; float32, `featureIds = [0, 2, 0, 2]` (feature 1 never mapped); AM grows 1→3; output[1] retains init value 0.0 (`m_InitValue` fill from `DataStore<T>` construction). Covers paths #4, #7, #8. |
| `AF-7 error path empty featureIds array` | implemented | Class 4 oracle (error invariant); featureIds array with 0 tuples; preflight succeeds; execute returns error -81882. Covers path #1. |
| `AF-8 error path mixed negative and positive featureIds` | implemented | Class 4 oracle (error invariant); `featureIds = [-1, 1, 2, -1]`; maxValue=2 passes old all-negative guard; `minValue < 0` guard catches it; preflight succeeds; execute returns error -81880. Covers path #2. |
| `AF-9 error path featureIds tuple count mismatch` | implemented | Class 4 oracle (error invariant); cellArray 4 tuples, featureIds 2 tuples (in separate smaller AM); preflight returns error -81883. Covers path #11. |

## Exemplar archive

- **Archive:** `6_5_test_data_1_v2.tar.gz` (shared input archive — not an oracle-specific archive for this filter)
- **SHA512:** `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/CreateFeatureArrayFromElementArrayFilter.md`
- **Oracle note:** The Class 1 and Class 4 oracles are encoded entirely as inline `REQUIRE` assertions in the AnalyticalFixtures test cases. No oracle-specific exemplar archive is created or required; `6_5_test_data_1_v2.tar.gz` provides regression test input data only.

## Deviations from DREAM3D 6.5.171

See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md`.

**Legacy comparison complete (2026-07-23) — one deviation, no migration impact.** Empirical A/B comparison was bit-identical on the Exact Match fixture. Static source analysis identified one deviation (D1): SIMPLNX handles the AM under-sized case (`max(featureIds) + 1 > AM.tupleCount`) by resizing the AM to accommodate, while SIMPL errors with -5555. For the Exact Match and AM over-sized cases — the only cases reachable from a SIMPL pipeline that ran successfully — SIMPLNX output is bit-identical to SIMPL. See `vv/deviations/CreateFeatureArrayFromElementArrayFilter.md` for full analysis.
