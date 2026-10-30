# V&V Report: ComputeFeatureNeighborMisorientationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `0b68fe25-b5ef-4805-ae32-20acb8d4e823` |
| SIMPLNX Human Name | Compute Feature Neighbor Misorientations |
| DREAM.3D v6.5.171 equivalent | `FindMisorientations` (UUID `286dd493-4fea-54f4-b59e-459dd13bbe57`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-02; second engineer: Nathan Young, 2026-06-09 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes. The port of DREAM.3D v6.5.171 `FindMisorientations` corrects the averaging divisor and checks phase bounds. |
| Oracle (confirmed) | Classes 1 and 4: three analytical fixtures and one invariant fixture in `test/ComputeFeatureNeighborMisorientationsTest.cpp`. All pass in CI. |
| Code paths enumerated | 8 of 17 exercised. Gaps include cancellation, disabled averaging, zero-divisor output assertions, other Laue classes, and storage-boundary execution. |
| Tests today | 8 test cases: 3 analytical, 1 invariant, 2 validation, 1 conversion, and 1 hidden storage-boundary case. |
| Exemplar archive | `6_6_stats_test_v2.tar.gz` is retired for this filter's tests. Inline analytical fixtures define the expected outputs. |
| Legacy comparison | Not run on the current algorithm. The DREAM.3D v6.5.171 comparison dated 2026-09-17 predates the phase-bounds fix. |
| Bug flags | None |
| V&V phase | Oracle fixtures, source review, and defect corrections are complete. A legacy comparison with the current algorithm remains open. |

## Summary

`ComputeFeatureNeighborMisorientationsFilter` calculates neighbor misorientation angles in degrees when the two features have the same valid Laue class. It can also calculate the mean of those angles, excluding incompatible neighbors. Three Class 1 fixtures and one Class 4 fixture check the outputs; the divisor and phase-bounds defects are fixed.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* DREAM.3D v6.5.171 `FindMisorientations::execute()` and the current algorithm use the same nested loops and crystal-structure gate. SIMPLNX corrects the averaging divisor and checks phase bounds.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Cubic rotations about Z give 5° and 10° neighbor angles and a 7.5° mean; incompatible crystal structures give NaN entries, and the invariant fixture checks finite-entry bounds and the mean formula.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeFeatureNeighborMisorientationsTest.cpp` — `Class 1 - Single Phase Two Neighbors`, `Class 1 - Mixed Phase Neighbors (exposes divisor bug)`, `Class 1 - Mismatch Last Order`, and `Class 4 - Invariants`; 4 fixtures. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-06-09.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureNeighborMisorientationsFilter-D1`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureNeighborMisorientationsFilter-D1` | Resetting the divisor for each neighbor discarded earlier exclusions and made the mean depend on neighbor order. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter initializes the divisor once per feature and decrements it for each incompatible neighbor. |
| `ComputeFeatureNeighborMisorientationsFilter-D3` | A negative or out-of-range phase index could read outside Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter checks current and neighbor phase indices before each ensemble lookup and returns `-34502` for an invalid index. |

## Code path coverage

8 of 17 paths exercised. The count uses the supplied in-core CI evidence and the assertions in the current tests.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureNeighborMisorientations.cpp` (126 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureNeighborMisorientationsFilter.cpp` (217 lines).

Test names below omit the common `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter:` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Per-feature | Cancellation → return before calculation | *Not directly tested. No fixture injects cancellation.* |
| 2 | Per-neighbor | Same valid cubic Laue class → calculate the angle and accumulate it | `Class 1 - Single Phase Two Neighbors` |
| 3 | Per-neighbor | Different Laue classes → NaN entry and divisor decrement | `Class 1 - Mixed Phase Neighbors (exposes divisor bug)`; `Class 1 - Mismatch Last Order` |
| 4 | Finalize | Nonzero divisor → divide the sum by the compatible-neighbor count | All three `Class 1` cases; `Class 4 - Invariants` |
| 5 | Finalize | No neighbors or no compatible neighbors → NaN average | *Not directly tested. Fixtures execute empty lists without checking their averages; no test asserts an all-incompatible list.* |
| 6 | Preflight | Feature tuple counts differ → error `-34501` | `Preflight Error - Feature array tuple count mismatch (-34501)` |
| 7 | Per-feature | Current phase is negative or beyond the ensemble array → error `-34502` | `Phase Index Bounds` — `Current Feature Phase returns an error`, values -1 and 2 |
| 8 | Per-neighbor | Neighbor phase is negative or beyond the ensemble array → error `-34502` | `Phase Index Bounds` — `Neighbor Feature Phase returns an error`, values -1 and 2 |
| 9 | Storage | Quaternion data crosses a 65,536-tuple chunk boundary into a one-tuple tail | *Not directly tested. The hidden `genuine HDF5 1MiB-chunk tail oracle` supplies assertions for a separate storage configuration.* |
| 10 | Optional output | Averaging disabled → omit the average array and calculate lists only | *Not directly tested. Execution fixtures enable averaging.* |
| 11 | Preflight | Quaternion component count is not four → error `-34500` | *Not directly tested. Parameter validation already requires four components.* |
| 12 | Per-feature | Skip background feature zero | `Phase Index Bounds` — `Feature zero is ignored` checks successful execution with invalid background phases |
| 13 | Per-neighbor | Different phase IDs with the same valid Laue class → calculate the angle | *Not directly tested. The mixed-phase fixtures use different Laue classes.* |
| 14 | Per-neighbor | Equal Laue indices outside the operator range → NaN entry | *Not directly tested. No fixture gives a participating pair equal invalid Laue indices.* |
| 15 | Per-neighbor | Same valid non-cubic Laue class → select that orientation operator | *Not directly tested. Test angle assertions use cubic pairs; hexagonal features only supply incompatible neighbors.* |
| 16 | Output | Copy calculated lists to the output NeighborList in input order | All three `Class 1` cases assert list lengths and entries |
| 17 | Loop limits | Only background feature exists → no calculation or output-copy iteration | *Not directly tested. Every fixture contains non-background features.* |

## Test inventory

All names are in `src/Plugins/OrientationAnalysis/test/ComputeFeatureNeighborMisorientationsTest.cpp`. The seven ordinary cases run in the supplied CI configuration. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Preflight Error - Feature array tuple count mismatch (-34501)` | kept | Ten quaternion and neighbor-list tuples versus nine phase tuples; expects preflight error `-34501`. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check identity, comments, and all seven converted arguments. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Single Phase Two Neighbors` | new-for-V&V | Four feature tuples including background; expects the focal list `[5°, 10°]` and mean 7.5°, within `1e-3°`. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Mixed Phase Neighbors (exposes divisor bug)` | new-for-V&V | Five feature tuples; neighbor order `[2, 4, 3]` gives `[5°, NaN, 10°]` and mean 7.5°. Detects D1. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 1 - Mismatch Last Order` | new-for-V&V | Neighbor order `[2, 3, 4]` gives `[5°, 10°, NaN]` and mean 7.5°. Controls for neighbor order. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Class 4 - Invariants` | new-for-V&V | Neighbor order `[4, 2, 3]`; focal rotations are 0°, 7.5°, and 12°. Checks finite entries in `[0°, 62.8°]`, a positive finite-entry count, and the mean within `1e-4°`. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: genuine HDF5 1MiB-chunk tail oracle` | new-for-V&V | Hidden (`[.OocStoreContract]`). Requires HDF5 stores and a 65,536-tuple quaternion chunk; asserts list and mean values of 15° and 30° across the boundary. |
| `OrientationAnalysis::ComputeFeatureNeighborMisorientationsFilter: Phase Index Bounds` | kept | Tests phases -1 and 2 on current and neighbor features, expecting `-34502`; also checks that invalid background phases do not prevent execution. |

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz` — retired for this filter's tests; current expectations are inline.
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureNeighborMisorientationsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureNeighborMisorientationsFilter.md` for root causes, affected users, and recommendations. Only Part 1 entries appear here.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureNeighborMisorientationsFilter-D1` | SIMPLNX excludes every incompatible neighbor from the averaging divisor; DREAM.3D v6.5.171 resets that divisor inside the neighbor loop. |
| `ComputeFeatureNeighborMisorientationsFilter-D2` | Quaternion arithmetic and library precision change individual angles and their means. |
