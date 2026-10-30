# V&V Report: CopyFeatureArrayToElementArrayFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `4c8c976a-993d-438b-bd8e-99f71114b9a1` |
| SIMPLNX Human Name | Create Element Array from Feature Array |
| DREAM.3D v6.5.171 equivalent | `CopyFeatureArrayToElementArray` (SIMPLib CoreFilters, UUID `99836b75-144b-5126-b261-b411133b5e8a`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-24; second engineer: Nathan Young, 2026-07-28 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to DREAM.3D v6.5.171 `CopyFeatureArrayToElementArray`. SIMPLNX copies the same feature tuples, with multiple input arrays, suffix-based names and different input checks. |
| Oracle (confirmed) | Classes 1 and 4: literal scalar, RGB and boolean outputs plus equal-feature invariants in `test/CopyFeatureArrayToElementArrayTest.cpp`. All pass in CI. |
| Code paths enumerated | 20 of 28 exercised. Uncovered paths include defensive guards, the Direct fallback, cancellation boundaries and the full-block tail case. |
| Tests today | 16 test definitions: 15 `TEST_CASE` definitions and one template over 10 numeric types. They cover values, validation, bulk I/O failures, cancellation and pipeline conversion; one definition is hidden. |
| Exemplar archive | None. Tests construct inputs and expected values in C++. |
| Legacy comparison | Not run for the current algorithm. The available DREAM.3D v6.5.171 comparison is dated 2026-07-23. |
| Bug flags | None |
| V&V phase | Oracle and regression checks are complete. The current algorithm still requires a legacy comparison. |

## Summary

`CopyFeatureArrayToElementArrayFilter` copies each selected feature tuple to every cell with that FeatureId. Class 1 literal expectations and Class 4 equal-feature invariants check the copied values. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; both kernels copy `source[FeatureIds[i] * C + c]`, while SIMPLNX adds multiple-array selection and changes naming and validation.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* A 12-cell, four-feature fixture defines 60 literal scalar, RGB and boolean values; cells with equal FeatureIds must have equal RGB tuples.

*Encoded:* `src/Plugins/SimplnxCore/test/CopyFeatureArrayToElementArrayTest.cpp::Analytical Oracle (Class 1)` and `Over-provisioned Feature array accepted` — two fixtures with 60 and 6 literal value checks. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-07-28.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for D4, D5 and D6. DREAM3D-NX v7.0.0 already rejects the negative identifiers described by D3.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `CopyFeatureArrayToElementArrayFilter-D3` | Negative FeatureIds cause reads outside the source array. | DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. | Execution rejects negative FeatureIds with error `-5355` before copying. |
| `CopyFeatureArrayToElementArrayFilter-D4` | Selecting a non-DataArray throws `std::bad_cast` during preflight. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Parameter validation restricts the selection to DataArray objects. |
| `CopyFeatureArrayToElementArrayFilter-D5` | A slash in the suffix throws `std::invalid_argument`. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Preflight rejects a slash with error `-3021`. |
| `CopyFeatureArrayToElementArrayFilter-D6` | Empty FeatureIds cause an invalid iterator read during range validation. | DREAM3D-NX v7.2.1 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Empty FeatureIds produce empty output arrays without reading an iterator. |

## Code path coverage

20 of 28 paths exercised. Each uncovered path has a reason below.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArray.cpp` (41 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArrayDirect.cpp` (147 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/CopyFeatureArrayToElementArrayScanline.cpp` (148 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/CopyFeatureArrayToElementArrayFilter.cpp` (164 lines), and `src/simplnx/Utilities/DataArrayUtilities.cpp` (515 lines).

Test names below omit the common `SimplnxCore::CopyFeatureArrayToElementArrayFilter: ` prefix. Direct and Scanline value fixtures use in-memory stores.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Reject an empty selection in preflight and execution | `Preflight Error - Empty selection (filter guard)` |
| 2 | Parameters | Reject non-DataArray selections | `Preflight Error - Non-DataArray selection rejected` — NeighborList input |
| 3 | Preflight | Reject a slash in the suffix with `-3021` | `Preflight Error - Suffix contains '/' (-3021)` |
| 4 | Preflight | Reject different feature-array tuple counts with `-3020` | `Preflight Error - Feature array tuple count mismatch (-3020)` |
| 5 | Preflight | Create one output per source, with cell tuple shape and source component shape | `Analytical Oracle (Class 1)` |
| 6 | Actions | Reject an existing output path with `-266`; preserve the original array | `Execute Error - Created name collides with existing array (-266)` |
| 7 | Validation | Reject negative FeatureIds with `-5355`; leave outputs at zero | `Execute Error - Negative FeatureIds (-5355)` |
| 8 | Validation | Reject FeatureIds at or above the feature tuple count with `-5351` | `Execute Error - FeatureId exceeds Feature tuple count (-5351)` |
| 9 | Validation | Accept unused feature tuples | `Over-provisioned Feature array accepted` |
| 10 | Validation | Accept empty FeatureIds and create empty outputs | `Zero-tuple FeatureIds accepted` |
| 11 | Dispatch | Return early for an empty algorithm selection | *Not directly tested. Filter validation rejects the empty selection before these defensive guards.* |
| 12 | Dispatch | Copy all 11 scalar types | `Valid filter execution` — 10 numeric types; `Analytical Oracle (Class 1)` — boolean values |
| 13 | Direct | Copy components through raw pointers | `Analytical Oracle (Class 1)`; `Valid filter execution` |
| 14 | Direct | Copy through the generic store fallback | *Not directly tested. The fixtures use concrete DataStore objects.* |
| 15 | Direct | Cancel before copying a selected array | *Not directly tested. The cancellation fixtures invoke Scanline only.* |
| 16 | Direct | Cancel inside the raw-pointer cell loop | *Not directly tested. Requires cancellation during the Direct copy.* |
| 17 | Direct | Cancel inside the generic store loop | *Not directly tested. Requires a fallback store and cancellation during the copy.* |
| 18 | Scanline | Cache feature values and copy a partial cell block | `Analytical Oracle (Class 1)` — literal scalar, RGB and boolean outputs |
| 19 | Scanline | Copy a full 65,536-cell block and a partial tail | *Not directly tested. The matching `genuine HDF5 65536-block tail oracle` fixture is hidden and requires an HDF5 store manager.* |
| 20 | Validation | Propagate a FeatureIds read failure | `Scanline propagates bulk failures before partial writes` / `validator FeatureIds read fails` |
| 21 | Scanline | Propagate a feature-cache read failure | `Scanline propagates bulk failures before partial writes` / `feature cache read fails` |
| 22 | Scanline | Propagate a cell-block read failure | `Scanline propagates bulk failures before partial writes` / `FeatureIds transfer read fails after validation` |
| 23 | Scanline | Propagate an output write failure | `Scanline propagates bulk failures before partial writes` / `output write fails` |
| 24 | Scanline | Honor cancellation before validation and array copying | `Pre-cancelled Scanline does not write` |
| 25 | Scanline | Honor cancellation after reading a cell block, before copying its values | `Cancellation after FeatureIds chunk read does not write` |
| 26 | Scanline | Cancel at the start of a later cell block | *Not directly tested. Requires cancellation between two block transfers.* |
| 27 | Scanline | Cancel after filling the output buffer, before writing it | *Not directly tested. Requires cancellation at this write boundary.* |
| 28 | Conversion | Convert the selected path and FeatureIds path; leave the suffix empty | `SIMPL Backwards Compatibility` — both conversion fixtures |

## Test inventory

Test names omit the common `SimplnxCore::CopyFeatureArrayToElementArrayFilter: ` prefix.

| Test case | Status | Notes |
|-----------|--------|-------|
| `Preflight Error - Empty selection (filter guard)` | new-for-V&V | Uses a valid FeatureIds path and checks the filter's empty-selection error in preflight and execution. |
| `Preflight Error - Feature array tuple count mismatch (-3020)` | kept | Selects arrays with three and four tuples; checks `-3020`. |
| `Preflight Error - Non-DataArray selection rejected` | new-for-V&V | Selects a NeighborList and checks invalid preflight. Covers D4. |
| `Preflight Error - Suffix contains '/' (-3021)` | new-for-V&V | Checks `-3021` for a slash in the suffix. Covers D5. |
| `Execute Error - Created name collides with existing array (-266)` | new-for-V&V | Checks the action error and every original FeatureIds value. |
| `Analytical Oracle (Class 1)` | new-for-V&V | Checks 60 literal scalar, RGB and boolean values, equal-feature RGB invariants and inherited tuple dimensions. |
| `genuine HDF5 65536-block tail oracle` | new-for-V&V | Hidden (`[.OocStoreContract]`). Requires HDF5 input and output stores; checks literal RGB tuples at cells 0, 65,535 and 65,536. |
| `Scanline propagates bulk failures before partial writes` | kept | Injects four read/write errors; checks error codes and unchanged output sentinels for the read failures. |
| `Pre-cancelled Scanline does not write` | kept | Sets cancellation before execution and checks all output sentinels. |
| `Cancellation after FeatureIds chunk read does not write` | kept | Cancels after the second FeatureIds read and checks all output sentinels. |
| `Execute Error - Negative FeatureIds (-5355)` | new-for-V&V | Checks `-5355` after successful preflight and verifies all three output arrays remain zero. Covers D3. |
| `Execute Error - FeatureId exceeds Feature tuple count (-5351)` | new-for-V&V | Uses ID 4 with four feature tuples; checks `-5351` and zero output arrays. |
| `Over-provisioned Feature array accepted` | new-for-V&V | Uses eight feature tuples with maximum ID 2; checks six literal outputs. Covers D2. |
| `Zero-tuple FeatureIds accepted` | new-for-V&V | Checks successful execution and zero output tuples. Covers D6. |
| `Valid filter execution` | kept | Template over 10 numeric types; checks two 30-cell outputs and tuple dimensions. V&V replaced uninitialized or constant inputs with distinct feature values. |
| `SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check UUID and arguments, including the selected path and empty suffix. |

## Exemplar archive

- **Archive:** None. Fixtures are constructed in `src/Plugins/SimplnxCore/test/CopyFeatureArrayToElementArrayTest.cpp`.
- **SHA512:** Not applicable. `src/Plugins/SimplnxCore/test/CMakeLists.txt` registers no archive for this filter.
- **Provenance:** Not applicable; no exemplar archive. Comparison evidence is cited in `src/Plugins/SimplnxCore/vv/deviations/CopyFeatureArrayToElementArrayFilter.md`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/CopyFeatureArrayToElementArrayFilter.md` for root causes, affected users and recommendations.

| Deviation | Observed difference |
|-----------|---------------------|
| `CopyFeatureArrayToElementArrayFilter-D1` | Converted pipelines retain the source array name in SIMPLNX; DREAM.3D uses the explicit output name. |
| `CopyFeatureArrayToElementArrayFilter-D2` | SIMPLNX accepts unused feature tuples; DREAM.3D requires the feature tuple count to equal the largest FeatureId plus one. |
| `CopyFeatureArrayToElementArrayFilter-D3` | SIMPLNX rejects negative FeatureIds; DREAM.3D can read outside the source array. |
