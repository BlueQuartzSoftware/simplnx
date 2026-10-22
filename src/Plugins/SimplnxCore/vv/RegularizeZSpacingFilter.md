# V&V Report: RegularizeZSpacingFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `d6599986-1932-4bfc-993d-71eafefe6db0` |
| SIMPLNX Human Name | Regularize Z Spacing |
| DREAM.3D v6.5.171 equivalent | `RegularizeZSpacing` (Sampling plugin, UUID `bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-08; second engineer: Jared Duffey, 2026-07-14 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `RegularizeZSpacing` in DREAM.3D v6.5.171. The plane mapping is unchanged; SIMPLNX adds validation and an optional separate output geometry. |
| Oracle (confirmed) | Class 1 plane maps and Class 4 geometry invariants are encoded in `test/RegularizeZSpacingTest.cpp`. All pass in CI. |
| Code paths enumerated | 13 of 22 exercised by the ordinary tests. Gaps include defensive guards, cancellation, grouped child copying and additional array types. |
| Tests today | 6 test cases: 3 valid executions, 1 input-validation case, 1 SIMPL conversion case and 1 hidden HDF5 case. The five ordinary cases cover analytical values, geometry properties and error codes. |
| Exemplar archive | None. The test source constructs inputs and defines analytical expected values. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None |
| V&V phase | Analytical checks, input-validation checks and second-engineer review are complete. |

## Summary

`RegularizeZSpacingFilter` copies image planes onto a uniform Z spacing from a file of slice-boundary positions. Class 1 fixtures check exact output values, and Class 4 checks cover geometry properties and loose child data. All pass in CI.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; both implementations use the strict `>` plane-boundary comparison and truncate the output Z dimension with a minimum of one plane.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Bounds `[0,1,3,6,10]` give source-plane map `[0,1,2,2,3]` at spacing 2 and `[0]` at spacing 20; invariant checks require preserved X/Y dimensions and spacing, origin, and loose child values.

*Encoded:* `src/Plugins/SimplnxCore/test/RegularizeZSpacingTest.cpp::SimplnxCore::RegularizeZSpacingFilter: Valid Execution (New Geometry)` and `Valid Execution (Spacing Exceeds Extent)` contain 2 analytical fixtures; `Valid Execution (In Place)` adds geometry checks. All pass in CI.

*Second-engineer review:* Jared Duffey, 2026-07-14.

## Bugs found and fixed

None.

## Code path coverage

13 of 22 paths exercised. Each uncovered row states the missing fixture or required injection.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RegularizeZSpacing.cpp` (208 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RegularizeZSpacingFilter.cpp` (310 lines), and `src/simplnx/Utilities/FilterUtilities.cpp` (156 lines, copied-AttributeMatrix warning).

Test names below omit the common `SimplnxCore::RegularizeZSpacingFilter: ` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Non-positive Z spacing returns `-5555` | `Invalid Parameters` / `Non-positive Z spacing` |
| 2 | Preflight | Boundary file cannot be opened; return `-5556` | *Not directly tested. Requires a file-open failure after parameter validation or a direct helper call.* |
| 3 | Preflight | Fewer than `ZPoints + 1` parseable values return `-5557` | `Invalid Parameters` / `File with too few values` |
| 4 | Preflight | Decreasing boundary values return `-5558` | `Invalid Parameters` / `Non-monotonic Z boundary values` |
| 5 | Preflight | Non-positive final boundary returns `-5559` | `Invalid Parameters` / `Zero total Z extent` |
| 6 | Geometry | Missing assigned cell AttributeMatrix returns `-5560` | `Invalid Parameters` / `Missing cell Attribute Matrix` |
| 7 | Geometry | Non-DataArray cell member returns `-5561` | `Invalid Parameters` / `Non-DataArray member in cell Attribute Matrix` |
| 8 | Geometry | In-place mode replaces the source geometry through deferred actions | `Valid Execution (In Place)` checks the resulting name, Z dimension and spacing |
| 9 | Geometry | Separate-output mode retains the source geometry | `Valid Execution (New Geometry)` checks geometry properties and all output cell values |
| 10 | Geometry | Copy a loose array with its values intact | `Valid Execution (New Geometry)` checks all four `LooseData` values |
| 11 | Mapping | Output Z dimension truncates to zero; clamp to one plane | `Valid Execution (Spacing Exceeds Extent)` checks dimensions and values `[0,1]` |
| 12 | Mapping | Strict `>` comparison selects source planes, including an exact boundary | `Valid Execution (New Geometry)` checks map `[0,1,2,2,3]` |
| 13 | Copy | Copy each mapped int32 plane as a contiguous slab | `Valid Execution (New Geometry)` and `Valid Execution (Spacing Exceeds Extent)` check every output value |
| 14 | Cancellation | Stop before an array or before a plane copy | *Not directly tested. Requires cancellation injection.* |
| 15 | Copy | Copy planes and loose data through HDF5 stores | *Not directly tested. Requires the hidden `real HDF5 slice-copy oracle`; the cited CI evidence covers in-memory execution.* |
| 16 | Execution | Propagate an error when the boundary file is read again | *Not directly tested. Requires a file change between preflight and execution or a direct algorithm call.* |
| 17 | Execution | Reject a non-DataArray cell member with `-5561` | *Not directly tested. Preflight rejects the same member before execution.* |
| 18 | Geometry | Copy a loose group and rebase descendant paths | *Not directly tested. The fixtures contain a loose array, not a loose group.* |
| 19 | Preflight | Warn `-4015` when an additional AttributeMatrix is copied unchanged | *Not directly tested. No fixture contains an additional AttributeMatrix.* |
| 20 | Copy | Dispatch boolean and other numeric types, or copy multiple components | *Not directly tested. The fixtures use scalar int32 arrays.* |
| 21 | Parameters | Accept a readable boundary file without a `.txt` extension | *Not directly tested. Every boundary-file fixture uses `.txt`.* |
| 22 | Conversion | Convert legacy geometry selection, file, spacing and in-place default | `SIMPL Backwards Compatibility` / `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` |

## Test inventory

All six cases are in `src/Plugins/SimplnxCore/test/RegularizeZSpacingTest.cpp`. The five ordinary cases run in Linux, macOS and Windows in-core builds. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RegularizeZSpacingFilter: Valid Execution (New Geometry)` | new-for-V&V | Checks all 10 output values against `[0,1,2,3,4,5,4,5,6,7]`, geometry dimensions, spacing, origin, four loose child values and source retention. |
| `SimplnxCore::RegularizeZSpacingFilter: Valid Execution (Spacing Exceeds Extent)` | new-for-V&V | Checks the one-plane minimum at spacing 20, output dimensions `(2,1,1)` and values `[0,1]`. |
| `SimplnxCore::RegularizeZSpacingFilter: Valid Execution (In Place)` | new-for-V&V | Checks replacement under the original name, absence of the separate output name, Z dimension 5 and Z spacing 2. Does not assert cell values. |
| `SimplnxCore::RegularizeZSpacingFilter: Invalid Parameters` | new-for-V&V | Six sections check exact codes `-5555`, `-5557`, `-5558`, `-5559`, `-5560` and `-5561`. |
| `SimplnxCore::RegularizeZSpacingFilter: real HDF5 slice-copy oracle` | new-for-V&V | Hidden. Requires HDF5 input and output stores; both in-place settings check all 10 cell values, four loose child values and tuple dimensions. |
| `SimplnxCore::RegularizeZSpacingFilter: SIMPL Backwards Compatibility` | new-for-V&V | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check the filter UUID, geometry path, input file, spacing and in-place default. |

## Exemplar archive

- **Archive:** None — inline analytical fixtures.
- **SHA512:** Not applicable. `src/Plugins/SimplnxCore/test/CMakeLists.txt` registers no archive for this filter.
- **Provenance:** Not applicable. The Class 1 and Class 4 oracles are in `src/Plugins/SimplnxCore/test/RegularizeZSpacingTest.cpp`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/RegularizeZSpacingFilter.md` for the source comparison and migration guidance. These Part 1 entries describe parameter and validation differences.

| Deviation | Observed difference |
|-----------|---------------------|
| `RegularizeZSpacingFilter-D1` | SIMPLNX selects the geometry's assigned cell AttributeMatrix; legacy conversion discards the explicitly selected AttributeMatrix name. |
| `RegularizeZSpacingFilter-D2` | SIMPLNX rejects incomplete or nonnumeric boundary files, decreasing boundaries and a non-positive final boundary. DREAM.3D v6.5.171 lacks these checks. |
| `RegularizeZSpacingFilter-D3` | SIMPLNX can retain the input geometry and create a separate output. DREAM.3D v6.5.171 replaces the selected cell data in place. |
