# V&V Report: ComputeAvgCAxesFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `453cdb58-7bbb-4576-ad5e-f75a1c54d348` |
| SIMPLNX Human Name | Compute Average C-Axis Orientations |
| DREAM.3D v6.5.171 equivalent | `FindAvgCAxes` (SIMPL UUID `c5a9a96c-7570-5279-b383-cc25ebae0046`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-05-28; second engineer: Nathan Young, 2026-06-09 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `FindAvgCAxes` in DREAM.3D v6.5.171. SIMPLNX adds phase selection, double-precision orientation calculations, NaN output for empty features, and final normalization. |
| Oracle (confirmed) | Classes 1 and 4 are encoded in `test/ComputeAvgCAxesTest.cpp`. The analytical fixture checks component values, NaN values, and unit-vector magnitudes. |
| Code paths enumerated | 15 of 29 exercised by ordinary test cases. Cancellation, I/O failures, Hexagonal_Low, additional input conditions, and the full-block boundary lack ordinary fixtures. |
| Tests today | 6 test cases: 5 ordinary cases cover the oracle, input errors, and SIMPL conversion; 1 hidden case checks a block boundary. |
| Exemplar archive | `compute_avg_c_axis.tar.gz` supplies hand-built inputs. It replaces `7_2_AvgCAxis.tar.gz`; expected values are encoded in C++. |
| Legacy comparison | Not run — comparison artifacts are not available. |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`ComputeAvgCAxesFilter` computes a unit average C-axis for each feature from its hexagonal-phase cells. Class 1 component expectations and Class 4 unit-vector invariants check the output. The filter has three fixed defects and two documented differences from DREAM.3D v6.5.171.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `OrientationAnalysisLegacyUUIDMapping.hpp` maps `FindAvgCAxes` to this filter; both sources rotate crystal `[001]`, align antipodal contributions, and average by feature, while SIMPLNX adds phase filtering and final normalization.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* The 11-cell fixture gives component values for F1–F4 and NaN values for F0, F5, and F6; F7 checks unit magnitude because its sign decision lies at a rounding boundary.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeAvgCAxesTest.cpp::OrientationAnalysis::ComputeAvgCAxesFilter: Class 1 Oracle (hand-built dataset)` — 1 fixture, 8 features, and a general magnitude invariant; the five ordinary test cases use the supplied in-core CI evidence. All pass in CI.

*Second-engineer review:* Skipped — the provenance records an analytical derivation from quaternion-rotated C-axes for the hand-built fixture, dated 2026-06-05.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.1 contains the fix for `ComputeAvgCAxesFilter-D5`. DREAM3D-NX v7.4.2 contains the fix for `ComputeAvgCAxesFilter-D4`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeAvgCAxesFilter-D3` | A negative or out-of-range phase on a participating cell could read outside Crystal Structures. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Returns error `-76404` before the lookup; skips background cells before checking their phases. |
| `ComputeAvgCAxesFilter-D4` | SIMPL conversion selected the cell matrix for feature output and supplied matrix paths for the phase and crystal-structure arrays. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Uses the legacy output path for the feature matrix and derives conventional phase-array paths. |
| `ComputeAvgCAxesFilter-D5` | SIMPL conversion expected a string for the output name, but the legacy value is a path object. | DREAM3D-NX v7.0.0 through v7.4.0. DREAM.3D v6.5.171 was not affected. | Extracts the array name with `DataArrayCreationToDataObjectNameFilterParameterConverter`. |

## Code path coverage

15 of 29 paths exercised. The count uses ordinary test cases; the hidden boundary fixture appears separately in the inventory.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeAvgCAxes.cpp` (206 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeAvgCAxesFilter.cpp` (174 lines).

The algorithm scans the ensemble, accumulates cell contributions, and finalizes feature values. Test names below omit the common `OrientationAnalysis::ComputeAvgCAxesFilter:` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Ensemble | No hexagonal phase → error `-76402` | `No_Hex_Phase` |
| 2 | Ensemble | All non-placeholder phases are hexagonal → proceed without a symmetry warning | `Phase Index Bounds` — invalid phase on background only; execution succeeds |
| 3 | Ensemble | Mixed crystal structures → warning `-76403`, then proceed | `Class 1 Oracle (hand-built dataset)` — checks output; does not assert the warning code |
| 4 | Cells | FeatureId is zero → skip | `Phase Index Bounds` — invalid background phase does not cause an error |
| 5 | Cells | Non-hexagonal cell → skip contribution | `Class 1 Oracle (hand-built dataset)` — F6 is NaN |
| 6 | Cells | Hexagonal_High cell → rotate and accumulate | `Class 1 Oracle (hand-built dataset)` — F1–F3 component checks |
| 7 | Cells | Negative cosine → flip the C-axis | `Class 1 Oracle (hand-built dataset)` — F4 antipodal pair |
| 8 | Cells | Cancellation before a chunk → return | *Not directly tested. No fixture injects cancellation.* |
| 9 | Finalize | Visit feature zero | `Class 1 Oracle (hand-built dataset)` — F0 is NaN |
| 10 | Finalize | No hexagonal contributions → three NaN components | `Class 1 Oracle (hand-built dataset)` — F0, F5, F6 |
| 11 | Finalize | Positive count → average and normalize | `Class 1 Oracle (hand-built dataset)` — F1–F4 components and F7 magnitude |
| 12 | Finalize | Cancellation during feature finalization → return | *Not directly tested. No fixture injects cancellation.* |
| 13 | Preflight | Cell tuple counts differ → error `-6400` | `Preflight Error - Cell array tuple count mismatch (-6400)` |
| 14 | Cells | Participating phase is negative → error `-76404` | `Phase Index Bounds` — phase -1 |
| 15 | Cells | Participating phase exceeds the ensemble bounds → error `-76404` | `Phase Index Bounds` — phase 2 with two ensemble tuples |
| 16 | Cells | FeatureId is negative → skip | *Not directly tested. Fixtures do not assign negative FeatureIds.* |
| 17 | Cells | Hexagonal_Low cell → rotate and accumulate | *Not directly tested. Fixtures use Hexagonal_High.* |
| 18 | Cells | Participating phase zero maps to a non-hexagonal structure → skip | *Not directly tested. No successful fixture assigns phase zero to a positive FeatureId.* |
| 19 | Ensemble I/O | Crystal Structures read fails → return its error | *Not directly tested. No fixture injects an ensemble read failure.* |
| 20 | Cell I/O | FeatureIds read fails → return its error with existing warnings | *Not directly tested. No fixture injects this read failure.* |
| 21 | Cell I/O | Cell Phases read fails → return its error with existing warnings | *Not directly tested. No fixture injects this read failure.* |
| 22 | Cell I/O | Quaternions read fails → return its error with existing warnings | *Not directly tested. No fixture injects this read failure.* |
| 23 | Output I/O | Average C-Axes write fails → return its error with existing warnings | *Not directly tested. No fixture injects an output write failure.* |
| 24 | Cell I/O | Process a full 4,096-tuple chunk and a partial tail | *Not directly tested. Ordinary fixtures are smaller than one full chunk; the hidden `genuine HDF5 4096-block tail oracle` defines this check.* |
| 25 | Conversion | Convert legacy array paths and output name | `SIMPL Backwards Compatibility` — `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` |
| 26 | Conversion | FeatureIds key is absent → skip derived phase paths | *Not directly tested. Both conversion fixtures contain the key.* |
| 27 | Conversion | Container or matrix name cannot be read → skip derived phase paths | *Not directly tested. Both conversion fixtures have valid path objects.* |
| 28 | Cells | Cosine is not negative → keep the C-axis sign | `Class 1 Oracle (hand-built dataset)` — aligned F3 cells |
| 29 | Cells | One feature contains both hexagonal and non-hexagonal cells → retain only the hexagonal contributions | *Not directly tested. Each fixture feature belongs to one phase.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeAvgCAxesFilter: Class 1 Oracle (hand-built dataset)` | new-for-V&V | Replaces the old exemplar-output comparison. Dynamic sections F0–F6 check three components or NaNs per feature; F7 checks magnitude; the general invariant checks features 1, 2, 3, 4, and 7. Component and magnitude tolerance is `1e-5`. |
| `OrientationAnalysis::ComputeAvgCAxesFilter: genuine HDF5 4096-block tail oracle` | new-for-V&V | Hidden. Requires HDF5 input and output stores; checks feature-zero NaNs and `(0, 0.5, √3/2)` from contributions at indices 4,095 and 4,096. |
| `OrientationAnalysis::ComputeAvgCAxesFilter:No_Hex_Phase` | kept | Reuses the hand-built input; changes phase 1 to Cubic_High and requires exactly one error, `-76402`. |
| `OrientationAnalysis::ComputeAvgCAxesFilter: Preflight Error - Cell array tuple count mismatch (-6400)` | new-for-V&V | Uses 10 quaternion and FeatureId tuples with 9 phase tuples; requires exactly one preflight error, `-6400`. |
| `OrientationAnalysis::ComputeAvgCAxesFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check identity, comments, and six converted arguments. The fixtures use the same matrix name for input and output, so they do not isolate the feature-matrix part of D4. |
| `OrientationAnalysis::ComputeAvgCAxesFilter: Phase Index Bounds` | new-for-V&V | Four generated combinations place phase -1 or 2 on a participating or background cell. Participating cells require `-76404`; background cells permit successful execution (D3). |

The five ordinary test cases use the in-core CI result. All pass in CI.

## Exemplar archive

- **Archive:** `compute_avg_c_axis.tar.gz`
- **SHA512:** `4ee957b4a5e78e1d75e3585016a33de40985c66a8e8d1036b252e5974eb2b3360f34dacdcb51cd1d1ae25bfef2bb638979912cbc4555ff521eb2f42a167155b0`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeAvgCAxesFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeAvgCAxesFilter.md` for the source evidence, affected users, and recommendations. Part 1 contains the following entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeAvgCAxesFilter-D1` | SIMPLNX skips non-hexagonal cells and writes NaN when no hexagonal cells contribute. DREAM.3D v6.5.171 has no phase inputs, leaves feature zero at zero, and uses `(0,0,1)` for an empty positive feature. |
| `ComputeAvgCAxesFilter-D2` | SIMPLNX normalizes the final average and uses double-precision orientation calculations with float32 sums. DREAM.3D v6.5.171 uses float calculations and leaves the final average unnormalized. |
