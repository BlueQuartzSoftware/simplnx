# V&V Report: ComputeFeaturePhasesFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `da5bb20e-4a8e-49d9-9434-fbab7bc434fc` |
| SIMPLNX Human Name | Compute Feature Phases |
| DREAM.3D v6.5.171 equivalent | `FindFeaturePhases` (Generic plugin, SIMPL UUID `6334ce16-cea5-5643-83b5-9573805873fa`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Nathan Young, 2026-07-15; second engineer: Michael Jackson, 2026-08-06 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes to DREAM.3D v6.5.171 `FindFeaturePhases`. SIMPLNX preserves cell order, skips background cells, rejects negative phases and limits warning text. |
| Oracle (confirmed) | Class 1 analytical outputs and Class 4 shape and warning checks are encoded in 10 inline fixtures. All pass in CI. |
| Code paths enumerated | 16 of 24 paths exercised. Gaps include cancellation, empty input, full chunks, I/O failures and plural warning text. |
| Tests today | 11 test cases: 10 inline fixtures and 1 SIMPL conversion test with two variants. All pass in CI. |
| Exemplar archive | `6_6_stats_test_v2.tar.gz` is retired for this filter. Current expectations are in the test source. |
| Legacy comparison | Not run on the current algorithm. No saved results support the reported 2026-07-10 comparison, and the algorithm changed afterward. |
| Bug flags | None |
| V&V phase | Analytical verification and release-source checks are complete. Second-engineer review is not recorded. |

## Summary

`ComputeFeaturePhasesFilter` assigns each positive feature the phase of its last cell in input order. Hand-derived fixtures check output values, background handling, warnings and invalid inputs. All pass in CI; DREAM3D-NX v7.4.2 contains the negative-phase validation fix.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps `FindFeaturePhases` to this filter; both use the last cell phase, while SIMPLNX adds validation, background exclusion and chunked reads.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) shape and warning checks.

*Applied:* Hand-written cell IDs and phases determine the last phase for each positive feature; background and unused feature entries stay zero.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeFeaturePhasesFilterTest.cpp::SimplnxCore::ComputeFeaturePhasesFilter: Valid: Uniform Phases` and nine companion fixtures check exact outputs, warning IDs and error codes. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-08-06.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeaturePhasesFilter-D2`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeaturePhasesFilter-D2` | A negative cell phase could be copied into feature output without an error. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was affected. | The filter rejects a negative phase on a positive feature with error `-61861`. |

## Code path coverage

16 of 24 paths exercised. Each uncovered path is listed below.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeaturePhases.cpp` (145 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeaturePhasesFilter.cpp` (148 lines), and `src/simplnx/Utilities/DataArrayUtilities.cpp` (515 lines, shared FeatureId validator).

Test names below omit the common `SimplnxCore::ComputeFeaturePhasesFilter: ` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Unequal cell-array tuple counts return `-61860` | `Invalid: Cell Array Size Mismatch` |
| 2 | Preflight | Equal tuple counts create a scalar feature array with the Feature Attribute Matrix shape | `Valid: Uniform Phases` |
| 3 | Validation | Negative FeatureId returns `-5355` | `Invalid: Negative FeatureId` |
| 4 | Validation | FeatureId exceeds the feature-array range and returns `-5351` | `Invalid: FeatureId Out Of Bounds` |
| 5 | Validation | FeatureId bulk read fails and returns its error | *Not directly tested. No fixture injects a validator read failure.* |
| 6 | Input | Empty input skips validation reads and the cell loop | *Not directly tested. Every fixture has cells.* |
| 7 | Input | Cancellation stops processing before a chunk read | *Not directly tested. No fixture injects cancellation.* |
| 8 | Input | Full chunks and later chunks preserve phase order across chunk boundaries | *Not directly tested. The largest fixture has 32 cells, below both the validator and algorithm chunk sizes.* |
| 9 | Input | A partial chunk reads both cell arrays successfully | `Valid: Uniform Phases` |
| 10 | Input | Algorithm FeatureId read fails and returns its error | *Not directly tested. No fixture injects this read failure.* |
| 11 | Input | Cell-phase read fails and returns its error | *Not directly tested. No fixture injects this read failure.* |
| 12 | Assignment | Background cells with positive phases leave feature zero at zero | `Valid: Feature 0 Skip` |
| 13 | Assignment | Background cells with negative phases bypass phase validation | `Valid: Negative Cell Phase On Background Cell` |
| 14 | Assignment | A negative phase on a positive feature returns `-61861` | `Invalid: Negative Cell Phase` |
| 15 | Assignment | First cell of a feature sets its phase | `Valid: Uniform Phases` |
| 16 | Assignment | A later cell with the same phase causes no conflict | `Valid: Uniform Phases` |
| 17 | Assignment | A later cell with a different phase marks the feature and replaces its phase | `Valid: Mixed Phase Warning` |
| 18 | Assignment | A feature with no cells stays zero | `Valid: Gap Feature Stays Zero` |
| 19 | Output | Bulk output write stores the feature values | `Valid: Uniform Phases` |
| 20 | Output | Bulk output write fails and returns its error | *Not directly tested. No fixture injects a write failure.* |
| 21 | Warning | No conflicting features return no warnings | `Valid: Uniform Phases` |
| 22 | Warning | Up to 15 conflicting features produce one warning `-500` with their IDs | `Valid: Mixed Phase Warning` |
| 23 | Warning | More than 15 conflicting features limit the list and append the omitted count | `Valid: Truncated Mixed Feature Warning` — checks IDs 1 through 15 and the text `and 1 more` |
| 24 | Warning | More than one omitted feature selects the plural suffix | *Not directly tested. The truncation fixture omits exactly one feature.* |

## Test inventory

All cases are in `src/Plugins/SimplnxCore/test/ComputeFeaturePhasesFilterTest.cpp`. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Uniform Phases` | new-for-V&V | Seven cells produce `[0,1,2,1]`; checks four output tuples and no warnings. |
| `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Feature 0 Skip` | new-for-V&V | Six cells produce `[0,1,2]`; checks background exclusion and no warnings (D1). |
| `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Mixed Phase Warning` | new-for-V&V | Two cells give feature 1 phase 2; checks warning `-500` and exactly the listed ID 1 (D3). |
| `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Truncated Mixed Feature Warning` | new-for-V&V | Sixteen conflicting features list IDs 1 through 15 and the text `and 1 more` (D3). Does not assert output phase values. |
| `SimplnxCore::ComputeFeaturePhasesFilter: Invalid: FeatureId Out Of Bounds` | new-for-V&V | FeatureId 5 with three feature tuples returns `-5351`. |
| `SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Cell Array Size Mismatch` | new-for-V&V | Four FeatureId tuples and five phase tuples return preflight error `-61860`. |
| `SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Negative Cell Phase` | new-for-V&V | Phase -1 on feature 1 returns `-61861` (D2). |
| `SimplnxCore::ComputeFeaturePhasesFilter: Invalid: Negative FeatureId` | new-for-V&V | FeatureId -1 returns `-5355`. |
| `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Gap Feature Stays Zero` | new-for-V&V | Four cells use features 1 and 3; compares all five outputs with `[0,1,0,2,0]`. |
| `SimplnxCore::ComputeFeaturePhasesFilter: Valid: Negative Cell Phase On Background Cell` | new-for-V&V | Background phase -5 causes no error or warning; output is `[0,2]` (D1, D2). |
| `SimplnxCore::ComputeFeaturePhasesFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` variants check filter identity, comments and all four converted arguments. |

## Exemplar archive

- **Archive:** `6_6_stats_test_v2.tar.gz` — retired for this filter; the 10 inline fixtures replace its output comparison.
- **SHA512:** `e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723`
- **Provenance:** [src/Plugins/SimplnxCore/vv/provenance/ComputeFeaturePhasesFilter.md](provenance/ComputeFeaturePhasesFilter.md)

## Deviations from DREAM.3D v6.5.171

See [vv/deviations/ComputeFeaturePhasesFilter.md](deviations/ComputeFeaturePhasesFilter.md) for the source-based differences, affected users and recommendations.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeaturePhasesFilter-D1` | SIMPLNX leaves feature zero at zero; DREAM.3D v6.5.171 stores the last background cell phase. |
| `ComputeFeaturePhasesFilter-D2` | SIMPLNX rejects negative phases on positive features; DREAM.3D v6.5.171 accepts them. |
| `ComputeFeaturePhasesFilter-D3` | SIMPLNX lists up to 15 conflicting feature IDs in one warning; DREAM.3D v6.5.171 lists a conflict count for every affected feature. |
