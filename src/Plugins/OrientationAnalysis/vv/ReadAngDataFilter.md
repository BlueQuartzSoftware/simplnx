# V&V Report: ReadAngDataFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `5b062816-79ac-47ce-93cb-e7966896bcbd` |
| SIMPLNX Human Name | Read EDAX EBSD Data (.ang) |
| DREAM.3D v6.5.171 equivalent | `ReadAngData` (SIMPL UUID `b8e128a8-c2a3-5e6c-a7ad-e4fb864e5d40`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-07; second engineer: Jared Duffey, 2026-07-23 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `ReadAngData` in DREAM.3D v6.5.171. SIMPLNX retains the import rules with changes to names, units, validation and array writes. |
| Oracle (confirmed) | Class 1 values and Class 4 ensemble invariants use inline fixtures in `test/ReadAngDataTest.cpp`. All pass in CI. |
| Code paths enumerated | 13 of 29 exercised by the ordinary CI cases. Gaps include defensive checks, cancellation, full-block writes and injected write failures. |
| Tests today | 9 test cases: 2 analytical/invariant cases, 5 error cases, 1 hidden block-boundary case and 1 SIMPL conversion case. |
| Exemplar archive | None in use. `read_ang_test.tar.gz` was retired and replaced by inline fixtures. |
| Legacy comparison | Not run on the current algorithm; the DREAM.3D v6.5.171 comparison dated 2026-07-07 predates later algorithm changes. The separate local proof run is dated 2026-09-17. |
| Bug flags | None |
| V&V phase | COMPLETE. The analytical fixtures and error cases pass in CI. |

## Summary

`ReadAngDataFilter` imports one EDAX `.ang` scan into an Image Geometry with cell and ensemble arrays. Hand-derived values and ensemble invariants check geometry, phase remapping, Euler interleaving and copied data. The V&V fixed two bounds defects, and the ordinary tests pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/OrientationAnalysisLegacyUUIDMapping.hpp` maps `ReadAngData` to this filter; both sources retain phase remapping, Euler interleaving and ensemble population, with the documented validation and metadata changes.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Derive geometry and array values from a 3×2 two-phase ANG fixture; derive unused ensemble-slot defaults from a sparse phase-2 fixture.

*Encoded:* `src/Plugins/OrientationAnalysis/test/ReadAngDataTest.cpp::OrientationAnalysis::ReadAngDataFilter: Class 1 Analytical Oracle` and `OrientationAnalysis::ReadAngDataFilter: Non-Contiguous Phase Index` — 2 inline fixtures. All pass in CI.

*Second-engineer review:* skipped — the recorded reason is that expected values follow directly from header values, column copies and symmetry mappings.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `ReadAngDataFilter-D3` and `ReadAngDataFilter-D6`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ReadAngDataFilter-D3` | Sparse phase indices could write outside the ensemble arrays. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Size arrays to `max(maxPhaseIndex, phaseCount) + 1`, initialize all slots, and reject indices outside the allocated range. |
| `ReadAngDataFilter-D6` | A geometry larger than the reader buffers caused out-of-bounds reads. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected by inconsistent column counts. | Return `-19503` before copying when the reader has fewer points than the geometry. |

## Code path coverage

13 of 29 paths exercised. The count uses the eight ordinary CI cases; each remaining row states its coverage gap.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/ReadAngData.cpp` (246 lines) and `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/ReadAngDataFilter.cpp` (258 lines).

Test names below omit the common `OrientationAnalysis::ReadAngDataFilter: ` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Propagate a header-reader error | *Not directly tested. No fixture makes `readHeaderOnly()` fail.* |
| 2 | Preflight | Reject a missing GRID key with `-19501` | `Missing GRID Preflight Error (-19501)` |
| 3 | Preflight | Reject HexGrid with `-19500` | `HexGrid Preflight Error (-19500)` |
| 4 | Preflight | Create geometry with header dimensions and spacing, zero origin and Micrometer units | `Class 1 Analytical Oracle` |
| 5 | Preflight | Create float32 arrays for the six ANG data columns | `Class 1 Analytical Oracle` |
| 6 | Preflight | Create an int32 array in the column-dispatch loop | *Not directly tested. `AngFields::getFilterFeatures()` supplies only float32 fields.* |
| 7 | Preflight | Skip a column with neither supported type | *Not directly tested. The fixed ANG field list supplies only supported float32 fields.* |
| 8 | Preflight | Create int32 Phases and three-component float32 EulerAngles | `Class 1 Analytical Oracle` |
| 9 | Preflight | Size ensemble arrays to `max(maxPhaseIndex, phaseCount) + 1` | `Class 1 Analytical Oracle`; `Non-Contiguous Phase Index` |
| 10 | Read | Propagate `readFile()` errors | `EbsdLib Error Passthrough - No Phase (-150)`; `EbsdLib Error Passthrough - Truncated Data (-600)` |
| 11 | Ensemble | Reject an empty phase vector in `loadMaterialInfo()` | *Not directly tested. The reader returns `-150` before this helper can receive an empty vector.* |
| 12 | Ensemble | Initialize slot 0 and unused slots to invalid-phase defaults | `Class 1 Analytical Oracle`; `Non-Contiguous Phase Index` |
| 13 | Ensemble | Copy symmetry indices, trimmed material names and lattice constants | `Class 1 Analytical Oracle`; `Non-Contiguous Phase Index` |
| 14 | Ensemble | Reject a phase section with index below 1 using `-19502` | `Phase 0 rejected (-19502)` |
| 15 | Ensemble | Reject an index at or above the allocated count using `-19504` | *Not directly tested. Requires a file change between preflight and execution.* |
| 16 | Copy | Reject fewer reader points than geometry cells using `-19503` | *Not directly tested. No fixture changes the file or supplies inconsistent column counts.* |
| 17 | Copy | Map cell phases below 1 to 1; preserve positive phases | `Class 1 Analytical Oracle` |
| 18 | Copy | Interleave and write a partial Euler block | `Class 1 Analytical Oracle` |
| 19 | Copy | Write a full 65,536-tuple Euler block and a tail | *Not directly tested. Ordinary CI fixtures have six points; the separate hidden `real HDF5 65536-block import oracle` defines this boundary fixture.* |
| 20 | Copy | Copy Image Quality, Confidence Index, SEM Signal, Fit and X/Y Position | `Class 1 Analytical Oracle` |
| 21 | Cancellation | Return at any of the four cancellation checks | *Not directly tested. Requires cancellation injection.* |
| 22 | Write | Propagate a Phases write failure | *Not directly tested. Requires an injected store write failure.* |
| 23 | Write | Propagate an EulerAngles write failure | *Not directly tested. Requires an injected store write failure.* |
| 24 | Write | Propagate an Image Quality write failure | *Not directly tested. Requires an injected store write failure.* |
| 25 | Write | Propagate a Confidence Index write failure | *Not directly tested. Requires an injected store write failure.* |
| 26 | Write | Propagate a SEM Signal write failure | *Not directly tested. Requires an injected store write failure.* |
| 27 | Write | Propagate a Fit write failure | *Not directly tested. Requires an injected store write failure.* |
| 28 | Write | Propagate an X Position write failure | *Not directly tested. Requires an injected store write failure.* |
| 29 | Write | Propagate a Y Position write failure | *Not directly tested. Requires an injected store write failure.* |

## Test inventory

The eight ordinary cases are in `src/Plugins/OrientationAnalysis/test/ReadAngDataTest.cpp`. All pass in CI. The same file contains the hidden boundary case listed below.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::ReadAngDataFilter: Class 1 Analytical Oracle` | new-for-V&V | Checks geometry, all eight cell arrays, all three ensemble arrays and inherited tuple dimensions against inline expected values. |
| `OrientationAnalysis::ReadAngDataFilter: Non-Contiguous Phase Index` | new-for-V&V | A phase-2-only header requires three ensemble tuples; slots 0 and 1 retain invalid-phase defaults. Covers D3. |
| `OrientationAnalysis::ReadAngDataFilter: HexGrid Preflight Error (-19500)` | new-for-V&V | Checks preflight error `-19500` for HexGrid. |
| `OrientationAnalysis::ReadAngDataFilter: Missing GRID Preflight Error (-19501)` | new-for-V&V | Checks preflight error `-19501` when GRID is absent. |
| `OrientationAnalysis::ReadAngDataFilter: EbsdLib Error Passthrough - No Phase (-150)` | new-for-V&V | Checks successful preflight and execution error `-150` for a header without phases. |
| `OrientationAnalysis::ReadAngDataFilter: Phase 0 rejected (-19502)` | new-for-V&V | Checks execution error `-19502` for a phase-0 header section. Covers D5. |
| `OrientationAnalysis::ReadAngDataFilter: EbsdLib Error Passthrough - Truncated Data (-600)` | new-for-V&V | Checks execution error `-600` when four data rows are supplied for six points. |
| `OrientationAnalysis::ReadAngDataFilter: real HDF5 65536-block import oracle` | new-for-V&V | Hidden. Defines 65,537 points and checks EulerAngles, Phases and Image Quality at the full-block boundary and tail, plus geometry and tuple dimensions. |
| `OrientationAnalysis::ReadAngDataFilter: SIMPL Backwards Compatibility` | new-for-V&V | The `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` dynamic sections check filter identity, empty comments and all four converted arguments. |

## Exemplar archive

- **Archive:** None in use; `read_ang_test.tar.gz` is retired. Current fixtures are inline.
- **SHA512:** No current registration in `src/Plugins/OrientationAnalysis/test/CMakeLists.txt`; the retired registration was `de7cd89d925da01f291f44686964ec89d469659d0005219f9869afe26b8f62af278461ac3f5deb3afe7f3e65ec074ab3a1357d77a1a5f92eb3a1ea8cc5e4b236`.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/ReadAngDataFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ReadAngDataFilter.md` for the root cause, affected users and recommendation. This table lists only Part 1 entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `ReadAngDataFilter-D1` | SIMPLNX trims material names; DREAM.3D v6.5.171 preserves trailing spaces. |
| `ReadAngDataFilter-D2` | SIMPLNX assigns Micrometer units to TEM/ACOM headers that DREAM.3D v6.5.171 assigns Nanometer units. |
| `ReadAngDataFilter-D3` | SIMPLNX imports sparse phase indices with valid ensemble bounds; DREAM.3D v6.5.171 can crash. |
| `ReadAngDataFilter-D4` | Rejected files can have different error codes and detection stages. |
| `ReadAngDataFilter-D5` | SIMPLNX rejects phase-0 header sections; DREAM.3D v6.5.171 writes them into ensemble slot 0. |
