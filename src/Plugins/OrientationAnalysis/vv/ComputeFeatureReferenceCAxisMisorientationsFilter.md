# V&V Report: ComputeFeatureReferenceCAxisMisorientationsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `16c487d2-8f99-4fb5-a4df-d3f70a8e6b25` |
| SIMPLNX Human Name | Compute Feature Reference C-Axis Misalignments |
| DREAM.3D v6.5.171 equivalent | `FindFeatureReferenceCAxisMisorientations` (SIMPL UUID `1a0848da-2edd-52c0-b111-62a4dc6d2886`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-06-11; second engineer: Nathan Young, 2026-06-23 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindFeatureReferenceCAxisMisorientations` in DREAM.3D v6.5.171. SIMPLNX adds hexagonal-phase gating, double-precision vector math, explicit NaN outputs and slice transfers. |
| Oracle (confirmed) | Classes 1 and 4 use analytical angles, feature statistics and invariants. Four cases in `test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp` encode these checks. All pass in CI. |
| Code paths enumerated | 21 of 29 exercised by the visible tests. Gaps include angles above 90°, cosine clamping, feature-zero statistics, transfer failures and cancellation. |
| Tests today | 14 test cases: 13 visible and 1 hidden slice-boundary case. The visible cases check analytical values, invalid inputs and SIMPL conversion. |
| Exemplar archive | `caxis_data.tar.gz` supplies input for `InValid Filter Execution`. The correctness oracle is defined in C++ fixtures. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None |
| V&V phase | Complete. The sign-off contains one engineer. |

## Summary

`ComputeFeatureReferenceCAxisMisorientationsFilter` calculates cell-to-feature c-axis angles and per-feature means and standard deviations. Class 1 analytical fixtures and Class 4 invariants check these outputs, including NaN for features with no hexagonal contributors. The current source excludes nonparticipating cells from statistics and validates cell tuple counts and feature indices before array access.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* The SIMPL conversion fixture preserves the legacy UUID; the DREAM.3D v6.5.171 source and current algorithm share the c-axis projection and feature aggregation, with the differences recorded in the sidecars.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Pure Bunge rotations `(0, Φ, 0)` give angles `abs(Φ_cell − Φ_avg)` folded to [0°, 90°]; hand calculations give feature means and population standard deviations for the hexagonal fixtures.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp::OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — Simple Hex Triple` and the three companion Class 1/Class 4 cases cover three base fixtures. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-06-23.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.0.0 contains the fix for `ComputeFeatureReferenceCAxisMisorientationsFilter-D1`. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureReferenceCAxisMisorientationsFilter-D3`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureReferenceCAxisMisorientationsFilter-D1` | Non-hexagonal cells contribute c-axis results that have no meaning under this filter's hexagonal-phase contract. | DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. | The filter skips non-hexagonal cells and returns NaN for a feature with no contributing cells. |
| `ComputeFeatureReferenceCAxisMisorientationsFilter-D3` | The algorithm discards its mixed-phase warning before returning. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The filter returns the result that contains warning `-9803`. |
| `ComputeFeatureReferenceCAxisMisorientationsFilter-D4` | An invalid phase indexes Crystal Structures before validation and can read outside the array. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | The filter checks participation and phase bounds before lookup and rejects an invalid positive participating phase with `-9804`. |

## Code path coverage

21 of 29 paths exercised. The count maps assertions in the 13 visible tests to source paths; the table lists the remaining paths and their gaps.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureReferenceCAxisMisorientations.cpp` (309 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureReferenceCAxisMisorientationsFilter.cpp` (205 lines).

Test names below omit `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: `. `Realistic Microstructure` refers to `Class 1 — Realistic Microstructure (exposes divide-by-zero)`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Cell tuple counts differ → error `-9800` | `Preflight Error - Cell array tuple count mismatch (-9800)` |
| 2 | Preflight | Valid input → three output actions and warning `-9801` | `Class 1 — Simple Hex Triple` — valid actions and output values |
| 3 | Execution validation | No hexagonal phase → error `-9802` | `InValid Filter Execution` — checks execution failure; `Empty arrays` — checks `-9802` for empty Crystal Structures |
| 4 | Execution validation | Mixed phases → warning `-9803` | `Realistic Microstructure` — checks a nonempty warning list, not the specific warning |
| 5 | Execution validation | All phases are hexagonal → continue | `Class 1 — Simple Hex Triple` |
| 6 | Cell angle | Hexagonal-High cell → calculate and accumulate angle | `Class 1 — Simple Hex Triple`; `Realistic Microstructure` |
| 7 | Cell angle | Hexagonal-Low cell → calculate and accumulate angle | `Statistics exclude nonparticipating cells` — Hexagonal-High and Hexagonal-Low cells give mean 20° and standard deviation 10° |
| 8 | Cell angle | Non-hexagonal cell → zero output and no contribution | `Realistic Microstructure`; `Class 4 — Invariants` |
| 9 | Cell angle | Feature zero → skip the phase lookup | `Phase Index Bounds` — feature-zero section checks successful execution only |
| 10 | Cell angle | Negative phase → skip the phase lookup | `Phase Index Bounds` — nonpositive-phase section checks successful execution only |
| 11 | Cell angle | Phase zero → skip the angle calculation | `Statistics exclude nonparticipating cells` — checks zero cell output and no contribution to feature statistics |
| 12 | Cell angle | Positive participating phase exceeds the ensemble bounds → error `-9804` | `Phase Index Bounds` — checks error `-9804` |
| 13 | Cell angle | Angle is at most 90° → keep the angle | All three Class 1 cases |
| 14 | Cell angle | Angle is above 90° → replace it with `180° − angle` | *Not directly tested. All analytical angle differences are below 90°.* |
| 15 | Cell angle | Cosine is outside [-1, 1] → clamp it | *Not directly tested. No fixture forces a cosine outside the interval.* |
| 16 | Feature mean | Positive contributor count → divide sum by count | `Class 1 — Simple Hex Triple`; `Realistic Microstructure` |
| 17 | Feature mean | Zero contributor count → explicit quiet NaN | `Realistic Microstructure`; `Class 4 — Invariants` |
| 18 | Variance | Read cell angles again and accumulate squared differences only for positive FeatureIds and positive hexagonal phases | All three Class 1 cases; `Statistics exclude nonparticipating cells` |
| 19 | Feature standard deviation | Positive count → `sqrt(sum of squared differences / count)` | `Class 1 — Simple Hex Triple`; `Realistic Microstructure` |
| 20 | Feature standard deviation | Zero count → explicit quiet NaN | `Realistic Microstructure`; `Class 4 — Invariants` |
| 21 | Feature output | Feature-zero mean and standard deviation remain zero | *Not directly tested. No case asserts both feature-zero outputs.* |
| 22 | Slice transfers | Feature contributions span multiple slices; skipped cells reset to zero | `Large constant-angle feature precision` — checks statistics across two slices; `Statistics exclude nonparticipating cells` — checks skipped-cell zeros across three slices |
| 23 | Cancellation | Stop in either slice pass or either feature loop | *Not directly tested. No fixture injects cancellation.* |
| 24 | Input cache | Crystal Structures or Average C-Axes read fails → return error | *Not directly tested. No fixture injects a cache-read failure.* |
| 25 | First slice pass | FeatureIds, Phases or Quats read fails → return error | *Not directly tested. No fixture injects an input-slice failure.* |
| 26 | Second slice pass | FeatureIds, Phases or cell-angle read fails → return error | *Not directly tested. No fixture injects a second-pass read failure.* |
| 27 | Output | Cell-angle, feature-mean or feature-standard-deviation write fails → return error | *Not directly tested. No fixture injects an output-write failure.* |
| 28 | Preflight | Cell Feature Ids, Cell Phases and Cell Quaternions have equal tuple counts that differ from the selected geometry cell count → error `-9805` | `Geometry cell tuple counts` — all arrays have 4 or 12 tuples for 8 cells; matching arrays in other Attribute Matrices are accepted |
| 29 | Execution validation | FeatureId is negative or exceeds the Average C-Axes tuple bounds → error `-9806` | `Feature Id bounds before statistics` — rejects IDs -1 and 2 even with phase zero; `Empty arrays` — rejects cells with no Average C-Axes tuples |

## Test inventory

The 13 visible cases and one hidden case are in `src/Plugins/OrientationAnalysis/test/ComputeFeatureReferenceCAxisMisorientationsTest.cpp`.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — Simple Hex Triple` | new-for-V&V | Three angles `[5, 0, 5]`; mean `10/3`; population standard deviation `sqrt(50/9)`. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — Realistic Microstructure (exposes divide-by-zero)` | new-for-V&V | Checks 25 cell angles, five feature means and five standard deviations. The cubic feature has zero cell angles and NaN feature outputs; the source now writes NaN explicitly. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 1 — All-Identical Orientation Feature` | new-for-V&V | Checks five zero cell angles and zero feature mean and standard deviation. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Class 4 — Invariants` | new-for-V&V | Three sections check angle bounds and non-hexagonal zeros, the feature-mean formula, and NaN outputs for the cubic feature. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: InValid Filter Execution` | kept | Loads the archive, changes the phase to Cubic-High and checks execution failure and tuple dimensions. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Preflight Error - Cell array tuple count mismatch (-9800)` | kept | Uses 10 FeatureIds/Quats tuples and 9 phase tuples; checks preflight error `-9800`. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check UUID, comments and converted arguments. This case reads JSON fixtures. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: genuine HDF5 slice-boundary oracle` | new-for-V&V | Hidden. A 17×19×4 fixture checks angles `[10, 20, 30]`, mean 20, standard deviation `sqrt(200/3)`, and zero values in skipped cells across slices. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Phase Index Bounds` | kept | Checks `-9804` for a positive phase outside the ensemble and successful execution for a negative phase or feature zero. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Geometry cell tuple counts` | new-for-V&V | Checks `-9805` when all three cell arrays have 4 or 12 tuples for 8 geometry cells. Accepts 8 tuples in other Attribute Matrices; checks `-9800` when each input in turn is short, with array paths and counts in the messages. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Feature Id bounds before statistics` | new-for-V&V | Checks execution error `-9806` for FeatureIds -1 and 2 with two feature tuples, even with phase zero. Checks both array paths in the message and rejection before the cancellation guard. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Statistics exclude nonparticipating cells` | new-for-V&V | Hexagonal-High and Hexagonal-Low angles 10° and 30° give mean 20° and population standard deviation 10°. Checks zero cell outputs for phases 0 and -1, a cubic phase and feature zero; checks NaN statistics for an empty feature. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Large constant-angle feature precision` | new-for-V&V | Two million cells at 13.1° give mean 13.1° and zero population standard deviation within 0.00001°. Checks the first cell angle and feature statistics across two slices. |
| `OrientationAnalysis::ComputeFeatureReferenceCAxisMisorientationsFilter: Empty arrays` | new-for-V&V | Checks zero cell output tuples and NaN feature statistics for empty cells, error `-9806` for empty Average C-Axes, and error `-9802` for empty Crystal Structures. |

## Exemplar archive

- **Archive:** `caxis_data.tar.gz` — input for the invalid-execution case; analytical expectations are in the test source.
- **SHA512:** `56468d3f248661c0d739d9acd5a1554abc700bf136586f698a313804536916850b731603d42a0b93aae47faf2f7ee49d4181b1c3e833f054df6f5c70b5e041dc`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureReferenceCAxisMisorientationsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureReferenceCAxisMisorientationsFilter.md` for the source evidence, affected users and recommendations. Only Part 1 entries appear below.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureReferenceCAxisMisorientationsFilter-D1` | SIMPLNX skips non-hexagonal cells and returns NaN for features with no contributors. DREAM.3D v6.5.171 computes angles for any positive phase. |
| `ComputeFeatureReferenceCAxisMisorientationsFilter-D2` | Double-precision orientation math and variance accumulation change numerical rounding relative to DREAM.3D v6.5.171. |
