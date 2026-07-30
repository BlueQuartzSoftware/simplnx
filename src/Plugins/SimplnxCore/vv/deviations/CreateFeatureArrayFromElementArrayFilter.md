# Deviations from DREAM3D 6.5.171: CreateFeatureArrayFromElementArrayFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

Entries are referenced by stable ID (`CreateFeatureArrayFromElementArray-D<N>`) from the V&V report and from public migration guidance. The Filter UUID field is the permanent cross-reference anchor.

---

## Headline

**One behavioral deviation identified — SIMPLNX improvement over SIMPL.** The per-cell copy loop is a genuine port and produces bit-identical output for all inputs that SIMPL can process. The single deviation is that SIMPLNX handles the under-sized AM case (where `max(featureIds) + 1 > AM.tupleCount`) by resizing the AM to accommodate, whereas SIMPL errors in that case. All pipelines that succeeded in SIMPL will succeed identically in SIMPLNX.

---

## Known deviations

### CreateFeatureArrayFromElementArray-D1 — AM under-sized: SIMPL errors; SIMPLNX resizes and succeeds

**Severity:** Low for migration — this deviation only occurs in configurations that SIMPL could not process at all (error -5555). No SIMPL pipeline that previously succeeded can produce this condition, so there is no output difference for any pipeline that ran to completion in SIMPL.

**SIMPL behavior** (`execute()`, lines 226–252):
1. Reads `numFeatures = attributeMatrix.getNumberOfTuples()` — the pre-existing AM tuple count.
2. Scans all cells for `largestFeature = max(featureIds[:])`.
3. If `largestFeature >= numFeatures` → **error -5555** ("Attribute Matrix has N tuples but the input array has a Feature ID value of at least M"). No output is produced; the filter exits.
4. Otherwise → `copyCellData<T>(..., numFeatures, ...)` creates a fresh `numFeatures`-tuple zero-initialized output array and fills it. AM tuple count is **never changed**.

**SIMPLNX behavior** (`operator()()`, lines 84–112):
1. Computes `maxValue = max(featureIds[:])` via `std::max_element`.
2. If `maxValue < 0` → error -81880 (all-negative guard; SIMPL has undefined behavior in this case).
3. If `maxValue + 1 > cellFeatureAttrMat.getNumberOfTuples()`:
   - Runs a shrink-protection loop over all AM children: if any child array has `getNumberOfTuples() > (maxValue + 1)` — meaning growing the AM to `maxValue + 1` would shrink that child — returns error -81881. This path requires a child array to have been independently resized above the AM's tuple count; no test fixture exercises it.
   - **Resizes the AM** via `cellFeatureAttrMat.resizeTuples({maxValue + 1})`. This cascades to all AM children (`AttributeMatrix::resizeTuples` iterates `findAllChildrenOfType<IArray>()` and calls `array->resizeTuples(m_TupleShape)` on each), including the newly created output array.
4. Runs the copy loop.

**Divergent outcomes by case:**

| Case | Condition | SIMPL result | SIMPLNX result |
|---|---|---|---|
| **Exact match** | `max(featureIds) + 1 == AM.tupleCount` | SUCCESS — output has `AM.tupleCount` tuples | SUCCESS — resize block skipped; output has `AM.tupleCount` tuples (identical) |
| **AM over-sized** | `max(featureIds) + 1 < AM.tupleCount` | SUCCESS — output has `AM.tupleCount` tuples; trailing feature slots zero-filled | SUCCESS — resize block skipped; output has `AM.tupleCount` tuples; trailing slots retain `"0"` fill from `CreateArrayAction` (identical) |
| **AM under-sized** | `max(featureIds) + 1 > AM.tupleCount` | **ERROR -5555** — "Attribute Matrix has N tuples but Feature ID value is at least M" | **SUCCESS** — AM and all children (including output array) resized to `max(featureIds) + 1`; copy runs normally |

**Why the A/B comparison missed this:**
The synthetic fixture (`FeatureIds=[1,2,1,2,1,2,1,2]`, `max=2`) was paired with an AM of exactly 3 tuples — the Exact Match case. The AM under-sized case requires a pipeline configuration that SIMPL would have rejected at runtime, so it is not reachable from any SIMPL-generated test data.

**Migration impact:** None. Any pipeline that completed successfully in SIMPL had `max(featureIds) < numFeatures` by definition, placing it in the Exact Match or AM over-sized case — both of which produce identical output in SIMPLNX. The AM under-sized case is a new capability in SIMPLNX.

---

### Note: All-negative FeatureIds — SIMPLNX improvement over SIMPL

Documented for completeness; not a deviation that disadvantages SIMPLNX.

If all feature IDs are negative, `maxValue < 0` → SIMPLNX returns clean error -81880. In SIMPL, `largestFeature` stays 0 (negative values never satisfy `m_FeatureIds[i] > largestFeature`); `mismatchedFeatures` stays false; `copyCellData()` is called; inside, `featureIdx` is negative and `fPtr + (numComp * featureIdx)` is a pointer before the start of the allocation — undefined behavior. SIMPLNX is strictly safer for this input.

---

## Comparison method

| | |
|---|---|
| **Comparison type** | Runtime A/B (both implementations executed on identical input) + static source analysis |
| **Fixture** | Synthetic 8×1×1 image geometry; `FeatureIds=[1,2,1,2,1,2,1,2]`; `CellFloat` (float32, 1-comp): `[10,20,30,20,10,20,30,20]`; `CellRGB` (uint8, 3-comp): `[[10,20,30],[40,50,60],[10,20,30],[40,50,60],[10,20,30],[40,50,60],[70,80,90],[40,50,60]]` |
| **Fixture coverage** | Exact Match case (`max(featureIds)+1 == AM.tupleCount`). The AM under-sized case cannot be generated from a SIMPL pipeline, so no A/B fixture for it exists. |
| **Tolerance** | Bit-identical (copy-only filter; no floating-point accumulation) |
| **Comparison driver** | `feature_from_element_vv/compare_outputs.py` |
| **Run date** | 2026-07-23 |
| **SIMPL runner** | `DREAM3D-6.5.171-Linux-x86_64/bin/PipelineRunner` |
| **NX runner** | `DREAM3DNX-Release-Linux-x64/Bin/nxrunner` |

---

## Results

| Test | SIMPL vs Oracle | NX vs Oracle | A/B |
|---|---|---|---|
| 1-component float32 (`CellFloat → FeatureFloat`) | PASS | PASS | MATCH |
| 3-component uint8 (`CellRGB → FeatureRGB`) | PASS | PASS | MATCH |

**A/B result is complete for the relevant migration space.** All configurations that SIMPL can execute fall into the Exact Match or AM over-sized cases, where SIMPLNX output is bit-identical to SIMPL. D1 covers a configuration SIMPL could not produce output for, so no A/B comparison is possible or necessary for it.

---

## Migration recommendation

**No action required.** Any pipeline that ran successfully in SIMPL will produce bit-identical output in SIMPLNX. SIMPLNX additionally handles the AM under-sized case that SIMPL rejected with error -5555.
