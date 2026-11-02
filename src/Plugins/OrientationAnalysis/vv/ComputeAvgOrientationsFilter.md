# V&V Report: ComputeAvgOrientationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `086ddb9a-928f-46ab-bad6-b1498270d71e` |
| SIMPLNX Human Name | Compute Feature Average Orientations |
| DREAM.3D v6.5.171 equivalent | `FindAvgOrientations` (OrientationAnalysis plugin, UUID `bf7036d8-25bd-540e-b6de-3a5ab0e42c5f`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-30; second engineer: Nathan Young, 2026-07-01 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to the Rodrigues method of `FindAvgOrientations` in DREAM.3D v6.5.171. SIMPLNX also provides von Mises-Fisher (vMF) and Watson methods, which have no legacy equivalent. |
| Oracle (confirmed) | Classes 1, 2 and 4: analytical Rodrigues values, EbsdLib directional estimates and invariants in `test/ComputeAvgOrientationsTest.cpp`. All pass in CI. |
| Code paths enumerated | 27 of 42 paths exercised. Gaps include two method combinations, invalid FeatureIds, external-sort contracts and defensive error paths. |
| Tests today | 15 test cases cover analytical values, reference estimates, method selection, phase handling, transfer failures, cancellation and SIMPL conversion. |
| Exemplar archive | None in use. `7_ComputeAvgOrientation_v2.tar.gz` was retired; the tests construct inputs and expected values in C++. |
| Legacy comparison | Not run on the current algorithm; the available DREAM.3D v6.5.171 comparison is dated 2026-09-17. Algorithm source changed after that run. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`ComputeAvgOrientationsFilter` calculates average feature orientations with Rodrigues, vMF or Watson methods. Analytical fixtures, EbsdLib reference values and invariants check the results. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* The legacy `FindAvgOrientations` and current Rodrigues path use nearest-symmetry quaternion accumulation; SIMPLNX adds feature-zero averaging, empty-feature cleanup and the optional vMF and Watson methods.

## Oracle

*Class:* 1 (Analytical), 2 (Reference — EbsdLib) and 4 (Invariant).

*Applied:* Hand-derived quaternion means check Rodrigues; fixed EbsdLib estimates check vMF and Watson; unit norm, symmetry, phase exclusion and empty-feature rules check invariants.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeAvgOrientationsTest.cpp::OrientationAnalysis::ComputeAvgOrientations: Rodrigues Analytical Oracle` has six feature fixtures; `vMF/Watson EbsdLib Reference Oracle` has one 22-quaternion reference feature and a background tuple. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-07-16 (oracle review recorded in the provenance sidecar).

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.1. DREAM3D-NX v7.4.2 contains the fix for `ComputeAvgOrientationsFilter-D3`, `ComputeAvgOrientationsFilter-D5`, `ComputeAvgOrientationsFilter-D6`, `ComputeAvgOrientationsFilter-D7`, `ComputeAvgOrientationsFilter-D8` and `ComputeAvgOrientationsFilter-D9`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeAvgOrientationsFilter-D3` | Empty features produced an invalid quaternion or divided by zero. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter finalizes tuple zero, writes identity and skips division for every empty feature. |
| `ComputeAvgOrientationsFilter-D5` | vMF/Watson gathering included phase-zero voxels that the counting pass excluded. | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. | The filter applies the same phase gate to counting and gathering. |
| `ComputeAvgOrientationsFilter-D6` | An invalid crystal-structure value indexed outside the orientation-operator vector. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter checks the operator index, skips unsupported input and returns warning `-54671`. |
| `ComputeAvgOrientationsFilter-D7` | An invalid positive phase indexed outside Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter checks the phase range, excludes the voxel and returns warning `-54672`. |
| `ComputeAvgOrientationsFilter-D8` | vMF/Watson workers accessed shared array stores concurrently. | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. | The filter runs the resident worker serially. |
| `ComputeAvgOrientationsFilter-D9` | The vMF/Watson feature loop did not check cancellation. | None. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. DREAM.3D v6.5.171 was not affected. | The filter checks cancellation before processing each feature. |

## Code path coverage

27 of 42 paths exercised. The table maps assertions on in-memory fixtures; external-sort provider contracts and defensive paths have separate gap rows.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgOrientations.cpp` (1499 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeAvgOrientationsFilter.cpp` (296 lines).

Test names below omit the common `OrientationAnalysis::ComputeAvgOrientations:` prefix.

| # | Path | Test case |
|---|------|-----------|
| 1 | Rodrigues only; select its output tuple count | `Direct and Scanline method parity` |
| 2 | vMF only; select its output tuple count | `Direct and Scanline method parity` |
| 3 | Watson only; select its output tuple count | `Direct and Scanline method parity` |
| 4 | vMF and Watson enabled, Rodrigues disabled | `vMF/Watson EbsdLib Reference Oracle`; `Direct and Scanline method parity` |
| 5 | All three methods enabled | `Rodrigues Analytical Oracle`; `Direct and Scanline method parity` |
| 6 | Rodrigues and vMF enabled, Watson disabled | *Not directly tested. No fixture selects this pair.* |
| 7 | Rodrigues and Watson enabled, vMF disabled | *Not directly tested. No fixture selects this pair.* |
| 8 | No method enabled: preflight `-54673` and runtime `-54670` | `No Method Enabled Error` |
| 9 | Different cell-array tuple counts: `-651` | `Cell Array Tuple Mismatch Error (-651)` |
| 10 | Negative or excessive FeatureId: `-5355` or `-5351` | *Not directly tested. This file has no invalid-FeatureId fixture.* |
| 11 | Positive-phase cells contribute, including FeatureId zero | `Direct and Scanline method parity` |
| 12 | Nonpositive phases do not contribute | `Rodrigues Analytical Oracle`; `vMF/Watson Ignores Phase-0 Voxels` checks phase zero. |
| 13 | Positive phase outside the ensemble range: warning `-54672` | `Unknown Crystal Structure and Out-Of-Range Phase Guards` |
| 14 | Unsupported crystal structure: warning `-54671` | `Unknown Crystal Structure and Out-Of-Range Phase Guards` |
| 15 | First Rodrigues cell resets the reference; later cells accumulate | `Rodrigues Analytical Oracle`; `Rodrigues Voxel Ordering Independence` |
| 16 | Cubic symmetry and quaternion sign reduction | `Rodrigues Cubic Symmetry Invariant` |
| 17 | Normalize a populated Rodrigues average and convert it to Euler angles | `Rodrigues Analytical Oracle` |
| 18 | Empty Rodrigues feature gives identity and zero Euler angles | `Rodrigues Analytical Oracle` |
| 19 | Empty directional feature retains NaN outputs | `zero-cell and empty-feature outputs` |
| 20 | Single directional sample bypasses EM and gives kappa zero | `Rodrigues Analytical Oracle` |
| 21 | Multiple directional samples call VMF and WAT estimators | `vMF/Watson EbsdLib Reference Oracle` |
| 22 | Highest-index valid phase selects directional feature symmetry | `highest-index valid phase selects feature symmetry` |
| 23 | Zero input cells with allocated feature tuples | `zero-cell and empty-feature outputs` |
| 24 | Full 65,536-cell chunk and partial tail preserve tuple placement | `cross-chunk original tuple ordering parity` |
| 25 | Scanline directional gathering without an external-sort provider | `bounded fallback without external sort` |
| 26 | External-sort grouping, including feature flush and final flush | *Not directly tested. These fixtures do not require an external-sort provider.* |
| 27 | Bulk input-read failure propagates | `Scanline failures and cancellation` — `bulk read failure` |
| 28 | Bulk output-write failure propagates | `Scanline failures and cancellation` — `bulk write failure` |
| 29 | Cancellation at Scanline entry preserves outputs | `Scanline failures and cancellation` — `entry cancellation` |
| 30 | Cancellation after a Rodrigues input read preserves outputs | `Scanline failures and cancellation` — `mid-scan cancellation` |
| 31 | Cancellation between Rodrigues output writes stops the second write | `Scanline failures and cancellation` — `cancellation between output writes` |
| 32 | Direct entry, Rodrigues finalization or resident directional-worker cancellation | *Not directly tested. No fixture injects cancellation at these points.* |
| 33 | Directional counting, gathering, estimation or output-write cancellation | *Not directly tested. Cancellation fixtures select Rodrigues only.* |
| 34 | Scanline rejects missing, wrongly typed or malformed input arrays: `-54674`/`-54675` | *Not directly tested. Fixtures use valid input arrays or fail preflight first.* |
| 35 | Scanline rejects missing or malformed output arrays: `-54670`/`-54674`/`-54675` | *Not directly tested. No enabled-method fixture removes or reshapes an output.* |
| 36 | Count, component-product or buffer-size overflow: `-54676` | *Not directly tested. Requires synthetic size metadata or inconsistent buffers.* |
| 37 | Gathered count differs from the counting pass: `-54678` | *Not directly tested. Requires inconsistent grouped data.* |
| 38 | Sorter creation, append, finish or read failure; null sorter `-54679` | *Not directly tested. No test case installs the declared failing-sorter helper.* |
| 39 | External-sort record-count mismatch or short read: `-54680` | *Not directly tested. Requires an injected sorter contract failure.* |
| 40 | Invalid sorted FeatureId or original tuple index: `-54681` | *Not directly tested. Requires an injected malformed record.* |
| 41 | Sorted group has no selected operator or is out of order: `-54677`/`-54682` | *Not directly tested. Requires an injected malformed group.* |
| 42 | Original tuple indices are not strictly increasing: `-54683` | *Not directly tested. Requires duplicate or reordered sort records.* |

## Test inventory

All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeAvgOrientations: Rodrigues Analytical Oracle` | new-for-V&V | Six feature tuples check identity, Rz90, Rz45, repeated orientations and empty features. Checks Euler invariants, directional single-sample shortcuts and NaN outputs. |
| `OrientationAnalysis::ComputeAvgOrientations: vMF/Watson EbsdLib Reference Oracle` | new-for-V&V | Checks both estimates for 22 cubic reference quaternions; component margin `5e-3`, kappa relative tolerance 2%. |
| `OrientationAnalysis::ComputeAvgOrientations: Rodrigues Cubic Symmetry Invariant` | new-for-V&V | Five symmetry/sign representations of Rz30 must average to Rz30 within `1e-5`. |
| `OrientationAnalysis::ComputeAvgOrientations: Rodrigues Voxel Ordering Independence` | new-for-V&V | Both `Ordering:` dynamic sections require the identity/Rz90 pair to average to Rz45. |
| `OrientationAnalysis::ComputeAvgOrientations: vMF/Watson Ignores Phase-0 Voxels` | new-for-V&V | Excludes invalid phase-zero samples from single-sample and EM features; checks D5. |
| `OrientationAnalysis::ComputeAvgOrientations: Unknown Crystal Structure and Out-Of-Range Phase Guards` | new-for-V&V | Checks warnings `-54671` and `-54672`, valid-feature values and excluded-feature identity/NaN values; checks D6 and D7. |
| `OrientationAnalysis::ComputeAvgOrientations: Direct and Scanline method parity` | kept | Five method combinations have independent single-orientation expectations. When both algorithms are selected, compares all enabled multi-sample output arrays. |
| `OrientationAnalysis::ComputeAvgOrientations: cross-chunk original tuple ordering parity` | kept | 65,538 cells put valid samples at indices 0, 65,536 and 65,537; checks their output feature tuples. Both-algorithm selection also compares multi-sample results. |
| `OrientationAnalysis::ComputeAvgOrientations: highest-index valid phase selects feature symmetry` | kept | Reverses two phases; expects Rz90 when the selected structure is valid, or NaNs and warning `-54671` when it is unknown. |
| `OrientationAnalysis::ComputeAvgOrientations: bounded fallback without external sort` | kept | Checks analytical outputs without a sorter and compares multi-sample results when both algorithms are selected. Returns early if a sorter is registered. |
| `OrientationAnalysis::ComputeAvgOrientations: Scanline failures and cancellation` | kept | Checks read/write error codes and unchanged output sentinels at three cancellation points. Assertions require selection of the Scanline scenario. |
| `OrientationAnalysis::ComputeAvgOrientations: zero-cell and empty-feature outputs` | kept | Zero cells and two features produce Rodrigues identity quaternions and six directional arrays filled with NaN. |
| `OrientationAnalysis::ComputeAvgOrientations: No Method Enabled Error` | new-for-V&V | Asserts preflight `-54673` and direct algorithm backstop `-54670`. |
| `OrientationAnalysis::ComputeAvgOrientations: Cell Array Tuple Mismatch Error (-651)` | new-for-V&V | Asserts preflight error `-651` for different cell-array tuple counts. |
| `OrientationAnalysis::ComputeAvgOrientationsFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` dynamic sections check converted UUID and argument values. |

## Exemplar archive

- **Archive:** None in use. `7_ComputeAvgOrientation_v2.tar.gz` was retired. The tests now construct inputs and expected values in C++.
- **SHA512:** Not applicable; the current `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` has no registration for this archive. The provenance sidecar retains the hash from its original registration.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeAvgOrientationsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeAvgOrientationsFilter.md` for the root cause, affected users and recommendation. D1 remains in the sidecar as a retired candidate.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeAvgOrientationsFilter-D2` | SIMPLNX averages positive-phase cells in feature zero; DREAM.3D v6.5.171 excludes them. |
| `ComputeAvgOrientationsFilter-D3` | SIMPLNX gives empty features identity quaternions; DREAM.3D v6.5.171 leaves tuple zero invalid and divides other empty features by zero. |
| `ComputeAvgOrientationsFilter-D4` | Different quaternion and Euler implementations produce float32 rounding differences. |
