# V&V Report: ComputeCAxisLocationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| DREAM3D 6.5.171 equivalent | `FindCAxisLocations` (SIMPL UUID `68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e`) - `Source/Plugins/OrientationAnalysis/OrientationAnalysisFilters/FindCAxisLocations.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Jared Duffey, 07-31-2026 |

## At a glance

A scannable dashboard for reviewers. Each row is one sentence to one short paragraph — enough that a reader can decide whether they need to read the long-form sections below.

| Aspect                 | Current state                                                                                                                |
|------------------------|------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | Port - The EbsdLib, matrix math, and SIMPL APIs have changed but the code is functionally identical. Addition of several error branches when the crystal structure type is not hexagonal. |
| Oracle (confirmed)     | Class 1 (Analytical) -  15 hand derived data fixtures |
| Code paths enumerated  | 7 of 8 paths exercised - only the filter cancelation path is untested |
| Tests today            | 5 test cases - 1 test with Class 1 Oracle, 2 error path tests, 1 warning path test, 1 SIMPL json backwards compatibility test |
| Exemplar archive       | None - removed test using circular oracle data from `caxis_data.tar.gz` |
| Legacy comparison      | Run 2026-07-31 against DREAM3D 6.5.171 using the 15 inline Class 1 fixtures and a shared serialized input. All 45 output float32 values were bit-identical, and the comparison artifacts were uploaded to OneDrive on 2026-07-31. |
| Bug flags              | None |
| V&V phase              | Ready for review |

For worked instances see `src/Plugins/OrientationAnalysis/vv/BadDataNeighborOrientationCheckFilter.md` and `src/Plugins/OrientationAnalysis/vv/ComputeAvgCAxesFilter.md` (on `topic/vv/compute_avg_caxis`).

## Summary

ComputeCAxisLocationsFilter determines the direction of the C-axis for each element, in the *sample reference frame*, by applying the quaternion of the element to the <001> direction, which is the C-axis for *Hexagonal* materials.

The filter is verified with a Class 1 (Analytical) oracle. The filter uses the quaternion to rotate the C-axis into the sample reference frame. This is done by converting the quaternion to a rotation matrix. Then the transpose of the matrix is used due to DREAM3D conventions (see `wrapping/python/docs/source/Reference_Frame_Notes.md`). Due to the transpose the sign of the third element may need to be flipped. A new test was added with handed verified data.

There were no deviations that affect the output found for hexagonal materials.

## Algorithm Relationship

*Classification*: Port

*Evidence*: Same loop with the same rotation equation used

- UUID changed from SIMPL and filter renamed to match "Compute" naming conventions.
- EbsdLib is now up to version 3 which has equivalent but changed API for orientations like quaternions. The internal matrix math API has also changed here to use EbsdLib and Eigen but is functionally identical.
- Added one error branch before main execution checking for at least one hexagonal phase
- Added one warning branch before main execution for when not all phases are hexagonal
- Added branch inside loop to set not hexagonal quaternions to NAN to identify them as invalid

*PR(s):* 

- **PR #576** ("FILTER: FindCAxisLocations & FindFeatureNeighborCAxisMisalignment filters") - Initial PR
- **PR #801** ("ENH: Rename complex to simplnx") - Library rename
- **PR #956** ("ENH: Rename Filters that start with Find/Generate/Calculate to Compute") - Filter rename
- **PR #1438** ("ENH: Microtexture related filter cleanup") - Header include changed
- **PR #1472** ("ENH: Update to EbsdLib 2.0.0 API") - EbsdLib 2.0.0 API update
- **PR #1582** ("ENH: Add missing cancel checks to lots of filters") - Added cancel check in loop

## Oracle

*Class:* **Class 1 (Analytical)**

*Applied:* Handed derived output of C-axis locations from quaternions. The expected outputs agree between DREAM3DNX, DREAM3D 6.5.171, and manual calculations (`v_passive ​= Rᵀv` with z component forced to positive). Includes 15 different orientations about x, y, and z at different angles. Using the previous formula, the exact form results were produced and compared against DREAM3D output.

*Encoded:* *`test/ComputeCAxisLocationsTest.cpp::"OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle"` - 15 fixtures, all pass.*

*Second-engineer review:* *Pending*

## Code path coverage

*7 of 8 paths exercised. The non-covered path is the cancellation branch which is not currently able to be tested for all filters*

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeCAxisLocations.cpp` (107 lines).

| #  | Phase           | Path                                              | Test case                                  |
|----|-----------------|---------------------------------------------------|--------------------------------------------|
| 1  | *Preflight* | Tuple validity check (-3520 error code) | "OrientationAnalysis::ComputeCAxisLocationsFilter: Preflight Error - Cell array tuple count mismatch (-3520)" |
| 2  | *Preflight* | Unconditional warning advising user to ensure their data contains hexagonal phases | "OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning" |
| 3  | *Execute* | No hexagonal phases check (-3522 error code) | "OrientationAnalysis::ComputeCAxisLocationsFilter: No hexagonal phases error" |
| 4  | *Execute* | Not all phases hexagonal check (-3523 warning code) | "OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning" |
| 5  | *Execute - per-cell* | Should cancel check | Not directly tested - no filter cancellation testing infrastructure |
| 6  | *Execute - per-cell* | Hexagonal C-axis location calculation path | "OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle" |
| 7  | *Execute - per-cell* | C-axis direction flip check | "OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle" |
| 8  | *Execute - per-cell* | Non-hexagonal NAN branch | "OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning" |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| "OrientationAnalysis::ComputeCAxisLocationsFilter: Valid Filter Execution" | retired | Circular oracle |
| "OrientationAnalysis::ComputeCAxisLocationsFilter: InValid Filter Execution" | retired | Superseded by more specific test "OrientationAnalysis::ComputeCAxisLocationsFilter: No hexagonal phases error" |
| "OrientationAnalysis::ComputeCAxisLocationsFilter: Preflight Error - Cell array tuple count mismatch (-3520)" | kept | Covers preflight error for tuple mismatch of phases and quats |
| "OrientationAnalysis::ComputeCAxisLocationsFilter: SIMPL Backwards Compatibility" | kept | Covers SIMPL json backwards compatibility |
| "OrientationAnalysis::ComputeCAxisLocationsFilter: No hexagonal phases error" | new-for-V&V | Covers no hexagonal phases branch |
| "OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning" | new-for-V&V | Covers non-hexagonal branch |
| "OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle" | new-for-V&V | Covers hand calculated quaternions which also agree with DREAM3D 6.5.171. Also covers the sign flip path. |

## Exemplar archive

No new exemplar archive was created for this V&V cycle: the Class 1 oracle is encoded entirely as inline expected values in the test source. Tests no longer use circular oracle `caxis_data.tar.gz`.

## Deviations from DREAM3D 6.5.171

The comparison was run on 2026-07-31 using the 15 quaternion fixtures from `"OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle"`. DREAM3D 6.5.171 serialized the shared input immediately before running `FindCAxisLocations`; the NX Debug pipeline read that same file and ran `ComputeCAxisLocationsFilter`. Nine shared input DataArrays remained identical in both outputs. The target arrays had identical float32 shape `(15, 3)`, 0 of 45 differing float32 words, and a maximum absolute NX-versus-6.5.171 difference of `0.0`; both outputs also satisfied the analytical oracle at absolute tolerance `1e-7`.

The CSV fixture, shared input, legacy and NX pipelines, output DREAM3D files, and comparison summary were uploaded to OneDrive on 2026-07-31.

- D1 - non-hexagonal cells: NaN (NX) vs meaningless-but-finite computed value (6.5.171). NX intentional improvement; no legacy fix warranted.
- D2 - no hexagonal phases present: hard error -3522 (NX) vs silent full execution (6.5.171).
- D3 - added warnings -3521 (preflight, unconditional) and -3523 (execute, mixed phases); legacy emits none.
