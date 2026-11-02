# V&V Report: ComputeCAxisLocationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `a51c257a-ddc1-499a-9b21-f2d25a19d098` |
| SIMPLNX Human Name | Compute C-Axis Locations |
| DREAM.3D v6.5.171 equivalent | `FindCAxisLocations` (SIMPL UUID `68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Jared Duffey, 2026-07-31; second engineer: Nathan Young, 2026-08-06 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `FindCAxisLocations` in DREAM.3D v6.5.171. SIMPLNX preserves the rotation equation and adds phase validation and non-hexagonal handling. |
| Oracle (confirmed) | Class 1 analytical vectors are encoded in 15 orientation fixtures and a mixed-phase companion in `test/ComputeCAxisLocationsTest.cpp`. All pass in CI. |
| Code paths enumerated | 11 of 20 exercised. Hexagonal-Low, phase zero, cancellation, empty input, chunk-boundary execution and transfer failures are outside the stated coverage. |
| Tests today | 7 ordinary test cases cover analytical values, mixed phases, diagnostics, phase bounds and SIMPL conversion. One additional hidden test defines a 65,537-cell boundary fixture. |
| Exemplar archive | None used by the current tests; inputs and expected values are defined in C++. `caxis_data.tar.gz` is retired for this filter. |
| Legacy comparison | Not run on the current algorithm. The available DREAM.3D v6.5.171 comparisons are dated 2026-07-31 and 2026-09-17; both precede the algorithm change. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`ComputeCAxisLocationsFilter` computes the C-axis direction in the sample reference frame for each hexagonal cell. Class 1 analytical fixtures check the rotation, normalization, whole-vector sign reversal and mixed-phase NaN output. The filter now rejects invalid phase indices before the lookup.

## Algorithm Relationship

*Classification:* Port

*Evidence:* DREAM.3D v6.5.171 `FindCAxisLocations::execute()` and SIMPLNX `ComputeCAxisLocations::operator()()` transpose the quaternion rotation matrix, apply it to `[0, 0, 1]`, normalize the vector and reverse all components when Z is negative.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Hand-derived rotations give 15 expected vectors, including an oblique rotation with three distinct components; the mixed-phase fixture requires analytical vectors for hexagonal cells and NaN for cubic cells.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp::OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle` — 15 fixtures; `Class 1 Oracle - Mixed hexagonal and non-hexagonal phases` reuses them with alternating phases. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-08-06.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeCAxisLocationsFilter-D4` | A negative or out-of-range cell phase could cause an out-of-bounds read from Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | The filter checks the phase before the lookup and returns error `-3524` for an invalid phase index. |

## Code path coverage

11 of 20 paths exercised. Coverage below uses the ordinary test cases; each remaining path states its gap.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeCAxisLocations.cpp` (138 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeCAxisLocationsFilter.cpp` (147 lines).

Test names below omit the common `OrientationAnalysis::ComputeCAxisLocationsFilter:` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Cell tuple counts differ → error `-3520` | `Preflight Error - Cell array tuple count mismatch (-3520)` |
| 2 | Preflight | Valid tuple counts → create a three-component output and emit warning `-3521` | `Not all hexagonal phases warning`; `Class 1 Oracle` |
| 3 | Ensemble scan | No hexagonal entry after ensemble index zero → error `-3522` | `No hexagonal phases error` |
| 4 | Ensemble scan | Hexagonal and non-hexagonal entries coexist → warning `-3523` | `Not all hexagonal phases warning`; `Class 1 Oracle - Mixed hexagonal and non-hexagonal phases` |
| 5 | Chunk loop | Cancellation → return before the next chunk | *Not directly tested. No fixture injects cancellation.* |
| 6 | Per-cell calculation | Hexagonal-High → calculate and normalize the C-axis vector | `Class 1 Oracle` |
| 7 | Per-cell calculation | Z is negative → reverse all three components | `Class 1 Oracle` — 180° rotations about X and Y |
| 8 | Per-cell calculation | Non-hexagonal cell → write three NaNs | `Not all hexagonal phases warning`; `Class 1 Oracle - Mixed hexagonal and non-hexagonal phases` |
| 9 | Per-cell calculation | Hexagonal-Low → calculate and normalize the C-axis vector | *Not directly tested. All hexagonal fixtures use Hexagonal-High.* |
| 10 | Phase validation | Negative cell phase → error `-3524` | `Phase Index Bounds` — phase -1 |
| 11 | Phase validation | Cell phase reaches or exceeds the ensemble size → error `-3524` | `Phase Index Bounds` — phase 2 with two ensemble tuples |
| 12 | Phase validation | Phase zero → use Crystal Structures entry zero | *Not directly tested. Successful fixtures use positive cell phases.* |
| 13 | Per-cell calculation | Z is zero or positive → keep the vector sign | `Class 1 Oracle` — identity, 90° and 45° rotations |
| 14 | Bulk transfer | Read the ensemble cache and cell buffers; write a partial chunk | `Class 1 Oracle`; `Class 1 Oracle - Mixed hexagonal and non-hexagonal phases` |
| 15 | Chunk loop | Process a full 65,536-cell chunk and a one-cell tail | *Not directly tested. The ordinary fixtures do not cross a chunk boundary; the hidden boundary fixture requires HDF5 stores.* |
| 16 | Chunk loop | Zero cells → skip the loop | *Not directly tested. No fixture supplies empty cell arrays.* |
| 17 | Bulk transfer | Ensemble read fails → return the transfer error | *Not directly tested. No fixture injects an ensemble read failure.* |
| 18 | Bulk transfer | Quaternion read fails → return the transfer error | *Not directly tested. No fixture injects a quaternion read failure.* |
| 19 | Bulk transfer | Phase read fails → return the transfer error | *Not directly tested. No fixture injects a phase read failure.* |
| 20 | Bulk transfer | Output write fails → return the transfer error | *Not directly tested. No fixture injects an output write failure.* |

## Test inventory

The seven ordinary test cases run in the in-core builds on Linux, macOS and Windows. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeCAxisLocationsFilter: Preflight Error - Cell array tuple count mismatch (-3520)` | kept | Checks error `-3520` for 10 quaternion tuples and 9 phase tuples. |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: No hexagonal phases error` | new-for-V&V | Checks execution error `-3522` when the only non-background ensemble is cubic. |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: Not all hexagonal phases warning` | new-for-V&V | Checks warnings `-3521` and `-3523` and all 30 output components for NaN. |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle` | new-for-V&V | Checks all 45 components of 15 hand-derived vectors with `Approx(...).margin(1e-7f)`, including sign reversal. |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: Class 1 Oracle - Mixed hexagonal and non-hexagonal phases` | new-for-V&V | Checks 24 analytical components for 8 hexagonal cells, 21 NaNs for 7 cubic cells and warning `-3523`. |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: SIMPL Backwards Compatibility` | kept | Checks filter UUID, empty comments, quaternion path and output name in the `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` dynamic sections. |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: Phase Index Bounds` | new-for-V&V | Checks error `-3524` for phases -1 and 2 with two ensemble tuples (D4). |
| `OrientationAnalysis::ComputeCAxisLocationsFilter: genuine HDF5 65536-page tail oracle` | new-for-V&V | Hidden. Requires HDF5 input/output stores; checks distinct literal vectors at indices 65,535 and 65,536, output shape and warning codes. |

## Exemplar archive

- **Archive:** None used by the current tests. `caxis_data.tar.gz` is retired for this filter and remains registered in the plugin's `test/CMakeLists.txt`.
- **SHA512:** `56468d3f248661c0d739d9acd5a1554abc700bf136586f698a313804536916850b731603d42a0b93aae47faf2f7ee49d4181b1c3e833f054df6f5c70b5e041dc` (retired archive; matches `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`).
- **Provenance:** Not applicable to the inline Class 1 fixtures; their inputs and expected values are in `src/Plugins/OrientationAnalysis/test/ComputeCAxisLocationsTest.cpp`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeCAxisLocationsFilter.md` for the root cause, affected users and recommendation for each Part 1 entry.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeCAxisLocationsFilter-D1` | SIMPLNX writes NaN for non-hexagonal cells; DREAM.3D v6.5.171 computes a vector for every quaternion. |
| `ComputeCAxisLocationsFilter-D2` | SIMPLNX returns error `-3522` when no non-background ensemble is hexagonal; DREAM.3D v6.5.171 executes without that restriction. |
| `ComputeCAxisLocationsFilter-D3` | SIMPLNX emits preflight warning `-3521` and mixed-ensemble warning `-3523`; DREAM.3D v6.5.171 emits neither warning. |
