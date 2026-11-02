# V&V Report: FillBadDataFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `a59eb864-9e6b-40bb-9292-e5281b0b4f3e` |
| SIMPLNX Human Name | Fill Bad Data |
| DREAM.3D v6.5.171 equivalent | `FillBadData` (Processing plugin, SIMPL UUID `30ae0a1e-3d94-5dab-b279-c5727ab5d7ff`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-07-03; second engineer: Michael Jackson, 2026-07-10 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `FillBadData` in DREAM.3D v6.5.171. SIMPLNX selects BFS or slice-based connected-component labeling (CCL). |
| Oracle (confirmed) | Class 1 expected arrays cover nine synthetic archive fixtures in `test/FillBadDataTest.cpp`. All pass in CI. |
| Code paths enumerated | 24 of 40 exercised. Cancellation, failure injection, large-index limits and the no-progress guard lack direct coverage. |
| Tests today | 20 test cases: 18 ordinary cases, one hidden HDF5 fixture and one hidden data generator. Ordinary cases cover analytical outputs, exemplar regressions, validation, ignored arrays and SIMPL conversion. |
| Exemplar archive | `6_5_fill_bad_data.tar.gz` supplies nine analytical input/output pairs and the Small IN100 regression pair. |
| Legacy comparison | Not run on the current algorithm. The comparison record is dated 2026-07-03; algorithm changes followed that run. |
| Bug flags | None |
| V&V phase | Analytical fixtures and regression tests are encoded. All pass in CI. |

## Summary

`FillBadDataFilter` fills connected zero-FeatureId regions smaller than the selected threshold by copying positive face-neighbor tuples. It preserves larger regions and can assign them a new phase. Class 1 expected arrays check the output; all ordinary tests pass in CI.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; BFS and CCL replace the single legacy flood-fill implementation while retaining the size rule and neighbor voting.

## Oracle

*Class:* 1 (Analytical).

*Applied:* Hand-derived FeatureIds and Phases define component retention, filling and the first-to-maximum neighbor vote rule; the archive script only serializes these arrays.

*Encoded:* `src/Plugins/SimplnxCore/test/FillBadDataTest.cpp` — `Test01_SingleSmallDefect` through `Test07_DefectsAtBoundaries`, `Test11_NeighborTieBreaking` and `Test13_StoreAsNewPhase`: nine fixtures. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-07-16, is named in the oracle review record.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `FillBadDataFilter-D1` through `FillBadDataFilter-D5`. The fixes for `FillBadDataFilter-D6` and `FillBadDataFilter-D7` will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `FillBadDataFilter-D1` | Preflight discarded the error for a minimum defect size below 1. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Return error -16500 before execution. |
| `FillBadDataFilter-D2` | Unfillable negative cells could keep the fill loop running forever. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Stop with a warning when no cell has a fill source. |
| `FillBadDataFilter-D3` | Negative input FeatureIds could corrupt the vote-array allocation size through unsigned conversion. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Use only positive FeatureIds to determine the maximum ID. |
| `FillBadDataFilter-D4` | A component size above INT32_MAX could become negative and be classified as small. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Compare component sizes without narrowing to Int32. |
| `FillBadDataFilter-D5` | A neighbor index above INT32_MAX could overflow and select an invalid tuple. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Keep neighbor indices in 64-bit storage. |
| `FillBadDataFilter-D6` | FeatureIds from another geometry could be processed with the selected image dimensions. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Require FeatureIds in the selected geometry's Cell Attribute Matrix (-16501). |
| `FillBadDataFilter-D7` | An ignored array selected through an alias could still be modified. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Normalize array paths to the geometry's Cell Attribute Matrix before filtering the ignored arrays. |

## Code path coverage

24 of 40 paths exercised. Each uncovered row states the missing fixture.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadData.cpp` (49 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadDataBFS.cpp` (378 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadDataCCL.cpp` (917 lines), and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/FillBadDataFilter.cpp` (194 lines).

Test names below omit the common plugin and filter prefixes. BFS visits connected cells in memory; CCL labels slices and resolves component sizes before filling.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Dispatch | Select BFS for resident arrays | `Test01_SingleSmallDefect` through `Test13_StoreAsNewPhase` — algorithm scenarios |
| 2 | Dispatch | Select CCL for resident arrays | `Test01_SingleSmallDefect` through `Test13_StoreAsNewPhase` — algorithm scenarios |
| 3 | Dispatch | Select CCL for external arrays | *Not directly tested. The hidden `real HDF5 cross-slice defect oracle` requires external-store execution.* |
| 4 | Components | Skip nonzero FeatureIds | `Test01_SingleSmallDefect` |
| 5 | Components | Start a component with no labeled backward neighbor | `Test06_SingleVoxelDefects` |
| 6 | Components | Inherit a backward neighbor label or visit a connected BFS neighbor | `Test02_SingleLargeDefect` |
| 7 | Components | Merge distinct CCL labels | *Not directly tested. No fixture isolates a merge of distinct labels.* |
| 8 | Classification | Region is smaller than the threshold; mark it for filling | `Test01_SingleSmallDefect`; `Test05_MixedSmallAndLarge` |
| 9 | Classification | Region is at or above the threshold; keep FeatureIds zero | `Test02_SingleLargeDefect`; `Test03_ThresholdBoundary` |
| 10 | Classification | Store Defects as New Phase is enabled; assign maximum phase plus one | `Test13_StoreAsNewPhase`; `200x200x200 Correctness` / `Variant: NewPhase` |
| 11 | Fill | Skip nonnegative FeatureIds and out-of-image neighbors | `Test07_DefectsAtBoundaries` |
| 12 | Fill | Copy the selected positive neighbor into a small defect | `Test01_SingleSmallDefect`; `FillBadData_SmallIN100` |
| 13 | Fill | Defer interior cells until a later iteration | `Test04_MultipleSmallDefects` |
| 14 | Fill | Break equal vote totals by the first feature to reach that total | `Test11_NeighborTieBreaking` |
| 15 | Setup | Store Defects as New Phase is disabled; omit the maximum-phase scan | `Test01_SingleSmallDefect`; `200x200x200 Correctness` / `Variant: NoNewPhase` |
| 16 | Cancellation | Stop component labeling, classification, fill or tuple transfer | *Not directly tested. No fixture injects cancellation.* |
| 17 | Preflight | Reject a minimum defect size below 1 with -16500 | `Invalid Preflight Min Defect Size` |
| 18 | Fill | Stop with a warning when remaining negative cells have no positive neighbor | *Not directly tested. `AllBadData_TerminatesWithoutHang` retains its nine-cell region at threshold 1, so it does not reach this guard.* |
| 19 | Setup | Exclude negative input FeatureIds from maximum-ID allocation sizing | *Not directly tested. No fixture supplies negative input FeatureIds.* |
| 20 | Classification | Compare a region size above INT32_MAX without narrowing | *Not directly tested. The fixtures contain fewer than INT32_MAX cells.* |
| 21 | Fill | Store a neighbor index above INT32_MAX without narrowing | *Not directly tested. The fixtures contain fewer than INT32_MAX cells.* |
| 22 | Preflight | Reject FeatureIds outside the selected Cell Attribute Matrix with -16501 | `Strict Feature Ids` |
| 23 | Preflight | Reject modified Phases outside the selected Cell Attribute Matrix with -16502 | `Strict Feature Ids` |
| 24 | Preflight | Reject an ignored array outside the selected Cell Attribute Matrix with -16503 | `Strict Feature Ids` |
| 25 | Paths | Normalize aliases before modification reporting and ignored-array selection | `Ignored Array Cell AM Alias` |
| 26 | Preflight | Warn that feature data must be recomputed | `Preflight Feature Data Modification Warning` — -14600 and output-value name |
| 27 | Fill | Leave ignored arrays unchanged | `200x200x200 Ignored Arrays` |
| 28 | I/O | Propagate array, candidate-pair or equivalence read/write failures | *Not directly tested. No fixture injects storage failures.* |
| 29 | Storage | Fall back to an in-memory temporary store after provider failure | *Not directly tested. No fixture forces provider failure.* |
| 30 | Storage | Reject a null temporary store with -87021 or -87023 | *Not directly tested. No fixture supplies a null provider result.* |
| 31 | Capacity | Reject equivalence or candidate capacity overflow with -87020, -87022 or -87029 | *Not directly tested. These limits require synthetic capacity injection.* |
| 32 | Capacity | Reject an empty image with -87027 | *Not directly tested. All fixtures contain cells.* |
| 33 | Capacity | Reject label-range exhaustion with -87028 or -87030 | *Not directly tested. No fixture reaches the Int32 label limit.* |
| 34 | Transfer | Reject zero or overflowing slice buffers with -87024 | *Not directly tested. All fixtures have valid slice and component sizes.* |
| 35 | Transfer | Reject a short candidate read with -87025 | *Not directly tested. No fixture supplies a short read.* |
| 36 | Transfer | Reject a candidate from a nonadjacent slice with -87026 | *Not directly tested. Normal candidate generation uses face neighbors.* |
| 37 | Transfer | Process full candidate batches and a final partial batch | `200x200x200 Correctness` |
| 38 | Conversion | Convert SIMPL parameters, with the ignored-array key present or absent | `SIMPL Backwards Compatibility` / `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` |
| 39 | Transfer | Copy every component of a cell tuple | `FillBadData_SmallIN100` — compares the three-component EulerAngles array |
| 40 | Preflight | Accept an unused Phases path outside the Cell Attribute Matrix | `Strict Feature Ids` / `Unused Cell Phases in another Attribute Matrix` |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::FillBadData_SmallIN100` | kept | Compares Small IN100 cell arrays with the stored legacy exemplar at threshold 1000. This is a regression comparison, not an independent correctness oracle. |
| `SimplnxCore::FillBadData::Test01_SingleSmallDefect` | kept | Class 1. Fills one small component at threshold 20; compares cell arrays with the analytical file. |
| `SimplnxCore::FillBadData::Test02_SingleLargeDefect` | kept | Class 1. Retains a large component at threshold 20. |
| `SimplnxCore::FillBadData::Test03_ThresholdBoundary` | kept | Class 1. Retains a component whose size equals threshold 25. |
| `SimplnxCore::FillBadData::Test04_MultipleSmallDefects` | kept | Class 1. Fills multiple small components at threshold 50. |
| `SimplnxCore::FillBadData::Test05_MixedSmallAndLarge` | kept | Class 1. Fills small components and retains a large component at threshold 50. |
| `SimplnxCore::FillBadData::Test06_SingleVoxelDefects` | kept | Class 1. Fills isolated single-cell defects at threshold 10. |
| `SimplnxCore::FillBadData::Test07_DefectsAtBoundaries` | kept | Class 1. Checks defects at image boundaries at threshold 20. |
| `SimplnxCore::FillBadData::Test11_NeighborTieBreaking` | kept | Class 1. Expects FeatureId 1 at the center of a three-vote tie. |
| `SimplnxCore::FillBadData::Test13_StoreAsNewPhase` | kept | Class 1. Retains 50 defect cells and changes their phase to 2 at threshold 20. |
| `SimplnxCore::FillBadDataFilter: Preflight Feature Data Modification Warning` | kept | Checks warning -14600, the Feature Data Modification Warning output and the absence of a Stale Arrays output. |
| `SimplnxCore::FillBadDataFilter:: Invalid Preflight Min Defect Size` | kept | Checks error -16500 for minimum size zero (FillBadDataFilter-D1). |
| `SimplnxCore::FillBadData::AllBadData_TerminatesWithoutHang` | kept | Checks successful return for nine bad cells at threshold 1. The region is retained; this does not exercise the no-progress guard. |
| `SimplnxCore::FillBadData: real HDF5 cross-slice defect oracle` | new-for-V&V | Hidden. Defines a 5×5×5 fixture with a retained three-cell component across slices and one filled cell; asserts FeatureIds, Phases and a two-component payload with the new-phase option off and on. |
| `SimplnxCore::FillBadDataFilter: SIMPL Backwards Compatibility` | kept | Checks converted UUID and arguments in dynamic sections SIMPL 6.5 (UUID) and SIMPL 6.4 (Filter_Name). |
| `SimplnxCore::FillBadData: 200x200x200 Correctness` | kept | Compares FeatureIds and Phases with `fill_bad_data_exemplars.tar.gz` in dynamic sections Variant: NoNewPhase and Variant: NewPhase. |
| `SimplnxCore::FillBadData: 200x200x200 Ignored Arrays` | kept | Checks that bad FeatureIds change and that every ignored sentinel value remains unchanged. |
| `SimplnxCore::FillBadDataFilter: Strict Feature Ids` | new-for-V&V | Six sections check valid cell selection, errors -16501/-16502/-16503, unused external Phases and valid modified Phases. |
| `SimplnxCore::FillBadDataFilter: Ignored Array Cell AM Alias` | new-for-V&V | Checks that aliased FeatureIds change, ignored values remain unchanged and ignored paths are absent from modification actions. |
| `SimplnxCore::FillBadData: Generate Test Data` | kept | Hidden utility. Writes a synthetic 200×200×200 input file; does not execute FillBadData or assert filter output. |

## Exemplar archive

- **Archive:** `6_5_fill_bad_data.tar.gz`
- **SHA512:** `73b6835252135619325d9b02b48286ca95f0235820610bddcc43ec0a3f075337481bace00bf20e9d59744f1d4024cc18be624ec522922a6461b55ae8dbb0e628`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/FillBadDataFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/FillBadDataFilter.md` for the root cause, affected users and recommendation. These Part 1 entries describe source-confirmed differences on invalid or extreme inputs.

| Deviation | Observed difference |
|-----------|---------------------|
| `FillBadDataFilter-D1` | SIMPLNX rejects a minimum size below 1; DREAM.3D v6.5.171 accepts it. |
| `FillBadDataFilter-D2` | SIMPLNX stops with a warning when no fill source exists; DREAM.3D v6.5.171 can keep looping. |
| `FillBadDataFilter-D3` | SIMPLNX excludes negative input IDs when sizing vote storage; DREAM.3D v6.5.171 converts them to unsigned values. |
| `FillBadDataFilter-D4` | SIMPLNX compares full-width component sizes; DREAM.3D v6.5.171 narrows them to Int32. |
| `FillBadDataFilter-D5` | SIMPLNX stores 64-bit neighbor indices; DREAM.3D v6.5.171 stores Int32 indices. |
