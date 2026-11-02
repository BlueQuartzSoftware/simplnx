# V&V Report: ErodeDilateBadDataFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `7f2f7378-580e-4337-8c04-a29e7883db0b` |
| SIMPLNX Human Name | Erode/Dilate Bad Data |
| DREAM.3D v6.5.171 equivalent | `ErodeDilateBadData` (Processing plugin, SIMPL UUID `3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Matthew Marine, 2026-08-07; second engineer: Michael Jackson, 2026-08-17 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `ErodeDilateBadData` in DREAM.3D v6.5.171. SIMPLNX uses rolling slice buffers for face-neighbor selection and tuple transfer. |
| Oracle (confirmed) | Class 4 checks preserve ignored arrays while FeatureIds change, including selection through an alias. Two fixtures encode these checks in `test/ErodeDilateBadDataTest.cpp`. |
| Code paths enumerated | 24 of 34 exercised. Cancellation, I/O failures, non-positive iterations and a partly zero geometry have no direct fixtures. |
| Tests today | 11 test cases: 10 ordinary cases cover comparisons, ignored arrays, validation and conversion; 1 hidden case generates inputs. |
| Exemplar archive | `6_6_erode_dilate_test.tar.gz` supplies inputs and stored outputs from a local proof version. The archive is a regression reference. |
| Legacy comparison | Not run on the current algorithm; the saved comparison has no recorded run date. |
| Bug flags | `ErodeDilateBadDataFilter-D2` |
| V&V phase | Complete. The non-positive iteration defect remains open. |

## Summary

`ErodeDilateBadDataFilter` grows or shrinks cells with FeatureId zero along the selected image axes and copies the associated cell data. Class 4 checks verify ignored-array preservation, while 28 parameter combinations and an archive comparison check stored regression values. All pass in CI.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps the legacy UUID to this filter; both implementations use six face neighbors, majority voting for erosion and the same face order.

## Oracle

*Class:* 4 (Invariant).

*Applied:* Require each ignored array to equal its input and require FeatureIds to change; also check that arrays retain their Attribute Matrix tuple dimensions.

*Encoded:* `src/Plugins/SimplnxCore/test/ErodeDilateBadDataTest.cpp::SimplnxCore::ErodeDilateBadDataFilter Ignored Path` and `SimplnxCore::ErodeDilateBadDataFilter: Ignored Array Cell AM Alias` — 2 fixtures. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-08-17.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fix for `ErodeDilateBadDataFilter-D1`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ErodeDilateBadDataFilter-D1` | Disabled axes still contributed face neighbors to erosion and dilation. | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. | The algorithm applies each direction flag to the two matching face neighbors. |

## Code path coverage

24 of 34 paths exercised. Each uncovered row states the missing fixture.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ErodeDilateBadData.cpp` (310 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ErodeDilateBadDataFilter.cpp` (210 lines), `src/simplnx/Utilities/NeighborUtilities.hpp` (634 lines), and `src/simplnx/Utilities/SliceBufferedTransfer.hpp` (364 lines).

Test names below omit the common `SimplnxCore::ErodeDilateBadDataFilter` prefix. The algorithm scans FeatureIds, selects source cells, then transfers completed slices.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Setup | Scan all slices to find the maximum FeatureId | `(Erode) Expanded`; `(Dilate) Expanded` |
| 2 | Scan | A nonzero FeatureId skips neighbor selection | Both `Expanded` cases compare unchanged cells |
| 3 | Scan | Dilate marks a good neighbor with the bad cell's source index | `(Dilate) Expanded` |
| 4 | Scan | Erode counts positive neighbor IDs and replaces the selected source only when its count exceeds the maximum | `(Erode) Expanded` |
| 5 | Scan | Erode resets counts for enabled, valid neighbors | `(Erode) Expanded` |
| 6 | Transfer | Copy the selected good-cell tuples into a bad cell | `(Erode) Expanded`; `(Erode)` |
| 7 | Transfer | Copy the selected bad-cell tuples into a good cell | `(Dilate) Expanded` |
| 8 | Transfer | A negative source mark preserves the destination tuple | Both `Expanded` cases compare all FeatureIds and Misc values |
| 9 | Preflight | All direction flags are false → error `-14601` | `No Direction` — both operations and both iteration counts |
| 10 | Preflight | All geometry dimensions are zero → error `-14602` | `No Dimensions` — direction flags stay enabled |
| 11 | Cancellation | Cancel during the initial FeatureId scan | *Not directly tested. Requires cancellation during a slice read.* |
| 12 | Preflight | FeatureIds are outside the selected Cell Attribute Matrix → error `-14605` | `: Strict Feature Ids` / `Feature Ids in another Attribute Matrix with the same tuple shape` |
| 13 | Preflight | An ignored array is outside the selected Cell Attribute Matrix → error `-14604` | `: Strict Feature Ids` / `Ignored array in another Attribute Matrix` |
| 14 | Selection | Resolve aliases to the selected Cell Attribute Matrix and exclude the ignored array | `: Ignored Array Cell AM Alias` |
| 15 | Scan | Enable or disable the two faces of each X, Y and Z axis | Both `Expanded` cases — seven non-empty axis combinations |
| 16 | Scan | Reject a face outside the image bounds | Both `Expanded` cases include boundary bad cells |
| 17 | Setup | Reset slice marks and read the previous output for another iteration | Both `Expanded` cases — two iterations |
| 18 | I/O | Load the first FeatureId slice for morphology | Both `Expanded` cases |
| 19 | I/O | Load a second slice when Z has more than one cell | Both `Expanded` cases |
| 20 | I/O | Rotate the window and read a later slice | `(Erode)` — the archive contains 20 slices |
| 21 | Transfer | Transfer the previous slice after all its marks are complete | Both `Expanded` cases |
| 22 | Transfer | Transfer the last slice of a multi-slice image | Both `Expanded` cases |
| 23 | Transfer | Transfer a single-slice image | `: Ignored Array Cell AM Alias` — FeatureIds change and the ignored array stays unchanged |
| 24 | Cancellation | Cancel immediately before the first morphology iteration | *Not directly tested. Requires cancellation before the first write.* |
| 25 | I/O | Propagate a read error from the initial maximum-ID scan | *Not directly tested. Requires an injected store read failure.* |
| 26 | I/O | Propagate failure while loading the first morphology slice | *Not directly tested. Requires an injected store read failure.* |
| 27 | I/O | Propagate failure while loading the second morphology slice | *Not directly tested. Requires an injected store read failure.* |
| 28 | I/O | Propagate failure while reading a slice after window rotation | *Not directly tested. Requires an injected store read failure.* |
| 29 | I/O | Propagate a tuple-transfer read or write failure for a previous slice | *Not directly tested. Requires an injected transfer failure.* |
| 30 | I/O | Propagate a tuple-transfer read or write failure for the final slice | *Not directly tested. Requires an injected transfer failure.* |
| 31 | Iteration | A non-positive count skips morphology | *Not directly tested. No fixture supplies zero or a negative count; see D2.* |
| 32 | Preflight | Emit warning `-14600` and the feature-data warning value | `: Preflight Feature Data Modification Warning` |
| 33 | I/O | Skip the next-slice read in a single-slice image | `: Ignored Array Cell AM Alias` |
| 34 | Preflight | One geometry dimension is zero while others are positive → error `-14602` | *Not directly tested. The existing fixture sets all three dimensions to zero.* |

## Test inventory

All ordinary cases are in `src/Plugins/SimplnxCore/test/ErodeDilateBadDataTest.cpp`. All pass in CI. The hidden generator is in `src/Plugins/SimplnxCore/test/ErodeDilateBadDataGeneratorTest.cpp`.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ErodeDilateBadDataFilter(Erode)` | kept | Runs two iterations with all axes enabled. Compares matching cell arrays with `Exemplar Bad Data Erode` and checks tuple dimensions. |
| `SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded` | new-for-V&V | Compares all 32 FeatureIds and Misc values with stored constants for seven direction combinations and one or two iterations: 14 cases. |
| `SimplnxCore::ErodeDilateBadDataFilter(Dilate) Expanded` | new-for-V&V | Same 14-case sweep for dilation. Both sweeps skip all-directions-off; `No Direction` checks its rejection. |
| `SimplnxCore::ErodeDilateBadDataFilter Ignored Path` | new-for-V&V | Checks unchanged Misc and changed FeatureIds for both operations. This is a Class 4 invariant. |
| `SimplnxCore::ErodeDilateBadDataFilter: Preflight Feature Data Modification Warning` | kept | Checks valid preflight, warning `-14600`, the feature-data warning value and absence of a `Stale Arrays` value. |
| `SimplnxCore::ErodeDilateBadDataFilter No Direction` | new-for-V&V | Checks error `-14601` for both operations and one or two iterations with all axes disabled. |
| `SimplnxCore::ErodeDilateBadDataFilter No Dimensions` | new-for-V&V | Checks error `-14602` for dimensions `{0,0,0}` with all direction flags enabled. |
| `SimplnxCore::ErodeDilateBadDataFilter: Strict Feature Ids` | new-for-V&V | Three sections check valid placement and exact errors `-14605` and `-14604`, including diagnostic paths. |
| `SimplnxCore::ErodeDilateBadDataFilter: Ignored Array Cell AM Alias` | new-for-V&V | Selects FeatureIds and an ignored array through an alias. Checks modification actions, unchanged ignored values and changed FeatureIds. |
| `SimplnxCore::ErodeDilateBadDataFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` variants check the converted UUID, operation, iteration count, axes and paths. They do not execute morphology. |
| `SimplnxCore::ErodeDilateBadDataFilter: Generate Test Data` | kept | Hidden (`[.GenerateTestData]`). Writes 20×20×20 and 200×200×200 synthetic inputs; does not execute morphology. |

## Exemplar archive

- **Archive:** `6_6_erode_dilate_test.tar.gz`
- **SHA512:** `5f0773e5d296936effbb2239965f5847e7c18533b0a2c3ec6a1d6a83b03417e5b459cce29808c8e0273613b3b6fa032c675e84926eb35d8da8a6ddc0641a0ef5`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/6_6_erode_dilate_test.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ErodeDilateBadDataFilter.md` for the root cause, affected users and recommendation. This table lists only Part 1 entries.

| Deviation | Observed difference |
|-----------|---------------------|
| `ErodeDilateBadDataFilter-D2` | SIMPLNX accepts non-positive iteration counts and skips morphology. DREAM.3D v6.5.171 rejects them with `-5555`. |
| `ErodeDilateBadDataFilter-D3` | SIMPLNX rejects all-directions-off with `-14601`. DREAM.3D v6.5.171 accepts that setting and leaves the input unchanged. |
