# V&V Report: RequireMinNumNeighborsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| SIMPLNX Human Name | Require Minimum Number of Neighbors |
| DREAM.3D v6.5.171 equivalent | `MinNeighbors` (Processing plugin, SIMPL UUID `dab5de3c-5f81-5bb5-8490-73521e1183ea`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Jared Duffey, 2026-07-31; second engineer: Michael Jackson, 2026-08-07 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to `MinNeighbors` in DREAM.3D v6.5.171. SIMPLNX retains threshold selection and face-neighbor voting, with input guards and bounded cell transfers. |
| Oracle (confirmed) | Class 1 analytical values and Class 4 invariants are encoded in two tests with three distinct inputs. All pass in CI. |
| Code paths enumerated | 20 of 30 paths exercised. Gaps include cancellation, store failures and defensive helper paths. |
| Tests today | 14 test cases cover both phase modes, exact cell copying, feature compaction, errors, shared paths, Small IN100 and SIMPL conversion. All pass in CI. |
| Exemplar archive | `6_5_test_data_1_v2.tar.gz` supplies Small IN100 input. The analytical expectations are encoded in the tests. |
| Legacy comparison | Run against DREAM.3D v6.5.171 on 2026-07-31: 2,646 values in 22 arrays over four fixture and mode variants matched DREAM.3D and the analytical oracle. The run predates the DREAM3D-NX algorithm changes of 2026-09-20; run the comparison again on the current algorithm. |
| Bug flags | None |
| V&V phase | Analytical verification and second-engineer sign-off are complete. A legacy comparison with the current algorithm remains open. |

## Summary

`RequireMinNumNeighborsFilter` removes features below a neighbor-count threshold, fills their cells from dominant valid face neighbors, and compacts feature arrays. Class 1 fixtures and Class 4 invariants check both phase modes, with regression and input-validation tests. All pass in CI; DREAM3D-NX v7.4.2 contains the three inherited defect fixes.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps `MinNeighbors` to this filter; both use threshold selection and six-face voting, while SIMPLNX adds safety guards and compacts cell IDs before filling.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Derive exact copied values and compacted feature IDs from three input fixtures; check ignored values, ID bounds and inherited tuple dimensions.

*Encoded:* `src/Plugins/SimplnxCore/test/RequireMinNumNeighborsTest.cpp::SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` and `SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture` — two mode-specific 4×1×1 inputs and one 6×6×6 input in both modes. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-08-07.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `RequireMinNumNeighborsFilter-D1`, `RequireMinNumNeighborsFilter-D2` and `RequireMinNumNeighborsFilter-D3`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `RequireMinNumNeighborsFilter-D1` | Negative FeatureIds caused an out-of-bounds feature-state lookup. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | Negative IDs bypass the lookup and enter face-neighbor reassignment. |
| `RequireMinNumNeighborsFilter-D2` | A FeatureId at or above the feature tuple count reached an unchecked feature-state lookup. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The marking pass returns `-55567` before the invalid lookup. |
| `RequireMinNumNeighborsFilter-D3` | Coarsening could loop indefinitely when unresolved cells had no valid face neighbor. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | A scan with unresolved cells and no fill candidate returns `-55572`. |

## Code path coverage

20 of 30 paths exercised. Each uncovered path is listed below.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RequireMinNumNeighbors.cpp` (228 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/FillBadVoxels.cpp` (469 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RequireMinNumNeighborsFilter.cpp` (237 lines).

Test names below omit the common `SimplnxCore::RequireMinNumNeighborsFilter: ` prefix. `Small IN100 regression` means `SimplnxCore::RequireMinNumNeighborsFilter`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Feature array tuple counts differ → `-252`. | `Preflight Error - tuple count mismatch (-252)` |
| 2 | Preflight | FeatureIds is outside the geometry's Cell Attribute Matrix → `-55575`. | `Preflight Error - FeatureIds outside Cell Attribute Matrix (-55575)`; `Strict Feature Ids` |
| 3 | Preflight | FeatureIds tuple count differs from the geometry cell count → `-59571`. | `Selected cell tuple counts` |
| 4 | Preflight | An ignored array is outside the Cell Attribute Matrix → `-55573`. | `Strict Feature Ids` — `Ignored array in a second Attribute Matrix` |
| 5 | Preflight | Number of Neighbors has an invalid Feature Attribute Matrix parent → `-55574`. | `Strict Feature Ids` — external, plain-group and Cell Attribute Matrix parents |
| 6 | Paths | Resolve shared Cell paths and compact the selected Feature Attribute Matrix. | `Shared Cell Paths and Selected Feature Matrix` — both alias variants |
| 7 | Preflight | Warn that NeighborLists will be removed. | Small IN100 regression — checks one warning, `-5558` |
| 8 | Selection | Requested phase is absent → `-5555`. | `Execute Error - unavailable phase (-5555)` |
| 9 | Selection | No nonbackground feature survives → `-55569`. | `Execute Error - all features rejected (-55569)` |
| 10 | Selection | All-phase mode retains features at or above the threshold. | `Analytical Oracle` — `ApplyToSinglePhase=0`; `Discriminating 6x6x6 Analytical Fixture` |
| 11 | Selection | Single-phase mode also retains other phases, including a feature with no cells. | `Analytical Oracle` — `ApplyToSinglePhase=1`; `Discriminating 6x6x6 Analytical Fixture` |
| 12 | Marking | Negative IDs bypass feature-state indexing and enter reassignment. | `Execute - negative feature ID is reassigned` |
| 13 | Marking | Nonnegative ID exceeds the feature range → `-55567`. | `Execute Error - feature ID out of range (-55567)` |
| 14 | Marking | Mark rejected cells and renumber retained IDs in full and partial 65,536-cell transfers. | Small IN100 regression — million-cell input; analytical fixtures check exact remapped IDs |
| 15 | Voting | Clip all six faces and preserve the first winning feature in a tie. | `Discriminating 6x6x6 Analytical Fixture` — boundary and source-index assertions |
| 16 | Filling | Repeat propagation to fill an enclosed rejected cell. | `Discriminating 6x6x6 Analytical Fixture` — cube center uses the second-pass source |
| 17 | Copy | Copy scalar and vector tuples while preserving ignored arrays. | `Analytical Oracle`; `Discriminating 6x6x6 Analytical Fixture` |
| 18 | Finalize | Compact feature arrays with the same mapping used for cell IDs. | Both analytical tests; `Shared Cell Paths and Selected Feature Matrix` |
| 19 | Filling | Unresolved cells have no fill candidate → `-55572`. | `Execute Error - no coarsening progress (-55572)` |
| 20 | Filling | No negative IDs remain → finish without another copy pass. | `Execute - negative feature ID is reassigned`; both analytical tests |
| 21 | Setup | A geometry dimension is zero → skip filling. | *Not directly tested. No fixture has a zero-sized geometry.* |
| 22 | Cancellation | Stop during selection, marking, filling or compaction. | *Not directly tested. No fixture injects cancellation.* |
| 23 | Validation | The shared fill scan finds an out-of-range positive ID → `-55567`. | *Not directly tested. The marking pass rejects this input before the shared scan.* |
| 24 | I/O | Marking reads or writes fail → propagate the store error. | *Not directly tested. No fixture injects a marking-store failure.* |
| 25 | I/O | Slice reads or writes fail during scanning, tuple copying or ID filling → propagate the error. | *Not directly tested. No fixture injects a slice-store failure.* |
| 26 | Finalize | Inactive-feature removal fails → propagate its error. | *Not directly tested. No fixture forces the compaction helper to fail.* |
| 27 | Filling | FeatureIds is selected as ignored but is still filled last. | *Not directly tested. No fixture includes FeatureIds in the ignored selection.* |
| 28 | Copy | Dispatch cell-array types other than the analytical int32 and float32 types. | *Not directly tested. No independent fixture checks exact copied values for each remaining type.* |
| 29 | Preflight | Report stale feature arrays in updated preflight values. | *Not directly tested. Tests do not assert the updated-value text.* |
| 30 | Finalize | Delete feature NeighborLists. | *Not directly tested. The regression checks the warning but does not assert that the arrays are absent.* |

## Test inventory

All 14 test cases are in `src/Plugins/SimplnxCore/test/RequireMinNumNeighborsTest.cpp`. All pass in CI. Dynamic sections are listed separately below.

| Test case | Status | Notes |
|---|---|---|
| `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` | new-for-V&V | Class 1/4 harness for two generated 4x1x1 variants. Each variant checks five arrays, 18 exact element values, eight final-ID invariants, feature-array tuple counts, and inherited tuple dimensions. |
| `ApplyToSinglePhase=0` | new-for-V&V | `DYNAMIC_SECTION` generated by `GENERATE(false, true)`; uses input `NumNeighbors [0,0,3,3]` and verifies all-phase selection and compaction. |
| `ApplyToSinglePhase=1` | new-for-V&V | `DYNAMIC_SECTION` generated by `GENERATE(false, true)`; uses input `NumNeighbors [0,0,0,3]` and verifies that nonselected phase-2 feature 2 remains active. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture` | new-for-V&V | Runs one common input in both phase modes. Checks six arrays with 2,610 exact element comparisons, a 1-vs-1 traversal-order tie, X/Y/Z boundaries, second-pass copying, ignored values, zero-cell feature retention, equal cell output across modes, and tuple dimensions. |
| `SimplnxCore::RequireMinNumNeighborsFilter` | kept | Modified: loads `6_5_test_data_1_v2.tar.gz`, recomputes neighbors, derives the expected retained `NumElements` sequence from the pre-filter input and threshold, checks every retained value, asserts the single `-5558` preflight warning, and verifies tuple dimensions. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)` | kept | Creates 5-tuple `NumNeighbors` and 4-tuple `Phases` arrays, then asserts invalid preflight and exact code `-252`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds outside Cell Attribute Matrix (-55575)` | new-for-V&V | Places `FeatureIds` in a second Attribute Matrix with the same tuple count as the Cell Attribute Matrix, then asserts invalid preflight and exact code `-55575`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)` | new-for-V&V | Supplies `FeatureIds [1,4,2,3]` for four feature tuples, asserts valid preflight, invalid execution, and exact code `-55567`; covers D2. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned` | new-for-V&V | Supplies `FeatureIds [-1,1,1,2]`, asserts successful execution, and checks all four final IDs equal `[1,1,1,2]`; covers D1. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)` | new-for-V&V | Selects absent phase 5, then asserts valid preflight, invalid execution, and exact code `-5555`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)` | new-for-V&V | Gives every nonzero feature zero neighbors with threshold 1, then asserts valid preflight, invalid execution, and exact code `-55569`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - no coarsening progress (-55572)` | new-for-V&V | Uses four negative cells with an otherwise active feature, asserts one execution error with code `-55572`, and verifies all four `FeatureIds` remain `-1`; covers D3. |
| `SimplnxCore::RequireMinNumNeighborsFilter: SIMPL Backwards Compatibility` | kept | Loads the two conversion fixtures below and verifies one converted filter, UUID, comments, and seven converted arguments in each section. |
| `SIMPL 6.5 (UUID)` | kept | `DYNAMIC_SECTION` using `test/simpl_conversion/6_5/RequireMinNumNeighborsFilter.json`; verifies UUID-based 6.5 pipeline conversion. |
| `SIMPL 6.4 (Filter_Name)` | kept | `DYNAMIC_SECTION` using `test/simpl_conversion/6_4/RequireMinNumNeighborsFilter.json`; verifies name-based 6.4 pipeline conversion. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Selected cell tuple counts` | new-for-V&V | Creates a 4-cell geometry with 3, 4, or 5 `FeatureIds` tuples stored separately. Equal counts pass; mismatches assert invalid preflight, exact code `-59571`, selected paths, tuple count, and cell count. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Strict Feature Ids` | new-for-V&V | Checks valid Cell and Feature Attribute Matrices, shared Cell paths, and read-only phases in another matrix. Rejects misplaced Feature Ids (`-55575`), ignored arrays (`-55573`), and invalid Feature Attribute Matrix locations (`-55574`); checks each error code and affected paths. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Shared Cell Paths and Selected Feature Matrix` | new-for-V&V | Alternates geometry and shared-parent paths for Feature Ids and ignored arrays. Checks geometry-based modification paths, reassigned IDs and cell values, unchanged ignored values, selected Feature Attribute Matrix compaction, unchanged external marker values, and inherited tuple dimensions. |

## Exemplar archive

- **Archive:** `6_5_test_data_1_v2.tar.gz`
- **SHA512:** `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/6_5_test_data_1_v2.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/RequireMinNumNeighborsFilter.md` for the root cause, affected users and recommendation. Source comparison establishes these Part 1 differences.

| Deviation | Observed difference |
|-----------|---------------------|
| `RequireMinNumNeighborsFilter-D1` | SIMPLNX reassigns negative FeatureIds safely; DREAM.3D v6.5.171 uses them as unchecked feature-state indices. |
| `RequireMinNumNeighborsFilter-D2` | SIMPLNX rejects out-of-range nonnegative FeatureIds with `-55567`; DREAM.3D v6.5.171 uses them as unchecked indices. |
| `RequireMinNumNeighborsFilter-D3` | SIMPLNX returns `-55572` when coarsening cannot progress; DREAM.3D v6.5.171 continues the loop. |
