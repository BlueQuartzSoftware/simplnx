# Deviations from DREAM.3D v6.5.171: CopyFeatureArrayToElementArrayFilter

Part 1 lists differences from DREAM.3D v6.5.171. Part 2 lists SIMPLNX defects fixed in DREAM3D-NX v7.4.2. All entries share one stable ID sequence.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### CopyFeatureArrayToElementArrayFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D1` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |

**Symptom:** A converted pipeline uses the source array name for the output in SIMPLNX. DREAM.3D v6.5.171 uses the explicit `CreatedArrayName` value.

**Root cause:** Algorithmic choice. SIMPLNX selects multiple feature arrays and appends one shared suffix to their names. In DREAM3D-NX v7.4.2, `FromSIMPLJson()` leaves that suffix empty. For example, copying `EquivalentDiameters` to `MyOutput` in DREAM.3D creates `EquivalentDiameters` after conversion. DREAM3D-NX v7.0.0 through v7.4.1 instead converted the explicit name to a suffix and created `EquivalentDiametersMyOutput`.

**Affected users:** Users who import a DREAM.3D pipeline with an output name different from its source name. Downstream references to the explicit legacy name must be updated.

**Recommendation:** Trust SIMPLNX. Update downstream references to the source array name, or rename the output before downstream processing.

**Evidence:** `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.cpp::FromSIMPLJson()` leaves the suffix empty. `SIMPL Backwards Compatibility` asserts this for both conversion fixtures. The source at each `upstream/release/v7.0.0` through `upstream/release/v7.4.1` maps `CreatedArrayName` to the suffix; `upstream/release/v7.4.2` contains the correction from PR #1682 (`5eb69f359`). The legacy `Source/SIMPLib/CoreFilters/CopyFeatureArrayToElementArray.cpp` calls `setName(getCreatedArrayName())`.

---

### CopyFeatureArrayToElementArrayFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D2` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |

**Symptom:** DREAM.3D v6.5.171 returns error `-5555` when a feature array has more tuples than the largest FeatureId plus one. SIMPLNX accepts these unused feature tuples.

**Root cause:** Algorithmic choice. DREAM.3D requires `largestFeatureId == numFeatures - 1`. SIMPLNX rejects only negative identifiers and identifiers at or above `numFeatures`. Unused feature tuples do not affect the values copied to cells.

**Affected users:** Users whose feature arrays retain entries for features absent from the cell FeatureIds, such as after cropping.

**Recommendation:** Trust SIMPLNX. Every referenced tuple is within the source array, and unused tuples need not be removed.

**Evidence:** `Over-provisioned Feature array accepted` checks six literal outputs from eight feature tuples with maximum ID 2. `vv_work:CopyFeatureArrayToElementArrayFilter/comparison/results.md` records a stock DREAM.3D v6.5.171 run on 2026-07-23: all three main-fixture output arrays match exactly, while the extra-feature probe returns `-5555` only in DREAM.3D. `comparison/make_input.py`, `comparison/compare.py`, and the `simpl_main.json`, `nx_main.d3dpipeline`, `simpl_overprov.json` and `nx_overprov.d3dpipeline` files under `vv_work:CopyFeatureArrayToElementArrayFilter/` define that comparison. These results apply to the algorithm used on that date. Subsequent algorithm-source changes are `2b7f72ca5` (2026-08-05), `4ec21e93f` (2026-09-20) and `a33c370ca` (2026-09-25).

---

### CopyFeatureArrayToElementArrayFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D3` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |

**Symptom:** A negative FeatureId can make DREAM.3D v6.5.171 read outside the feature array and produce an undefined value. SIMPLNX returns error `-5355` before copying values.

**Root cause:** Bug. DREAM.3D v6.5.171 was affected: its maximum-ID check does not reject negatives, and `copyData<T>()` uses the identifier in an unchecked pointer offset. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. Their validation rejects negative identifiers before the copy.

**Affected users:** Users with negative sentinel values, such as `-1` for an unassigned cell, in FeatureIds.

**Recommendation:** Trust SIMPLNX. Replace negative sentinels with valid feature identifiers before execution. Undefined legacy values are not valid feature data.

**Evidence:** `vv_work:CopyFeatureArrayToElementArrayFilter/comparison/results.md` records the 2026-07-23 negative-ID probe: DREAM.3D completes and writes an observed zero at the affected cell, while SIMPLNX returns `-5355`. The probe pipelines are `vv_work:CopyFeatureArrayToElementArrayFilter/comparison/simpl_negid.json` and `vv_work:CopyFeatureArrayToElementArrayFilter/comparison/nx_negid.d3dpipeline`. `Execute Error - Negative FeatureIds (-5355)` checks the current error and all zero-initialized output values. Each release source uses either `ValidateNumFeaturesInArray()` (v7.0.0 through v7.2.0) or `ValidateFeatureIdsToFeatureAttributeMatrixIndexing()` (v7.2.1 through v7.4.2), both with negative-ID rejection. The legacy source is `Source/SIMPLib/CoreFilters/CopyFeatureArrayToElementArray.cpp` at SIMPL `0afaa6854ac0ef27df17278e11be711f74f77b90`, the last revision on or before the DREAM.3D `v6.5.171` tag date, 2026-05-27.

---

## Part 2 — SIMPLNX defects and behavior changes

### CopyFeatureArrayToElementArrayFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D4` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Selecting a NeighborList or StringArray can throw `std::bad_cast` during preflight instead of returning a validation error.

**Root cause:** Bug. The selection parameter accepted any array type, but preflight obtained each selected object as `IDataArray`. NeighborList and StringArray do not meet that requirement. The filter now permits only `IArray::ArrayType::DataArray`. DREAM.3D has no corresponding reference-cast path; its copy routine checks supported primitive types.

**Affected users:** Users who select a non-DataArray as feature data in DREAM3D-NX v7.0.0 through v7.4.1.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later to receive a validation error for an unsupported selection.

**Evidence:** PR #1689 (`2b7f72ca5`) restricts the parameter type. Each release source from `upstream/release/v7.0.0` through `upstream/release/v7.4.1` uses `ArrayType::Any`; `upstream/release/v7.4.2` uses `ArrayType::DataArray`. `Preflight Error - Non-DataArray selection rejected` checks a NeighborList. The legacy source cited in D3 uses checked dynamic casts and reports unsupported primitive types.

---

### CopyFeatureArrayToElementArrayFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D5` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** A slash in the created-array suffix can throw `std::invalid_argument` instead of returning a preflight error.

**Root cause:** Bug. The filter combined the unchecked suffix with the source name and constructed an invalid output DataPath. Preflight now checks for `/` and returns error `-3021`. DREAM.3D has no suffix parameter or this DataPath construction path.

**Affected users:** Users who enter `/` in the suffix in DREAM3D-NX v7.0.0 through v7.4.1.

**Recommendation:** Trust SIMPLNX. Remove `/` from the suffix. DREAM3D-NX v7.4.2 or later reports the invalid suffix before creating an output path.

**Evidence:** PR #1689 (`2b7f72ca5`) adds the slash check. It is absent from each release source through `upstream/release/v7.4.1` and present in `upstream/release/v7.4.2`. `Preflight Error - Suffix contains '/' (-3021)` checks the diagnostic.

---

### CopyFeatureArrayToElementArrayFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `CopyFeatureArrayToElementArrayFilter-D6` |
| **Filter UUID** | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.1 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Empty FeatureIds can cause an invalid memory read or a crash during execution.

**Root cause:** Bug. The range validator dereferenced the iterators returned by `std::minmax_element()` even when the input was empty. The validator now returns success before that read, and the filter creates empty outputs. DREAM3D-NX v7.0.0 through v7.2.0 used a range loop that skipped empty input. DREAM.3D uses a count-controlled maximum-ID loop and has no empty-iterator dereference.

**Affected users:** Users who copy feature values to an empty cell array in DREAM3D-NX v7.2.1 through v7.4.1.

**Recommendation:** Trust SIMPLNX. Use DREAM3D-NX v7.4.2 or later for empty cell arrays.

**Evidence:** PR #1689 (`2b7f72ca5`) adds the empty-store guard to `src/simplnx/Utilities/DataArrayUtilities.cpp`. Direct source inspection finds the unsafe min/max dereference in each release from `upstream/release/v7.2.1` through `upstream/release/v7.4.1`, and the guard in `upstream/release/v7.4.2`. `Zero-tuple FeatureIds accepted` checks successful execution and zero output tuples. The legacy source cited in D3 has no min/max iterator reduction.
