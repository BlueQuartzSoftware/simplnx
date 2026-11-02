# V&V Report: ComputeSchmidsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `b4681855-0a3d-4237-97f2-5aec509115c4` |
| SIMPLNX Human Name | Compute Schmid Factors |
| DREAM.3D v6.5.171 equivalent | `FindSchmids` (SIMPL UUID `e67ca06a-176f-58fc-a676-d6ee5553511a`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Michael Jackson, 2026-08-21; second engineer: pending |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindSchmids` in DREAM.3D v6.5.171. SIMPLNX retains the feature calculation with higher precision, input checks and corrected library outputs. |
| Oracle (confirmed) | Classes 1 and 4 use 12 analytical fixtures in `test/ComputeSchmidsTest.cpp`. All pass in CI. |
| Code paths enumerated | 20 of 25 exercised. Cancellation, the hidden storage fixture, the sentinel-only loop, other Laue classes and one option combination remain outside this count. |
| Tests today | 7 test cases: 3 analytical, 2 option/input-validation and 1 conversion case, plus 1 hidden storage case. The six ordinary cases run in CI; all pass in CI. |
| Exemplar archive | `6_6_stats_test_v2.tar.gz` is retired for this filter. Current tests construct their inputs and use analytical expectations. |
| Legacy comparison | Not run for the current algorithm. The DREAM.3D v6.5.171 comparison dated 2026-08-20 precedes the latest algorithm change. |
| Bug flags | None |
| V&V phase | COMPLETE |

## Summary

`ComputeSchmidsFilter` calculates each feature's maximum Schmid factor from its average orientation, crystal structure and loading direction. Twelve Class 1 fixtures and Class 4 invariants check the cubic calculations, with additional input and output checks. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps `FindSchmids` to this filter; both sources use the same feature loop and automatic/override calculations, with higher precision and added validation in SIMPLNX.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Closed-form cubic slip-system products define expected factors, angles, indices and poles; invariants check physical bounds, loading-scale invariance and cubic-symmetry invariance.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeSchmidsTest.cpp` — `Class 1 analytical oracle, auto slip system` (5 loadings), `orientation-matrix convention and equivariance` (4 quaternions), and `override slip system path` (3 loadings). All pass in CI.

*Second-engineer review:* No second engineer is named in the sign-off.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `ComputeSchmidsFilter-D1`, `ComputeSchmidsFilter-D3`, `ComputeSchmidsFilter-D6` and `ComputeSchmidsFilter-D7`. DREAM3D-NX did not use the legacy angle sentinel described in `ComputeSchmidsFilter-D2`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeSchmidsFilter-D1` | Truncated cubic normalizers inflated factors and allowed values above 0.5. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | EbsdLib uses full-precision square-root constants. |
| `ComputeSchmidsFilter-D2` | Skipped legacy angle rows retained the error-code value `-301`. | DREAM.3D v6.5.171 was affected. DREAM3D-NX was not affected. | Skipped output rows contain zero. |
| `ComputeSchmidsFilter-D3` | Library routines left outputs unchanged or read a previous value, which could contaminate the next feature. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Library routines initialize outputs; the filter also resets local outputs for each feature. |
| `ComputeSchmidsFilter-D6` | Invalid phases, malformed arrays and zero vectors could cause invalid reads or output. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Preflight checks array sizes and vectors; execution checks phase bounds. |
| `ComputeSchmidsFilter-D7` | Output actions omitted an explicit fill value, so skipped rows depended on store initialization. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected by this omission. | All five output actions explicitly request zero initialization. |

## Code path coverage

20 of 25 paths exercised. Each uncovered row states its gap.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeSchmids.cpp` (139 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeSchmidsFilter.cpp` (307 lines).

Test names below omit the common `OrientationAnalysis::ComputeSchmidsFilter: ` prefix. Preflight validates inputs and declares outputs; execution initializes the sentinel and processes each remaining feature.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | *Preflight* | Reject unequal Feature-array tuple counts (`-13503`) | "preflight input validation" / "Feature Phases and Average Quaternions must have equal tuple counts" |
| 2 | *Preflight* | Reject empty Feature arrays (`-13504`) | "preflight input validation" / "Feature arrays must contain the sentinel tuple" |
| 3 | *Preflight* | Reject an empty Crystal Structures array (`-13505`) | "preflight input validation" / "Crystal Structures must contain at least one ensemble" |
| 4 | *Preflight* | Reject a zero Loading Direction (`-13506`) | "preflight input validation" / "Loading Direction must be non-zero" |
| 5 | *Preflight* | Create the three unconditional output arrays with a zero fill | `Class 1 analytical oracle, auto slip system` |
| 6 | *Preflight* | Create `Phis`/`Lambdas` when `StoreAngleComponents` is true | "Class 1 analytical oracle, auto slip system" |
| 7 | *Preflight* | Skip `Phis`/`Lambdas` when `StoreAngleComponents` is false | "options, skip path and phase guards" / "StoreAngleComponents == false does not create Phis/Lambdas" |
| 8 | *Preflight* | Reject a zero override Slip Plane (`-13507`) | "preflight input validation" / "Override Slip Plane must be non-zero" |
| 9 | *Preflight* | Reject a zero override Slip Direction (`-13508`) | "preflight input validation" / "Override Slip Direction must be non-zero" |
| 10 | *Preflight* | Reject a slip direction not lying in the slip plane (`-13500`) | "override slip system path" / "preflight rejects a slip direction that is not in the slip plane" |
| 11 | *Execute* | Zero the Feature-0 sentinel row | "Class 1 analytical oracle, auto slip system" |
| 12 | *Execute* | Normalize the loading direction | "Class 1 analytical oracle, auto slip system" / loading `[3,6,9]` |
| 13 | *Execute* | Normalize the user slip plane and direction (`OverrideSystem` true) | "override slip system path" |
| 14 | *Execute - per-Feature* | Negative phase id guard (`-13502`) | "options, skip path and phase guards" / "a negative phase id is an error" |
| 15 | *Execute - per-Feature* | Out-of-range phase id guard (`-13501`) | "options, skip path and phase guards" / "a phase id beyond the ensemble count is an error" |
| 16 | *Execute - per-Feature* | Skip a Feature whose Laue class is at or beyond `LaueGroupEnd` | "options, skip path and phase guards" / "Features whose Laue class is beyond LaueGroupEnd get defined zeros" |
| 17 | *Execute - per-Feature* | Triclinic automatic mode has no enumerated slip systems | "options, skip path and phase guards" / "Valid Laue classes without slip systems keep zero Schmid outputs and computed Poles" |
| 18 | *Execute - per-Feature* | Cubic-High automatic slip-system path | "Class 1 analytical oracle, auto slip system"; "orientation-matrix convention and equivariance" |
| 19 | *Execute - per-Feature* | Cubic-High override path, including a zero maximum | "override slip system path" |
| 20 | *Execute - per-Feature* | Store or omit angle components | `Class 1 analytical oracle, auto slip system`; `override slip system path`; `options, skip path and phase guards` |
| 21 | *Execute - per-Feature* | `m_ShouldCancel` early return | *Not directly tested. No fixture injects cancellation.* |
| 22 | Storage | Quaternion chunk and one-feature tail | *Not directly tested. The hidden `real HDF5 quaternion-chunk tail oracle` requires an HDF5 store.* |
| 23 | Execute | Only Feature 0 exists; the feature loop is empty | *Not directly tested. No executing fixture has exactly one feature tuple.* |
| 24 | Library selection | Other Laue classes in automatic mode; all non-Cubic-High classes in override mode | *Not directly tested. Fixtures select Cubic-High in both modes and Triclinic in automatic mode only.* |
| 25 | Options | Override enabled with angle storage disabled | *Not directly tested. All executing override fixtures enable angle storage.* |

## Test inventory

The six ordinary test cases run in CI. All pass in CI. The hidden case is listed for source completeness.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeSchmidsFilter: Class 1 analytical oracle, auto slip system` | new-for-V&V | Five loading variants: `[0,0,1]`, `[1,1,1]`, `[0,1,1]`, `[1,2,3]`, `[3,6,9]`. Checks all five outputs, the sentinel, bounds and the cosine product. |
| `OrientationAnalysis::ComputeSchmidsFilter: orientation-matrix convention and equivariance` | new-for-V&V | Four quaternion variants: identity, 180 degrees about Z, 120 degrees about [111], and 180 degrees about X. Checks factors, indices and signed poles. |
| `OrientationAnalysis::ComputeSchmidsFilter: override slip system path` | new-for-V&V | Three loading variants: `[1,2,3]`, `[1,1,1]`, `[0,0,1]`. Checks all outputs, the positive-factor angle identity and the zero-result sentinel; rejects a nonperpendicular plane/direction pair. |
| `OrientationAnalysis::ComputeSchmidsFilter: options, skip path and phase guards` | new-for-V&V | Checks angle storage off, all skipped rows in a 20,000-feature fixture, Triclinic zero outputs, and errors for negative/out-of-range phases. |
| `OrientationAnalysis::ComputeSchmidsFilter: preflight input validation` | new-for-V&V | Checks unequal feature tuple counts, empty feature/ensemble arrays and three zero-vector errors (`-13503` through `-13508`). |
| `OrientationAnalysis::ComputeSchmidsFilter: real HDF5 quaternion-chunk tail oracle` | new-for-V&V | Hidden. Requires HDF5 stores and a 65,536-tuple quaternion chunk; asserts factors 0.5 and 0.48, pole components and a skipped-row zero. |
| `OrientationAnalysis::ComputeSchmidsFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check filter identity, comments, booleans and array paths/names. |

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz` — retired for this filter; no current test consumes it.
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723` (retired archive; still registered for other plugin tests).
- **Provenance:** Not applicable to the current inline fixtures in `src/Plugins/OrientationAnalysis/test/ComputeSchmidsTest.cpp`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeSchmidsFilter.md` for the root cause, affected users and recommendation. This table lists only Part 1 entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeSchmidsFilter-D1` | The corrected cubic automatic calculation removes the legacy normalizer bias. |
| `ComputeSchmidsFilter-D2` | Skipped angle rows contain zero instead of `-301`. |
| `ComputeSchmidsFilter-D3` | Initialized library outputs prevent values from a previous feature from being reused. |
| `ComputeSchmidsFilter-D4` | Default angle-array names and preflight error codes differ; mode-dependent angle units are shared behavior. |
| `ComputeSchmidsFilter-D5` | Override indices differ because the libraries order tied symmetry operators differently. |
| `ComputeSchmidsFilter-D6` | SIMPLNX rejects malformed inputs that DREAM.3D v6.5.171 accepted. |
