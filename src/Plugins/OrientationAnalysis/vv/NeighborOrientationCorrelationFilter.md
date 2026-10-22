# V&V Report: NeighborOrientationCorrelationFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `4625c192-7e46-4333-a294-67a2eb64cb37` |
| SIMPLNX Human Name | Neighbor Orientation Correlation |
| DREAM.3D v6.5.171 equivalent | `NeighborOrientationCorrelation` (SIMPL UUID `6427cd5e-0ad2-5a24-8847-29f8e0720f4f`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-06; second engineer: Nathan Young, 2026-07-08 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `NeighborOrientationCorrelation` in DREAM.3D v6.5.171. SIMPLNX corrects the pass count and neighbor selection, uses double-precision orientations, and transfers data by slice. |
| Oracle (confirmed) | Classes 1 and 4 define analytical tuples and invariants in `test/NeighborOrientationCorrelationTest.cpp`. A Class 2 NumPy reference supplies the 12-fixture comparison evidence in provenance. |
| Code paths enumerated | 26 of 40 exercised. Gaps include transfer-order cases, cancellation, and I/O failures. |
| Tests today | 19 ordinary test cases cover analytical outputs, invariants, invalid inputs and SIMPL conversion. All pass in CI; 1 additional hidden case generates input data. |
| Exemplar archive | `Small_IN100_dream3d_v3.tar.gz` supplies production-test input only. The two filter-output exemplars are retired; expected outputs come from analytical fixtures and invariants. |
| Legacy comparison | Not run on the current algorithm; the run against DREAM.3D v6.5.171 on 2026-07-07 predates the slice-transfer changes. |
| Bug flags | None |
| V&V phase | Verification records and regression fixtures are complete. A legacy comparison with the current algorithm remains open. |

## Summary

`NeighborOrientationCorrelationFilter` replaces low-confidence cells with tuples from the face neighbor that has the highest similarity count over `6 - Level` passes. Analytical fixtures and invariants verify selection, phase handling and tuple transfer. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; source comparison retains face-neighbor pair counting but changes selection, pass count, precision and transfer order.

## Oracle

*Class:* 1 (Analytical) and 4 (Invariant), with 2 (NumPy reference implementation) for the archived comparison fixtures.

*Applied:* Hand-derived rotations and similarity counts specify source tuples; invariants require unchanged high-confidence cells, unchanged ignored arrays, and no changes at Level 6.

*Encoded:* `src/Plugins/OrientationAnalysis/test/NeighborOrientationCorrelationTest.cpp::OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F01 - uniform neighbors 3D` and F02–F13 provide 13 analytical cases; `Class 4 - Level >= 6 is a no-op (I4)` and `Small IN100 Pipeline` check invariants. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-07-08.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for D2, D3, D5 and D6. The fix for D7 will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX releases were not affected by D1.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `NeighborOrientationCorrelationFilter-D1` | A stale angle lets mixed-phase or phase-zero pairs count as similar. | DREAM.3D v6.5.171 was affected. DREAM3D-NX v7.0.0 through v7.4.2 were not affected. | Count a pair only inside the same-positive-phase condition. |
| `NeighborOrientationCorrelationFilter-D2` | A second level decrement stops cleanup passes early. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Run exactly `6 - Level` passes for valid levels below 6. |
| `NeighborOrientationCorrelationFilter-D3` | Resetting the best count for every neighbor selects the last positive count instead of the maximum. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Select the largest count; resolve ties to the last neighbor. |
| `NeighborOrientationCorrelationFilter-D5` | The exemplar test skips every missing array and can pass without comparing output. | DREAM3D-NX v7.0.0 through v7.4.1 test sources. DREAM.3D v6.5.171 was not affected. | Check production-scale snapshots and analytical tuples instead of the retired output archive. |
| `NeighborOrientationCorrelationFilter-D6` | Tuple transfer omits NeighborList and String cell arrays. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Include both array types in transfer and check their values in F13. |
| `NeighborOrientationCorrelationFilter-D7` | Invalid participating phase or Laue indices cause out-of-bounds access. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | Return -580096 for an invalid positive phase and -580097 for an invalid Laue index. |

## Code path coverage

26 of 40 paths exercised. Each uncovered path states its missing fixture below.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/NeighborOrientationCorrelation.cpp` (359 lines), `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/NeighborOrientationCorrelationFilter.cpp` (243 lines), and `src/simplnx/Utilities/SliceBufferedTransfer.hpp` (364 lines).

Test names below omit the common `OrientationAnalysis::NeighborOrientationCorrelationFilter: ` prefix. Each pass scans and transfers one Z slice at a time.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Cell tuple counts differ → error -580093 | `Preflight Error - Cell array tuple count mismatch (-580093)` |
| 2 | Preflight | Level < 0 → error -580094 | `Preflight - Level validation (-580094 error, -580095 warning)` |
| 3 | Preflight | Level >= 6 → warning -580095 | `Preflight - Level validation (-580094 error, -580095 warning)` |
| 4 | Preflight | Valid inputs → proceed and report modified arrays | `Oracle F01 - uniform neighbors 3D` |
| 5 | Scan | Confidence is at least the threshold → skip | `Oracle F01 - uniform neighbors 3D`; `Small IN100 Pipeline` |
| 6 | Scan | Face neighbor is outside the image → skip | `Oracle F11 - volume corner`; `Oracle F07 - 2D image` |
| 7 | Scan | Unequal X, Y and Z strides → locate the correct neighbors | `Oracle F12 - anisotropic dims` |
| 8 | Scan | Same positive phase and angle below tolerance → count both neighbors | `Oracle F01 - uniform neighbors 3D` |
| 9 | Scan | Angle is at or above tolerance → do not count | `Oracle F02 - dissimilar neighbors untouched` |
| 10 | Scan | Different phases → do not compare orientations | `Oracle F05 - mixed-phase pair never similar (D1 regression)` |
| 11 | Scan | Phase zero → do not count | `Oracle F06 - phase-0 neighbors never counted`; `Phase and Laue Index Bounds` |
| 12 | Scan | Hexagonal symmetry folds a 58° pair to 2° → replace | `Oracle F08 - Laue-class folding (hex vs cubic)` — Hexagonal-High section |
| 13 | Scan | Cubic symmetry folds the same pair to 32° → keep input | `Oracle F08 - Laue-class folding (hex vs cubic)` — Cubic-High section |
| 14 | Select | All similarity counts are zero → no source | `Oracle F02 - dissimilar neighbors untouched` |
| 15 | Select | Counts tie → select the last valid maximum | `Oracle F01 - uniform neighbors 3D`; `Oracle F11 - volume corner` |
| 16 | Select | Counts differ → select the maximum | `Oracle F03 - argmax selection (D3 regression)`; `Oracle F12 - anisotropic dims` |
| 17 | Transfer | Numeric arrays copy through SliceBufferedTransferOneZ | `Oracle F01 - uniform neighbors 3D`; `Oracle F12 - anisotropic dims` |
| 18 | Transfer | NeighborList and String arrays copy their source tuples | `Oracle F13 - NeighborList and String cell arrays transferred` |
| 19 | Transfer | Ignored numeric array → exclude from transfer | `Oracle F09 - ignored arrays untouched (I2)` |
| 20 | Passes | Later passes use the preceding pass output | `Oracle F04 - pass schedule Level 2 (D2 regression)`; `Oracle F10 - pass schedule Level 4 (D2 regression)` |
| 21 | Transfer | Adjacent replaced cells depend on snapshot versus ascending-copy order | *Not directly tested. No fixture asserts distinct source payloads for two replaced cells in the same slice.* |
| 22 | Passes | Level >= 6 → no cleanup passes | `Class 4 - Level >= 6 is a no-op (I4)` — Level 6 |
| 23 | Setup | Single-slice image → omit Z-neighbor reads | `Oracle F07 - 2D image` |
| 24 | Validation | Participating positive phase exceeds ensemble bounds → error -580096 | `Phase and Laue Index Bounds` |
| 25 | Validation | Participating Laue index exceeds operator bounds → error -580097 | `Phase and Laue Index Bounds` |
| 26 | Scan | Negative phase → do not count | *Not directly tested. The nonpositive-phase fixture uses zero only.* |
| 27 | Setup | Read initial slices; rotate the window and stop reads at the final slice | `Oracle F01 - uniform neighbors 3D`; `Oracle F12 - anisotropic dims` |
| 28 | Transfer | Clear source marks after each slice instead of retaining them across passes | *Not directly tested. No fixture isolates a repaired cell whose old source changes in a later pass.* |
| 29 | Cancellation | Cancel at a slice boundary → return | *Not directly tested. Requires cancellation injection.* |
| 30 | I/O | Crystal Structures read fails → return error | *Not directly tested. Requires an injected ensemble read failure.* |
| 31 | I/O | Initial quaternion slice read fails → return error | *Not directly tested. Requires an injected quaternion read failure.* |
| 32 | I/O | Initial phase slice read fails → return error | *Not directly tested. Requires an injected phase read failure.* |
| 33 | I/O | Second initial quaternion slice read fails → return error | *Not directly tested. Requires an injected quaternion read failure.* |
| 34 | I/O | Second initial phase slice read fails → return error | *Not directly tested. Requires an injected phase read failure.* |
| 35 | I/O | Quaternion read after window rotation fails → return error | *Not directly tested. Requires an injected quaternion read failure.* |
| 36 | I/O | Phase read after window rotation fails → return error | *Not directly tested. Requires an injected phase read failure.* |
| 37 | I/O | Confidence slice read fails → return error | *Not directly tested. Requires an injected confidence read failure.* |
| 38 | I/O | Numeric transfer read or write fails → return error | *Not directly tested. Requires an injected transfer failure.* |
| 39 | Transfer | No source marks → preserve numeric tuples without a slice write | `Oracle F02 - dissimilar neighbors untouched` |
| 40 | Transfer | CopyArrayTuple receives a numeric array → copyTuple | *Not directly tested. The caller routes numeric arrays through SliceBufferedTransferOneZ.* |

## Test inventory

All names are in `src/Plugins/OrientationAnalysis/test/NeighborOrientationCorrelationTest.cpp`. The 19 ordinary cases are the CI validation set. All pass in CI. The hidden data generator is listed separately from correctness checks.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Small IN100 Pipeline` | kept | Uses the Small IN100 input archive. Replaces the skipped exemplar comparison with snapshots of all numeric cell arrays; checks that only low-confidence cells change and that at least one changes (D5). |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Preflight Error - Cell array tuple count mismatch (-580093)` | kept | Checks error -580093 for nine phase tuples and ten confidence/quaternion tuples. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Preflight - Level validation (-580094 error, -580095 warning)` | new-for-V&V | Checks a negative Level error, a Level 6 warning, and no warning at Level 2. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Phase and Laue Index Bounds` | kept | Checks execution errors -580096 and -580097 for participating invalid indices, and success with phase-zero neighbors (D7). |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: SIMPL Backwards Compatibility` | kept | Dynamic sections `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check filter identity and converted scalar and path arguments. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F01 - uniform neighbors 3D` | new-for-V&V | Checks all six numeric arrays over 125 cells. Only cell 62 copies cell 87, the final tied neighbor. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F02 - dissimilar neighbors untouched` | new-for-V&V | Checks that all six numeric arrays stay unchanged when no neighbor pair is similar. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Generate Test Data` | kept | Hidden (`[.GenerateTestData]`). Writes a 200×200×200 synthetic input with 25-cell blocks; does not execute this filter or check its output. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F03 - argmax selection (D3 regression)` | new-for-V&V | Checks that a count-3 group wins over a later count-1 pair; compares all six numeric arrays. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F04 - pass schedule Level 2 (D2 regression)` | new-for-V&V | Checks that four passes fill the bad region and copy identity quaternions throughout the volume. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F10 - pass schedule Level 4 (D2 regression)` | new-for-V&V | Checks that two passes leave only the cube center below the confidence threshold. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F05 - mixed-phase pair never similar (D1 regression)` | new-for-V&V | Checks that cell 12 copies cell 11, not the different-phase neighbor; compares all six numeric arrays. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F06 - phase-0 neighbors never counted` | new-for-V&V | Checks that cell 62 copies cell 57 and that phase-zero neighbors do not contribute. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F07 - 2D image` | new-for-V&V | Checks that cell 12 copies the final valid +Y neighbor, cell 17, in a 5×5×1 image. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F08 - Laue-class folding (hex vs cubic)` | new-for-V&V | Checks both symmetry modes: a 58° pair folds to 2° under hexagonal symmetry and replaces the center, but to 32° under cubic symmetry and leaves it unchanged. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F09 - ignored arrays untouched (I2)` | new-for-V&V | Checks that ignored Payload2 stays unchanged while all other numeric arrays copy the selected source. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F13 - NeighborList and String cell arrays transferred` | new-for-V&V | Checks all numeric, NeighborList and String tuples; only the center copies cell 87 (D6). |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F11 - volume corner` | new-for-V&V | Checks that corner cell 0 copies cell 16 with only three valid face neighbors. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Oracle F12 - anisotropic dims` | new-for-V&V | Checks that cell 29 copies cell 28 on a 4×5×3 image, with unequal similarity counts and axis strides. |
| `OrientationAnalysis::NeighborOrientationCorrelationFilter: Class 4 - Level >= 6 is a no-op (I4)` | new-for-V&V | Executes Level 6 and compares every numeric tuple with its input snapshot. |

## Exemplar archive

- **Archive:** `Small_IN100_dream3d_v3.tar.gz` (input only; no filter-output exemplar).
- **SHA512:** `bfa9547e787b0f8e8122702da0eb4e519f48a48bf4f94aad020f72479d071d32dfc96a1425705874c68507a61ed391d28606d9c4f4acd559043ef0ace64fd33f`
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/NeighborOrientationCorrelationFilter_inputs.md`; output-archive retirement and comparison evidence: `src/Plugins/OrientationAnalysis/vv/provenance/neighbor_orientation_correlation_v2.md`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/NeighborOrientationCorrelationFilter.md` for the root cause, affected users and recommendation. This table lists only Part 1 entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `NeighborOrientationCorrelationFilter-D1` | Mixed-phase and phase-zero pairs cannot inherit an earlier pair's angle. |
| `NeighborOrientationCorrelationFilter-D2` | Cleanup uses `6 - Level` passes instead of half that number, rounded up. |
| `NeighborOrientationCorrelationFilter-D3` | The highest similarity count wins; equal maxima retain the legacy last-neighbor rule. |
| `NeighborOrientationCorrelationFilter-D4` | Misorientation uses double precision instead of single precision. |
| `NeighborOrientationCorrelationFilter-D8` | Numeric tuples use a snapshot per slice, and source marks reset after each slice; DREAM.3D uses ascending in-place copies and retains marks across passes. |
