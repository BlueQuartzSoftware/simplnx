# V&V Report: MultiThresholdObjectsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `4246245e-1011-4add-8436-0af6bed19228` |
| SIMPLNX Human Name | Multi-Threshold Objects |
| DREAM.3D v6.5.171 equivalent | `MultiThresholdObjects` (Threshold Objects) and `MultiThresholdObjects2` (Threshold Objects (Advanced)) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Matthew Marine, 2026-07-24; second engineer: Michael Jackson, 2026-08-19 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite. One threshold tree replaces the flat basic filter and the nested Advanced filter in DREAM.3D v6.5.171. |
| Oracle (confirmed) | Class 1 analytical masks are encoded in `test/MultiThresholdObjectsTest.cpp` and `test/MultiThresholdObjectsVvTest.cpp`. All pass in CI. |
| Code paths enumerated | 36 of 51 exercised. Gaps include large chunks, several cancellation points, single-option custom values and basic pipeline conversion. |
| Tests today | 27 test declarations: 23 ordinary cases and 4 template cases. They cover comparisons, nested logic, data types, validation, I/O errors, cancellation and conversion. |
| Exemplar archive | None. Tests construct inputs and expected masks in memory. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None |
| V&V phase | Analytical verification and second-engineer review are complete. Three fixed defects and three capability differences are documented. |

## Summary

`MultiThresholdObjectsFilter` compares array values with thresholds and combines the results through nested AND, OR and inversion operations. Class 1 analytical fixtures check the mask values, while other tests check validation, cancellation, I/O errors and pipeline conversion. All pass in CI.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps both legacy UUIDs to this filter; Direct and Scanline evaluate the same unified threshold-tree model.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Hand-derived comparisons cast the threshold to the input scalar type, then combine tuple results with AND, OR and logical inversion.

*Encoded:* `src/Plugins/SimplnxCore/test/MultiThresholdObjectsVvTest.cpp::SimplnxCore::MultiThresholdObjects V&V: Valid Threshold Sets` supplies seven tree fixtures; `Exemplar Single Thresholds: Int` and `Exemplar Single Thresholds: Float` supply two literal masks, with comparison and type sweeps in the same file. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-08-19.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `MultiThresholdObjectsFilter-D1`, `MultiThresholdObjectsFilter-D2` and `MultiThresholdObjectsFilter-D4`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `MultiThresholdObjectsFilter-D1` | Nested-set evaluation applied an unused false buffer and overwrote or cleared valid mask values. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Each child has a separate result; the parent combines that result with the selected operator. |
| `MultiThresholdObjectsFilter-D2` | Inversion could reverse tuple order, omit nested inversion or invert children before combining the root. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Each node inverts its own completed mask values without changing tuple order. |
| `MultiThresholdObjectsFilter-D4` | Advanced pipeline conversion flattened nested sets and lost leaf union operators. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Conversion preserves the nested tree, inversion flags and child union operators. |

## Code path coverage

36 of 51 paths exercised. Each uncovered row states the missing fixture or injection point.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjects.cpp` (38 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjectsDirect.cpp` (240 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/MultiThresholdObjectsScanline.cpp` (324 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/MultiThresholdObjectsFilter.cpp` (293 lines); `src/simplnx/Parameters/ArrayThresholdsParameter.cpp` (306 lines).

Test names below omit their common namespace prefix. `V&V:` identifies cases in `MultiThresholdObjectsVvTest.cpp`; other names refer to `MultiThresholdObjectsTest.cpp`. The scenario harness selects Direct and Scanline on arrays in memory.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | No required input arrays → error `-4000` | V&V: `Invalid Execution` / `Empty ArrayThresholdSet` |
| 2 | Preflight | Input tuple counts differ → `UnequalTuples` | V&V: `Invalid Execution` / `Mismatching Tuples in Threshold Arrays` |
| 3 | Preflight | Component index exceeds the array bounds → `InvalidComponentIndex` | V&V: `Invalid Execution` / `Out of Bounds Component Index` |
| 4 | Preflight | Boolean output with custom TRUE enabled → `CustomTrueWithBoolean` | V&V: `Invalid Execution - Boolean Custom Values` / `Custom True Value` |
| 5 | Preflight | Boolean output with custom FALSE enabled → `CustomFalseWithBoolean` | V&V: `Invalid Execution - Boolean Custom Values` / `Custom False Value` |
| 6 | Preflight | Custom TRUE is outside the output type range → `CustomTrueOutOfBounds` | V&V: `Invalid Execution - Out of Bounds Custom Values` — below minimum and above maximum |
| 7 | Preflight | Custom FALSE is outside the output type range → `CustomFalseOutOfBounds` | V&V: `Invalid Execution - Out of Bounds Custom Values` — below minimum and above maximum |
| 8 | Preflight | Valid inputs → create a scalar mask with the input tuple shape | `Valid Execution` — checks the output action, type, store and mask values |
| 9 | Parameter validation | Threshold input path is absent → reject the argument | V&V: `Invalid Execution` / `Empty ArrayThreshold DataPath` |
| 10 | Dispatch | Select Direct for arrays in memory | `Valid Execution`; V&V: `Valid Threshold Sets` — algorithm scenario harness |
| 11 | Dispatch | Select Scanline for arrays in memory | `Valid Execution`; V&V: `Valid Threshold Sets` — algorithm scenario harness |
| 12 | Comparison | Less-than comparison in Direct and Scanline | V&V: `Valid Single Thresholds: Int`, `Float`, `Int Multi-Component` / `ArrayThreshold: <` |
| 13 | Comparison | Greater-than comparison in Direct and Scanline | V&V: `Valid Single Thresholds: Int`, `Float`, `Int Multi-Component` / `ArrayThreshold: >` |
| 14 | Comparison | Equal comparison in Direct and Scanline | V&V: `Valid Single Thresholds: Int`, `Float`, `Int Multi-Component` / `ArrayThreshold: ==` |
| 15 | Comparison | Not-equal comparison in Direct and Scanline | V&V: `Valid Single Thresholds: Int`, `Float`, `Int Multi-Component` / `ArrayThreshold: !=` |
| 16 | Comparison | Unknown comparison value → throw | *Not directly tested. No fixture supplies an invalid comparison enumerator.* |
| 17 | Comparison | Dispatch all 11 input scalar types | V&V: `Valid Execution, Input Array DataType` — ten numeric types and boolean |
| 18 | Output | Dispatch all 11 mask scalar types | V&V: `Valid Execution, Mask DataType` — ten numeric types; `Valid Execution` — boolean |
| 19 | Comparison | Read a selected nonzero component | V&V: `Valid Single Thresholds: Int Multi-Component` — indices 0, 1 and 2 |
| 20 | Tree | First child seeds the result; later children combine with it | V&V: `Valid Threshold Sets` — `ArraySet 1` through `ArraySet 7` |
| 21 | Tree | Combine child results with AND | V&V: `Valid Threshold Sets` / `ArraySet 1`, `ArraySet 3` |
| 22 | Tree | Combine child results with OR | V&V: `Valid Threshold Sets` / `ArraySet 2`, `ArraySet 4` |
| 23 | Tree | Invert or retain each leaf result | V&V: `Valid Single Thresholds: Int`, `Float`, `Int Multi-Component` — both inversion states |
| 24 | Tree | Invert a nested set after evaluating its children | V&V: `Valid Threshold Sets` / `ArraySet 5`, `ArraySet 7: leaf + inverted nested set` |
| 25 | Tree | Invert the completed root result | `Top-level inversion is logical`; `Nested sets compose before root inversion` |
| 26 | Tree | Evaluate mixed leaf and nested-set siblings | V&V: `Valid Threshold Sets` / `ArraySet 6: leaf + nested set`, `ArraySet 7: leaf + inverted nested set` |
| 27 | Output | Use default 1/0 values or enable both custom values | `Valid Execution`; V&V: `Valid Execution - Custom Values` — 25/10 for ten numeric types |
| 28 | Output | Enable only custom TRUE during execution | *Not directly tested. Successful custom-value fixtures enable both options.* |
| 29 | Output | Enable only custom FALSE during execution | *Not directly tested. Successful custom-value fixtures enable both options.* |
| 30 | Direct cancellation | Cancel before evaluation; preserve output | `Direct and Scanline honor cancellation without completing output` / `Direct` |
| 31 | Direct cancellation | Cancel during leaf evaluation, recursion, merge, inversion or output writes | *Not directly tested. The Direct cancellation fixture sets the flag before entry.* |
| 32 | Scanline I/O | Read and write a short chunk | `Direct and Scanline evaluate the same nested tree` — 20 tuples |
| 33 | Scanline I/O | Process a full 65,536-tuple chunk and a later tail | *Not directly tested. Current fixtures have at most 20 tuples.* |
| 34 | Scanline I/O | Propagate an input bulk-read error through recursive evaluation | `Scanline propagates bulk I/O failures` / `input bulk read` — error `-73001` |
| 35 | Scanline I/O | Propagate an output bulk-write error | `Scanline propagates bulk I/O failures` / `output bulk write` — error `-73002` |
| 36 | Scanline cancellation | Cancel after reading input; return before writing output | `Direct and Scanline honor cancellation without completing output` / `Scanline` |
| 37 | Scanline cancellation | Cancel at node entry or before the first chunk | *Not directly tested. The Scanline fixture cancels after its first read.* |
| 38 | Scanline cancellation | Cancel during child iteration, merge, inversion or between completed chunks | *Not directly tested. The cancellation fixture has one leaf and one short chunk.* |
| 39 | Tree | Empty nested set yields false, or true when inverted | *Not directly tested. The empty root fixture fails preflight; no valid tree contains an empty child set.* |
| 40 | Output | Zero input tuples → no output writes | *Not directly tested. All valid fixtures contain tuples.* |
| 41 | Tree | Node is neither a leaf nor a set | *Not directly tested. Fixtures use only the two supported threshold classes.* |
| 42 | Scanline merge | The internal `insertThresholdChunk` inversion argument is true | *Not directly tested. Its only caller passes false; node inversion occurs separately.* |
| 43 | SIMPL conversion | Select Advanced conversion by UUID | `SIMPL Backwards Compatibility` / `SIMPL 6.5 (UUID)` |
| 44 | SIMPL conversion | Select Advanced conversion by class name | `SIMPL Backwards Compatibility` / `SIMPL 6.4 (Filter_Name)` |
| 45 | SIMPL conversion | Select basic flat-list conversion | *Not directly tested. Both compatibility fixtures identify the Advanced filter.* |
| 46 | SIMPL conversion | Preserve nested sets, child union operators and inversion | V&V: `SIMPL Nested Set Conversion` |
| 47 | SIMPL conversion | Convert a supplied `ScalarType` | `SIMPL Backwards Compatibility` — fixtures supply the key; assertions check identity and output name |
| 48 | SIMPL conversion | Use default mask type when `ScalarType` is absent | V&V: `SIMPL Nested Set Conversion` — fixture omits the key; assertions check the threshold tree |
| 49 | SIMPL conversion | Reject a basic parameter that is not an array | *Not directly tested. No malformed basic conversion fixture exists.* |
| 50 | SIMPL conversion | Reject an Advanced parameter that is not an object or whose thresholds are not an array | *Not directly tested. No malformed Advanced conversion fixture exists.* |
| 51 | Dispatch | Select Scanline when an input or the output uses an external store | *Not directly tested. These test files construct stores in memory.* |

## Test inventory

The two files contain 27 test declarations. All pass in CI. Template type counts and dynamic sections are stated in Notes.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::MultiThresholdObjects: Valid Execution` | kept | Checks 20-tuple float and integer masks, output allocation and in-memory storage. |
| `SimplnxCore::MultiThresholdObjects: Top-level inversion is logical` | kept | Checks `NOT(i > 10)` for all 20 tuples with boolean output. |
| `SimplnxCore::MultiThresholdObjects: Nested sets compose before root inversion` | kept | Checks `NOT(i > 2 AND (i > 10 OR i < 5))` for all 20 tuples. |
| `SimplnxCore::MultiThresholdObjects: Scanline propagates bulk I/O failures` | kept | Injects bulk-read and bulk-write failures and checks errors `-73001` and `-73002`. |
| `SimplnxCore::MultiThresholdObjects: Direct and Scanline evaluate the same nested tree` | kept | Compares Direct and Scanline outputs for an inverted nested tree with component selection and custom values 19/4. This is a consistency check. |
| `SimplnxCore::MultiThresholdObjects: Direct and Scanline honor cancellation without completing output` | kept | Checks unchanged output after cancellation before Direct entry and after the first Scanline read. |
| `SimplnxCore::MultiThresholdObjects: Valid Execution - Custom Values` | kept | Template case over ten numeric output types; checks TRUE=25 and FALSE=10 for `Int32 > 15`. |
| `SimplnxCore::MultiThresholdObjects: Invalid Execution` | kept | Checks empty thresholds, a missing path, an invalid component index and unequal tuple counts. |
| `SimplnxCore::MultiThresholdObjects: Invalid Execution - Out of Bounds Custom Values` | kept | Template case over nine numeric types; checks custom TRUE and FALSE below the minimum and above the maximum. |
| `SimplnxCore::MultiThresholdObjects: Invalid Execution - Boolean Custom Values` | kept | Checks rejection of custom TRUE or FALSE with boolean output. |
| `SimplnxCore::MultiThresholdObjects: Valid Execution, DataType` | kept | Checks threshold output values for ten numeric mask types. |
| `SimplnxCore::MultiThresholdObjects: Valid Execution - Multicomponent` | kept | Checks component 1 of a three-component array, including the zero tuple and alternating signs. |
| `SimplnxCore::MultiThresholdObjectsFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check converted filter identity and output name; both fixtures use the Advanced filter. |
| `SimplnxCore::MultiThresholdObjects V&V: Exemplar Single Thresholds: Int` | kept | Checks the literal eight-value mask `[0,0,0,0,0,1,1,1]` for `Int32 > 4`. |
| `SimplnxCore::MultiThresholdObjects V&V: Exemplar Single Thresholds: Float` | kept | Checks the literal eight-value mask `[0,1,0,0,0,0,0,0]` for `Float32 == 0.02`. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Single Thresholds: Int` | kept | Sweeps four operators, eight thresholds and both inversion states. The oracle casts the threshold to int32, including 5.5. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Single Thresholds: Float` | kept | Sweeps four operators, six thresholds and both inversion states. The oracle rounds both operands to float32. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Single Thresholds: Int Multi-Component` | kept | Sweeps four operators, eight thresholds, both inversion states and component indices 0, 1 and 2. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Threshold Sets` | kept | Seven tree fixtures check AND, OR, nested sets and inversion. ArraySet 6 and ArraySet 7 add regressions for D1 and D2. |
| `SimplnxCore::MultiThresholdObjects V&V: Invalid Execution` | kept | Checks empty thresholds, a missing path, an invalid component index and unequal tuple counts. |
| `SimplnxCore::MultiThresholdObjects V&V: Invalid Execution - Out of Bounds Custom Values` | kept | Template case over nine numeric types; checks all four custom-value range errors. |
| `SimplnxCore::MultiThresholdObjects V&V: Invalid Execution - Boolean Custom Values` | kept | Checks rejection of custom TRUE or FALSE with boolean output. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Execution, Mask DataType` | kept | Checks ten numeric output types. The fixture split selects both false and true values, so an unchanged zero mask fails. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Execution, Input Array DataType` | kept | Checks all eleven input types against an int8 mask, including a separate boolean expectation. |
| `SimplnxCore::MultiThresholdObjectsFilter V&V: SIMPL Nested Set Conversion` | new-for-V&V | Checks converted leaf comparisons, nested grouping, inversion and OR preservation for D4. It checks arguments, not execution output. |
| `SimplnxCore::MultiThresholdObjectsFilter V&V: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check converted identity and output name for the Advanced filter. |
| `SimplnxCore::MultiThresholdObjects V&V: Valid Execution - Custom Values` | kept | Template case over ten numeric mask types; checks TRUE=25 and FALSE=10 for `Int32 > 3`, plus inherited tuple dimensions. |

## Exemplar archive

- **Archive:** None. Both test files construct their array fixtures in memory; `src/Plugins/SimplnxCore/test/CMakeLists.txt` registers no archive for this filter.
- **SHA512:** Not applicable; no archive is registered.
- **Provenance:** Not applicable; expected masks are defined in the test source.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/MultiThresholdObjectsFilter.md` for the root cause, affected users and recommendation. This table lists Part 1 only.

| Deviation | Observed difference |
|-----------|---------------------|
| `MultiThresholdObjectsFilter-D3` | SIMPLNX accepts a selected component of a multi-component array; both legacy filters require scalar arrays. |
| `MultiThresholdObjectsFilter-D5` | SIMPLNX offers selectable mask types and defaults to uint8; both legacy filters create boolean masks. |
| `MultiThresholdObjectsFilter-D6` | SIMPLNX offers custom TRUE and FALSE values; both legacy filters write boolean values. |
