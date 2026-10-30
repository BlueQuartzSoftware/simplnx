# V&V Report: ComputeFeatureFaceMisorientationFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `f3473af9-db77-43db-bd25-60df7230ea73` |
| SIMPLNX Human Name | Compute Feature Face Misorientation (Face) |
| DREAM.3D v6.5.171 equivalent | `GenerateFaceMisorientationColoring` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-05-19; second engineer: Michael Jackson, 2026-05-28 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `GenerateFaceMisorientationColoring` in DREAM.3D v6.5.171. SIMPLNX returns one angle, supports all 11 Laue classes and writes NaN for faces without a computed value. |
| Oracle (confirmed) | Class 1 analytical expectations in `test/ComputeFeatureFaceMisorientationTest.cpp` check 33 angles and four NaN values. |
| Code paths enumerated | 9 of 19 exercised by the ordinary tests. Cancellation, defensive guards, I/O failures and the full validation-block boundary have no direct ordinary-test coverage. |
| Tests today | Three ordinary cases cover analytical values, phase bounds and SIMPL conversion. One hidden case targets a 65,536-face block boundary. |
| Exemplar archive | None. The tests construct their inputs in C++. |
| Legacy comparison | Not run on the current algorithm. The archived comparison predates the phase-bounds validation and lookup changes. |
| Bug flags | None |
| V&V phase | COMPLETE. |

## Summary

`ComputeFeatureFaceMisorientationFilter` computes the symmetry-reduced misorientation angle between the features on each mesh face. Class 1 expectations check all 11 Laue classes and distinguish missing values from zero angles. The filter includes the cubic precision and phase-bounds fixes below.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* SIMPLNX replaces the three-component axis-times-angle output with one angle; `OrientationAnalysisLegacyUUIDMapping.hpp` maps the legacy UUID to this filter.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Pure c-axis rotations of 45°, 90° and 180° give three symmetry-reduced angles for each of 11 Laue classes; two background and two mixed-phase faces must give NaN.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ComputeFeatureFaceMisorientationTest.cpp::OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Curated Data` — 37 face expectations, with absolute angle error below 0.000012°. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-05-28, as recorded in the report sign-off.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureFaceMisorientations-D4`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureFaceMisorientations-D4` | Cubic angle extraction loses precision near zero; the SIMPLNX 90° symmetry fixture gave a false residual of about 0.0212°. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected by precision loss in its float calculation. | The filter computes cubic angles through EbsdLib with explicit reduced-quaternion components and `2*atan2` angle extraction. |
| `ComputeFeatureFaceMisorientationFilter-D5` | A positive phase outside Crystal Structures can cause an out-of-bounds read. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | The filter validates participating phases before the lookup, returns `-98412` for an invalid shared phase and skips mismatched phases. |

## Code path coverage

9 of 19 paths exercised. The count uses the three ordinary tests; the hidden boundary fixture is listed separately in the inventory.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ComputeFeatureFaceMisorientation.cpp` (205 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ComputeFeatureFaceMisorientationFilter.cpp` (179 lines).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Validation | Read feature phases and a partial block of face labels successfully | `Curated Data`; `Phase Index Bounds` |
| 2 | Validation | Participating shared phase exceeds the ensemble bounds → error `-98412` | `Phase Index Bounds` — `Participating Phase returns an error` |
| 3 | Validation | Mismatched phases bypass the participating-phase bounds check | `Phase Index Bounds` — `Mismatched Phase is ignored` |
| 4 | Cancellation | Cancel before processing a face → return | *Not directly tested. No fixture injects cancellation.* |
| 5 | Per-face | Front feature is zero → NaN | `Curated Data` — face 30 |
| 6 | Per-face | Back feature is zero → NaN | `Curated Data` — face 31 |
| 7 | Per-face | Positive features have different phases → NaN | `Curated Data` — faces 32 and 33 |
| 8 | Per-face | Shared positive phase has an unsupported Laue index → NaN | *Not directly tested. No fixture assigns an unsupported Laue index to a participating phase.* |
| 9 | Per-face | Supported Laue index → calculate and write an angle in degrees | `Curated Data` — three values for each of 11 classes |
| 10 | Per-face math | Cubic symmetry reduction gives zero for a 90° c-axis rotation | `Curated Data` — face 4 |
| 11 | Preflight | Valid input → create one float32 output component per face | `Curated Data` — output creation and angle checks |
| 12 | Preflight | Quaternion and feature-phase tuple counts differ → error `-98410` | *Not directly tested. No fixture changes these tuple counts.* |
| 13 | Preflight | Face Labels lookup fails → error `-98411` | *Not directly tested. Parameter validation rejects missing or incorrectly typed arrays first.* |
| 14 | Validation | Feature-phase bulk read fails → return the store error | *Not directly tested. No fixture injects a store read failure.* |
| 15 | Validation | Face-label bulk read fails → return the store error | *Not directly tested. No fixture injects a store read failure.* |
| 16 | Per-face | A feature label is negative → use phase zero and write NaN | *Not directly tested. Background fixtures use label zero.* |
| 17 | Per-face | A positive feature has a nonpositive phase → NaN | *Not directly tested. Referenced positive features have positive phases.* |
| 18 | Validation | No faces → skip the validation and face loops | *Not directly tested. All fixtures contain faces.* |
| 19 | Validation | Read a full 65,536-face block and a partial tail | *Not directly tested. The ordinary cases do not span a full block; the hidden `genuine HDF5 65536-block tail oracle` defines this fixture.* |

## Test inventory

The three ordinary test cases have the supplied Linux, macOS and Windows in-core CI evidence. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Curated Data` | new-for-V&V | Checks 33 analytical angles and four NaN values. Replaces the former archive regression; the cubic 90° expectation is zero after D4. |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: Phase Index Bounds` | kept | Checks `-98412` for an invalid shared phase and NaN for mismatched phases. The two sections pin D5. |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: genuine HDF5 65536-block tail oracle` | new-for-V&V | Hidden. Requires HDF5-backed inputs and output; checks 15° at tuple 65,535 and 30° at tuple 65,536 with a 0.000012° margin. |
| `OrientationAnalysis::ComputeFeatureFaceMisorientationFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` variants check filter identity, empty comments, four input paths and the output name. |

## Exemplar archive

- **Archive:** None; inputs are constructed in `test/ComputeFeatureFaceMisorientationTest.cpp`.
- **SHA512:** Not applicable; these tests consume no archive from `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ComputeFeatureFaceMisorientationFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureFaceMisorientationFilter.md` for the root cause, affected users and recommendation. D1–D4 retain their existing IDs.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureFaceMisorientations-D1` | SIMPLNX computes angles for all 11 Laue classes; DREAM.3D v6.5.171 computes only Hexagonal_High and Cubic_High. |
| `ComputeFeatureFaceMisorientations-D2` | SIMPLNX returns one angle in degrees; DREAM.3D v6.5.171 returns three axis-times-angle components. |
| `ComputeFeatureFaceMisorientations-D3` | SIMPLNX writes NaN for faces without a computed value; DREAM.3D v6.5.171 leaves zero vectors. |
| `ComputeFeatureFaceMisorientations-D4` | SIMPLNX uses stable cubic angle extraction in EbsdLib; DREAM.3D v6.5.171 uses float intermediates and `acos`. |
