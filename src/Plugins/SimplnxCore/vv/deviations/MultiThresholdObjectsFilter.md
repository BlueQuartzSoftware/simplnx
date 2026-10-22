# Deviations from DREAM.3D v6.5.171: MultiThresholdObjectsFilter

Part 1 lists three capability differences from DREAM.3D v6.5.171. Part 2 lists three fixed SIMPLNX defects. All entries keep their existing IDs.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### MultiThresholdObjectsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D3` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |

**Symptom:** SIMPLNX can threshold one selected component of a multi-component array. Both DREAM.3D v6.5.171 filters require scalar arrays.

**Root cause:** algorithmic choice. Each SIMPLNX leaf stores its own component index. Preflight checks that index against its input array. Both legacy filters reject non-scalar arrays with error `-11003`.

**Affected users:** Users who threshold vector or other multi-component arrays. This capability has no equivalent in the legacy filters.

**Recommendation:** trust SIMPLNX. Select the required component explicitly; a legacy pipeline must first extract that component into a scalar array.

**Evidence:** `src/Plugins/SimplnxCore/test/MultiThresholdObjectsVvTest.cpp`, `Valid Single Thresholds: Int Multi-Component`, checks indices 0, 1 and 2. Legacy `Source/SIMPLib/CoreFilters/MultiThresholdObjects.cpp` and `MultiThresholdObjects2.cpp` contain the scalar-array checks in the SIMPL snapshot `0afaa6854`, selected as the last revision on or before the DREAM.3D v6.5.171 tag date, 2026-05-27. `vv_work:MultiThresholdObjectsFilter/ReadMe.md` confirms that `MultiThresholdObjects2::dataCheck()` rejects non-scalar arrays with error `-11003`.

---

### MultiThresholdObjectsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D5` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |

**Symptom:** SIMPLNX offers selectable mask types and defaults to uint8. DREAM.3D v6.5.171 always creates a boolean mask.

**Root cause:** algorithmic choice. `MultiThresholdObjectsFilter::parameters()` sets the default mask type to uint8. `FromSIMPLJson()` retains that default when `ScalarType` is absent. Both legacy filters create `DataArray<bool>` and expose no mask-type parameter.

**Affected users:** Users who convert a legacy pipeline or connect the mask to a downstream operation that requires a boolean array. Default values remain 1 and 0, but the declared array type differs.

**Recommendation:** either acceptable within zero tolerance for default mask values. Select boolean as the Mask Type when downstream operations require the legacy array type.

**Evidence:** Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/MultiThresholdObjectsFilter.cpp` defines the default and conversion fallback. The `Valid Execution, Mask DataType` case in `src/Plugins/SimplnxCore/test/MultiThresholdObjectsVvTest.cpp` checks ten numeric output types; `Valid Execution` in `MultiThresholdObjectsTest.cpp` checks boolean output. Both legacy sources identified under D3 allocate boolean masks.

---

### MultiThresholdObjectsFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D6` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |

**Symptom:** SIMPLNX can write custom TRUE and FALSE values. DREAM.3D v6.5.171 writes boolean values and has no custom-value option.

**Root cause:** algorithmic choice. SIMPLNX adds separate enable flags and numeric values for TRUE and FALSE. Direct and Scanline use the selected values to represent matches and nonmatches. The legacy parameter lists contain no corresponding options.

**Affected users:** Users who need numeric mask values other than 1 and 0. Default settings retain boolean mask values.

**Recommendation:** trust SIMPLNX. Use the default values for legacy-compatible mask values; use custom values when the downstream operation requires them.

**Evidence:** `Valid Execution - Custom Values` in both `src/Plugins/SimplnxCore/test/MultiThresholdObjectsTest.cpp` and `MultiThresholdObjectsVvTest.cpp` checks TRUE=25 and FALSE=10 across ten numeric output types. Both legacy sources identified under D3 write boolean output.

---

## Part 2 — SIMPLNX defects and behavior changes

### MultiThresholdObjectsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D1` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A nested threshold set could overwrite valid mask values with false. A leaf followed by an AND-connected nested set could produce an all-false mask.

**Root cause:** bug. `ThresholdSet()` allocated a false-filled temporary vector but evaluated its children into the parent output store. It then copied or combined the untouched temporary vector into that same output. The current Direct and Scanline evaluators give each child a separate result and combine it into its parent. DREAM.3D v6.5.171 evaluated children into a separate set buffer and was not affected.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.1 whose threshold trees contain nested sets, including mixed leaf and nested-set siblings.

**Recommendation:** trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later and regenerate masks produced with affected nested trees.

**Evidence:** `git show` inspection of all ten release sources places the defect in v7.0.0 through v7.4.1 and the correction in v7.4.2. The affected code is in `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/MultiThresholdObjectsFilter.cpp` through v7.4.0 and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.cpp` in v7.4.1. PR #1688 (`467a250d1`) supplies the correction. `src/Plugins/SimplnxCore/test/MultiThresholdObjectsVvTest.cpp`, `Valid Threshold Sets` / `ArraySet 6: leaf + nested set`, checks the corrected mask. Legacy `Source/SIMPLib/CoreFilters/MultiThresholdObjects2.cpp`, `thresholdSet()`, in SIMPL snapshot `0afaa6854` uses its set buffer for child evaluation. `vv_work:MultiThresholdObjectsFilter/ReadMe.md` records the 2026-07-23 comparison: corrected SIMPLNX masks matched DREAM.3D v6.5.171 and the independent NumPy oracle in all three 100-tuple cases. Before the correction, AB2 produced an all-false mask with 38 of 100 values wrong; AB1 matched. `vv_work:MultiThresholdObjectsFilter/compare.py` compares input values and checks each mask against the legacy output and the oracle after conversion to boolean. The run predates the DREAM3D-NX algorithm changes of 2026-09-20; a comparison with the current algorithm remains open.

---

### MultiThresholdObjectsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D2` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Inverted threshold trees could select the wrong tuples. Reversing tuple order did not compute the logical complement of each value.

**Root cause:** bug. The replace-and-invert paths called `std::reverse()` instead of flipping each mask value. Recursive calls also passed false for inversion, and the root inversion flag was passed to individual children before combination. The current evaluators apply each node's inversion after that node is complete. DREAM.3D v6.5.171 used element-wise `invertThreshold()`, honored nested inversion and inverted the completed root; it was not affected.

**Affected users:** Users of DREAM3D-NX v7.0.0 through v7.4.1 who invert a root threshold set or a nested set.

**Recommendation:** trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later and regenerate masks made with inverted threshold trees.

**Evidence:** All release sources through v7.4.1 contain `std::reverse()` in the threshold evaluator at the paths identified under D1. The v7.4.2 source applies logical inversion in `InsertThreshold()` and at the final root write. `src/Plugins/SimplnxCore/test/MultiThresholdObjectsVvTest.cpp`, `Valid Threshold Sets` / `ArraySet 7: leaf + inverted nested set`, checks nested inversion. `Top-level inversion is logical` and `Nested sets compose before root inversion` in `src/Plugins/SimplnxCore/test/MultiThresholdObjectsTest.cpp` check the root. Legacy `MultiThresholdObjects2.cpp` in SIMPL snapshot `0afaa6854` contains the corresponding logical inversion paths. `vv_work:MultiThresholdObjectsFilter/ReadMe.md` records 51 of 100 incorrect values in AB3 before the correction. This case combines an inverted nested set with OR. On 2026-07-23, the corrected mask matched DREAM.3D v6.5.171 and the independent NumPy oracle. The run predates the DREAM3D-NX algorithm changes of 2026-09-20 and does not verify the current algorithm.

---

### MultiThresholdObjectsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `MultiThresholdObjectsFilter-D4` |
| **Filter UUID** | `4246245e-1011-4add-8436-0af6bed19228` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Converting an Advanced legacy pipeline could discard its grouping and OR operations, so the converted pipeline computed a different mask.

**Root cause:** bug. `convertArrayThreshold()` did not copy the leaf union operator. `flattenSetThreshold()` removed nested groups and tried to negate comparisons by swapping operators. That transformation lost equality boundaries and did not apply De Morgan's rule. The converter now retains nested sets, their inversion flags and each leaf's union operator. DREAM.3D v6.5.171 was not affected because this SIMPLNX conversion path does not exist in the legacy application.

**Affected users:** Users who converted Advanced legacy pipelines with nested sets or OR operations in DREAM3D-NX v7.0.0 through v7.4.1. Native SIMPLNX pipelines did not use this converter.

**Recommendation:** trust SIMPLNX. Convert the original legacy pipeline again with DREAM3D-NX v7.4.2 or later, then regenerate its masks.

**Evidence:** `src/simplnx/Parameters/ArrayThresholdsParameter.cpp` in every release through v7.4.1 contains `flattenSetThreshold()` and omits the leaf `setUnionOperator()` call. The v7.4.2 source preserves both. PR #1688 (`467a250d1`) contains the fix. `src/Plugins/SimplnxCore/test/MultiThresholdObjectsVvTest.cpp`, `SIMPL Nested Set Conversion`, checks the tree from `src/Plugins/SimplnxCore/test/simpl_conversion/6_5/MultiThresholdObjectsFilter_Nested.json`. All filter tests pass in CI at `419f9db35` on Linux, macOS and Windows in-core builds.
