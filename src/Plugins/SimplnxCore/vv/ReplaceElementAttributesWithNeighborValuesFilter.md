# V&V Report: ReplaceElementAttributesWithNeighborValuesFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `65128c53-d3be-4a69-a559-32a48d603884` |
| SIMPLNX Human Name | Replace Element Attributes with Neighbor (Threshold) |
| DREAM.3D v6.5.171 equivalent | `ReplaceElementAttributesWithNeighborValues` (OrientationAnalysis plugin, SIMPL UUID `17410178-4e5f-58b9-900e-8194c69200ab`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-07-21; second engineer: Matthew Marine, 2026-07-22 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `ReplaceElementAttributesWithNeighborValues` in DREAM.3D v6.5.171. SIMPLNX retains the face-neighbor rules, preserves the comparison value's type, and transfers numeric tuples one Z slice at a time. |
| Oracle (confirmed) | Classes 1 and 4 use four synthetic sections in `test/ReplaceElementAttributesWithNeighborValuesTest.cpp`. All pass in CI. |
| Code paths enumerated | 20 of 30 paths exercised. Gaps include cancellation, I/O failures, nonnumeric siblings, other comparison types, and a loop with no passing values. |
| Tests today | 4 test cases: synthetic replacement, archive regression, SIMPL conversion, and a hidden data generator. The synthetic case has four sections. |
| Exemplar archive | `6_6_replace_element_attributes_with_neighbor.tar.gz` is used by the archive regression. The test loads the same file as input and reference; the independent oracle is in the synthetic test. |
| Legacy comparison | Not run on the current algorithm. The comparison dated 2026-07-21 predates the change to slice transfers. |
| Bug flags | `ReplaceElementAttributesWithNeighborValuesFilter-D3` |
| V&V phase | COMPLETE. Two defects are fixed; the loop-termination defect remains open. |

## Summary

`ReplaceElementAttributesWithNeighborValuesFilter` replaces a cell that fails the threshold by copying numeric tuples from the best passing face neighbor. Four synthetic sections check analytical values, threshold saturation, and copying to a second array; all pass in CI. Two defects are fixed, but loop mode does not terminate when all comparison values fail the threshold.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; both implementations use six face neighbors, while SIMPLNX preserves the candidate type and transfers one Z slice at a time.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Hand-derived values on a 3×3×3 grid specify replacement of isolated bad cells and preservation of a center with no passing neighbor; invariants check threshold saturation and a changed marker.

*Encoded:* `src/Plugins/SimplnxCore/test/ReplaceElementAttributesWithNeighborValuesTest.cpp::SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Synthetic neighbor replacement` — four fixture sections. All pass in CI.

*Second-engineer review:* Matthew Marine, 2026-07-22.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains both fixes.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ReplaceElementAttributesWithNeighborValuesFilter-D1` | A float32 temporary could change the selected neighbor for a float64 or large-integer comparison array. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The best candidate retains the comparison array's numeric type. |
| `ReplaceElementAttributesWithNeighborValuesFilter-D2` | A nonnumeric sibling in the cell AttributeMatrix could cause a failed reference cast during replacement. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The filter copies numeric `IDataArray` siblings and skips other objects. |

## Code path coverage

20 of 30 paths exercised. Each uncovered path has a reason below.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReplaceElementAttributesWithNeighborValues.cpp` (368 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReplaceElementAttributesWithNeighborValuesFilter.cpp` (140 lines).

Rows that name a section refer to `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Synthetic neighbor replacement`. Each pass selects neighbors from a rolling comparison window and transfers each completed Z slice.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Loop | Cancel before a pass | *Not directly tested. Requires cancellation injection.* |
| 2 | Scan | A passing cell skips the neighbor search | `LessThan replaces low voxels and loops until none remain` |
| 3 | Scan | A failing cell increments the count and searches neighbors | `LessThan replaces low voxels and loops until none remain`; `GreaterThan replaces high voxels and loops until none remain` |
| 4 | Scan | Reject the -Z neighbor at the boundary | `LessThan replaces low voxels and loops until none remain` — cell 0 |
| 5 | Scan | Reject the -Y neighbor at the boundary | `LessThan replaces low voxels and loops until none remain` — cell 0 |
| 6 | Scan | Reject the -X neighbor at the boundary | `LessThan replaces low voxels and loops until none remain` — cell 0 |
| 7 | Scan | Reject the +X neighbor at the boundary | `LessThan replaces low voxels and loops until none remain` — cell 26 |
| 8 | Scan | Reject the +Y neighbor at the boundary | `LessThan replaces low voxels and loops until none remain` — cell 26 |
| 9 | Scan | Reject the +Z neighbor at the boundary | `LessThan replaces low voxels and loops until none remain` — cell 26 |
| 10 | Scan | A passing, better candidate becomes the selected source | `LessThan replaces low voxels and loops until none remain` |
| 11 | Scan | Reject a neighbor that fails the threshold | `bad voxel surrounded only by bad neighbors is not replaced (compare1=false skip)` |
| 12 | Scan | Reject a passing neighbor whose value equals the best value | `LessThan replaces low voxels and loops until none remain` — equal candidates at cell 13 |
| 13 | Copy | Copy a selected source into the comparison and marker arrays | `LessThan replaces low voxels and loops until none remain` — exact replaced values and changed center marker |
| 14 | Copy | Preserve cells with no selected source | `bad voxel surrounded only by bad neighbors is not replaced (compare1=false skip)` — center stays 0.1 |
| 15 | Loop | Repeat when loop mode is enabled and the pass counted failing cells | `LessThan replaces low voxels and loops until none remain` |
| 16 | Loop | Exit after a pass counts no failing cells | `LessThan replaces low voxels and loops until none remain`; `GreaterThan replaces high voxels and loops until none remain` |
| 17 | Loop | Stop after one pass when loop mode is disabled | `loop disabled performs a single pass`; `bad voxel surrounded only by bad neighbors is not replaced (compare1=false skip)` |
| 18 | Setup | Select LessThan and prefer larger passing values | `LessThan replaces low voxels and loops until none remain` |
| 19 | Setup | Select GreaterThan and prefer smaller passing values | `GreaterThan replaces high voxels and loops until none remain` |
| 20 | Slices | Read initial slices, rotate the window, read the next slice, and reset source marks after each transfer | `bad voxel surrounded only by bad neighbors is not replaced (compare1=false skip)`; both loop sections |
| 21 | Slices | Process a single Z slice without reading a second slice | *Not directly tested. Synthetic fixtures have three slices; the archive regression does not isolate this condition.* |
| 22 | I/O | Return a first-slice comparison read error | *Not directly tested. Requires an injected read failure.* |
| 23 | I/O | Return a second-slice comparison read error | *Not directly tested. Requires an injected read failure.* |
| 24 | I/O | Return a comparison read error after window rotation | *Not directly tested. Requires an injected read failure.* |
| 25 | I/O | Return a sibling tuple-transfer error | *Not directly tested. Requires an injected transfer read or write failure.* |
| 26 | Copy | Skip a sibling that is not an `IDataArray` | *Not directly tested. No fixture includes a nonnumeric sibling.* |
| 27 | Loop | Cancel after a completed pass | *Not directly tested. Requires cancellation injection.* |
| 28 | Dispatch | Compare boolean, integer, or float64 values instead of float32 | *Not directly tested. All executing test cases select a float32 comparison array.* |
| 29 | Loop | Repeat without progress when every value fails the threshold | *Not directly tested. An uncancelled fixture would not terminate; see `ReplaceElementAttributesWithNeighborValuesFilter-D3`.* |
| 30 | Preflight | Accept the comparison array and declare its parent contents modified | `Synthetic neighbor replacement` — preflight succeeds in all four sections |

## Test inventory

The test cases are in `src/Plugins/SimplnxCore/test/ReplaceElementAttributesWithNeighborValuesTest.cpp`. The three ordinary cases pass in CI. The generator is hidden.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Generate Test Data` | kept | Hidden (`[.GenerateTestData]`). Writes patterned 20×20×20 and 200×200×200 inputs with confidence, Euler-angle, and phase arrays; does not execute the filter. |
| `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter` | kept | Loads the same archive file as input and reference, runs LessThan at 0.1 with loop enabled, compares cell arrays, and checks tuple dimensions. This checks stability on the stored data. |
| `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: Synthetic neighbor replacement` | new-for-V&V | Four sections check LessThan, GreaterThan, one pass, and a bad center surrounded by bad neighbors. Checks all 27 values against the threshold in the loop sections, exact replacement values, and a changed center marker. |
| `SimplnxCore::ReplaceElementAttributesWithNeighborValuesFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check identity, comments, threshold, comparison, loop, and both input paths. Does not execute replacement. |

## Exemplar archive

- **Archive:** `6_6_replace_element_attributes_with_neighbor.tar.gz`
- **SHA512:** `319ebdf08b83ce5ec915afda8ee2af1e0952a3ce26b3e65a4171eb3125c7bc6613c3994610bf526f70413c00da9061c3e6c2d867643220d62faa8c3cd79a96cd`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ReplaceElementAttributesWithNeighborValues.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ReplaceElementAttributesWithNeighborValuesFilter.md` for the root cause, affected users, and recommendation. This table lists only Part 1.

| Deviation | Observed difference |
|-----------|---------------------|
| `ReplaceElementAttributesWithNeighborValuesFilter-D1` | SIMPLNX retains the comparison value's numeric type when it selects the best neighbor; DREAM.3D v6.5.171 rounds the candidate to float32. |
