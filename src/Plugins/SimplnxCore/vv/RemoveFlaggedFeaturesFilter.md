# V&V Report: RemoveFlaggedFeaturesFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| SIMPLNX Human Name | Remove/Extract Flagged Features |
| DREAM.3D v6.5.171 equivalent | `RemoveFlaggedFeatures` (Processing) and `ExtractFlaggedFeatures` (Sampling) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-09-02; second engineer: Nathan Young, 2026-09-08 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes. SIMPLNX combines the two DREAM.3D v6.5.171 filters and adds validation to removal and extraction. |
| Oracle (confirmed) | Classes 1 and 4: hand-derived cell copies and feature compaction, plus Small IN100 invariants in `test/RemoveFlaggedFeaturesTest.cpp`. All pass in CI. |
| Code paths enumerated | 31 of 49 exercised. Gaps cover defensive failures, some cancellation checkpoints and inputs absent from the fixtures. |
| Tests today | 28 test cases cover all three operations, fill on/off, errors, progress, shared paths and SIMPL conversion. All pass in CI. |
| Exemplar archive | `6_5_test_data_1_v2.tar.gz` supplies Small IN100 input. Expected outputs are derived in the tests. |
| Legacy comparison | Not run for the current algorithm. The DREAM.3D v6.5.171 comparison dated 2026-09-02 predates algorithm changes; its results are in provenance. |
| Bug flags | None. |
| V&V phase | Oracle and source checks are complete. A legacy comparison with the current algorithm remains open. |

## Summary

`RemoveFlaggedFeaturesFilter` removes flagged features, extracts their bounding boxes, or does both. Class 1 fixtures check exact cell copies and feature compaction; a Class 4 test checks Small IN100 invariants. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* The Direct and Scanline implementations preserve the legacy mark, face-neighbor fill and compaction rule; extraction delegates bounds and crops to `ComputeFeatureRectFilter` and `CropImageGeometryFilter`.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Four hand-built fixtures (5×2×1, 4×4×1, 3×3×3 and a 12-feature line) define exact IDs, copy sources and crops; Small IN100 supplies compaction and conservation invariants.

*Encoded:* `src/Plugins/SimplnxCore/test/RemoveFlaggedFeaturesTest.cpp::Class 1 Oracle - *` (8 cases) and `Class 4 Invariants - Small IN100 remove small features with fill` (1 case). All pass in CI.

*Second-engineer review:* Nathan Young, 2026-09-08.

## Bugs found and fixed

All defects in this table are fixed. The fixes will be in the DREAM3D-NX release after v7.4.2. DREAM3D-NX v7.4.2 contains the fix for `RemoveFlaggedFeaturesFilter-D1`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `RemoveFlaggedFeaturesFilter-D1` | A single valid neighbor or distinct neighbor features could leave fill running forever. | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. | The first sighting counts as a hit and can supply the cell. |
| `RemoveFlaggedFeaturesFilter-D2` | Background cells kept the fill loop running forever. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Nonnegative cells are never fill targets; background remains a valid source. |
| `RemoveFlaggedFeaturesFilter-D3` | Fill could loop forever without a source or when FeatureIds was ignored. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | No progress returns `-45436`; FeatureIds cannot be ignored and gives warning `-45438`. |
| `RemoveFlaggedFeaturesFilter-D4` | Invalid FeatureIds or tuple counts permitted out-of-bounds access. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | Removal validates IDs and tuple counts before changes, with errors `-45435` and `-45437`. |
| `RemoveFlaggedFeaturesFilter-D5` | Extraction ignored delegated execution errors and threw on preflight errors. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected by this result-handling defect. | Delegated failures return `-53901` through `-53904` with the cause. |
| `RemoveFlaggedFeaturesFilter-D6` | An empty flagged feature caused an exception or a false crop. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was affected. | Warning `-53905` lists empty features; the filter creates no geometry for them. |
| `RemoveFlaggedFeaturesFilter-D8` | The internal bounds array remained in source and extracted feature data. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Bounds are copied into a local buffer; `tempBounds` is removed before crops and after bounds-execution failure. |
| `RemoveFlaggedFeaturesFilter-D9` | Preflight gave duplicate NeighborList deletion warnings. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D v6.5.171 was not affected. | Removal gives one warning `-5558`; extraction alone gives none. |

## Code path coverage

31 of 49 paths exercised. Each gap is listed below. Direct and Scanline tests use in-memory stores.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RemoveFlaggedFeatures.cpp` (50 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RemoveFlaggedFeaturesDirect.cpp` (562 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RemoveFlaggedFeaturesScanline.cpp` (838 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RemoveFlaggedFeaturesCommon.hpp` (120 lines); `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/RemoveFlaggedFeaturesFilter.cpp` (233 lines).

Test names below omit the `SimplnxCore::RemoveFlaggedFeaturesFilter:` or `SimplnxCore::RemoveFlaggedFeatures:` prefix.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | (a) Preflight | Flag array parent is not a Feature Attribute Matrix in the selected geometry → error `-9895` | `Preflight Error - flag array parent is not an Attribute Matrix (-9895)`; `Strict Feature Ids` |
| 2 | (b) Extract | Temporary `tempBounds` array is created by the `ComputeFeatureRect` sub-filter and deleted by the algorithm before the first crop, so no extracted geometry carries it | `Class 1 Oracle - Extract crops the bounding box of each flagged feature` (asserts `tempBounds` is absent from the source and from the extracted geometry) |
| 3 | (a) Preflight | *Remove* or *Extract then Remove* -> cell and feature groups marked modified, NeighborLists scheduled for deletion, one warning `-5558` | `Preflight Warning - NeighborLists are removed (-5558)` (`Functionality = 0`, `Functionality = 2`) |
| 4 | (a) Preflight | *Extract* -> no NeighborList warning, NeighborLists kept | `Preflight Warning - NeighborLists are removed (-5558)` (`Extract alone does not remove NeighborLists`) |
| 5 | Validation | Missing or unsupported flag array → error `-53900` | *Not directly tested. Parameter validation rejects unsupported arrays before execution.* |
| 6 | (b) Extract | `ComputeFeatureRect` preflight fails -> error `-53901` | *Not directly tested. No fixture forces the delegated preflight to fail.* |
| 7 | (b) Extract | `ComputeFeatureRect` execute fails -> error `-53902`, `tempBounds` removed | *Not directly tested. No Extract-only fixture supplies an out-of-range FeatureId.* |
| 8 | (b) Extract | One or more flagged features own no cell -> one warning `-53905` listing them, no geometry for them | `Extract - flagged feature with no cells is skipped with a warning` (two empty features, one warning) |
| 9 | (b) Extract | Flagged feature -> new geometry `<prefix>-<zero-padded id>` with the bounding-box dimensions, origin, all cell arrays and the feature Attribute Matrix | `Class 1 Oracle - Extract crops the bounding box of each flagged feature`, `Class 1 Oracle - Extract zero-pads names and handles several features`, `Class 1 Oracle - Extract then Remove with fill` |
| 10 | (b) Extract | Unflagged feature -> no geometry | `Class 1 Oracle - Extract zero-pads names and handles several features` |
| 11 | (b) Extract | Crop preflight fails -> error `-53903` | *Not directly tested. No fixture forces crop preflight to fail.* |
| 12 | (b) Extract | Crop execute fails -> error `-53904` | *Not directly tested. Requires a delegated execution failure.* |
| 13 | (b) Extract | Cancellation at extraction checkpoints outside the crop task | *Not directly tested. The cancellation test injects cancellation inside the crop task instead.* |
| 14 | (c) Marking | A FeatureId is negative or >= feature tuple count -> error `-45435`, nothing modified | `Execute Error - FeatureId out of range (-45435)` (both sections) |
| 15 | (c) Marking | Feature Ids tuple count differs from the geometry cell count -> error `-45437`, nothing modified | `Execute Error - Feature Ids tuple count differs from the geometry (-45437)` |
| 16 | (c) Marking | Fewer than two feature tuples -> error `-45433`, nothing modified | *Not directly tested. No fixture has fewer than two feature tuples.* |
| 17 | (c) Marking | Every feature flagged -> error `-45433`, nothing modified | `Execute Error - all features flagged (-45433)` (`Fill = false`, `Fill = true`) |
| 18 | (c) Marking | Fill off: flagged cell -> 0 | `Class 1 Oracle - Remove without fill` |
| 19 | (c) Marking | Fill on: flagged cell -> -1 | Every `Class 1 Oracle - Fill *` test |
| 20 | (d) Fill | Vacated cell with a single valid neighbor, or all-distinct neighbors: first sighting records the source | `Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)` (cells 3 and 9) |
| 21 | (d) Fill | Majority vote: the source moves only when a feature's tally strictly exceeds the previous maximum | `Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` (cell 13, 4 to 2) |
| 22 | (d) Fill | 1-vs-1 tie -> first-seen feature in -Z, -Y, -X, +X, +Y, +Z order | `Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)` (cells 3 and 4) |
| 23 | (d) Fill | A vacated cell with no nonnegative neighbor is filled in a later iteration from a neighbor that was itself filled | `Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)`; `Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` |
| 24 | (d) Fill | Background cell (FeatureId 0) is skipped as a target and counted as a source | `Class 1 Oracle - Fill treats FeatureId 0 as a source and never a target` (cell 14 -> 0) |
| 25 | (d) Fill | Vacated cells remain and the pass filled none -> error `-45436`, cells left at -1 | `Execute Error - no fill progress (-45436)` |
| 26 | (d) Fill | Feature Ids array listed in the ignore list -> removed from the list, warning `-45438`, fill proceeds | `Fill cannot ignore the Feature Ids array (-45438 warning)` |
| 27 | (d) Fill | Every non-ignored cell array is copied from the source cell; ignored arrays are untouched | `Class 1 Oracle - Fill copies every non-ignored Cell array` and the `IgnoredValue` checks in every fill test |
| 28 | (d) Fill | Face neighbors outside the volume are excluded | `Class 1 Oracle - Remove without fill`; `Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` |
| 29 | (d) Fill | Cancel during a pass -> return | *Not directly tested. Requires cancel-signal injection.* |
| 30 | (e) Compaction | Surviving features renumbered contiguously from 1 in input order; feature arrays compacted in place; NeighborLists deleted | Every remove test (`CheckFeatureArraysCompacted`), `Class 4 Invariants - Small IN100 remove small features with fill` |
| 31 | (e) Compaction | Feature compaction fails → propagate its error | *Not directly tested. Requires a compaction failure.* |
| 32 | (a) Preflight | Cell Feature Ids is not in the selected geometry's Cell Attribute Matrix (every operation) -> error `-9893` | `Strict Feature Ids` — rejects a second Attribute Matrix; accepts another path to the same Cell Attribute Matrix. |
| 33 | (a) Preflight | *Remove* or *Extract then Remove* with fill on, and an Attribute Arrays to Ignore entry is not in the selected geometry's Cell Attribute Matrix -> error `-9894` | `Strict Feature Ids` — rejects misplaced ignored arrays only when removal and fill are enabled. |
| 34 | (d) Fill, (e) Compaction | Feature Ids or ignored arrays selected through a second path to the Cell Attribute Matrix -> modifications are reported on the geometry's Cell path, ignored arrays are untouched, and only the selected Feature Attribute Matrix is compacted | `Shared Cell Paths and Selected Feature Matrix` — both Cell paths. |
| 35 | Dispatch | Direct algorithm, including scalar, vector and Boolean copies | `fill direct and scanline parity` |
| 36 | Dispatch | Scanline algorithm, including slice transfers and final slice | `fill direct and scanline parity`; `Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` |
| 37 | Extract | Progress reaches 100 before the success message | `Extraction progress completes` |
| 38 | Extract | Cancellation before delegated crop execution suppresses completion | `Extraction progress does not complete after cancellation` |
| 39 | Preflight | Stale feature-array warning only for operations that remove features | `Preflight Stale Arrays Only When Modifying` |
| 40 | Validation | Boolean feature flags | `Class 1 Oracle - Remove without fill` |
| 41 | Validation | UInt8 feature flags | *Not directly tested. The fixtures use Boolean flags.* |
| 42 | Storage | Bulk read or write failure during validation, marking, fill or renumbering | *Not directly tested. Requires a store that injects transfer failures.* |
| 43 | Extract | Bounds-buffer read failure; remove the temporary array and return the error | *Not directly tested. Requires a bounds-store read failure.* |
| 44 | Scanline fill | Invalid component shape or slice-size overflow → error `-45435` | *Not directly tested. Requires invalid array metadata.* |
| 45 | Scanline fill | Source slice outside the geometry → error `-45436` | *Not directly tested. Requires an invalid internal source mark.* |
| 46 | Scanline fill | Zero Z dimension → return without filling | *Not directly tested. All fixtures have nonzero dimensions.* |
| 47 | Removal | No flagged features → leave labels unchanged | *Not directly tested. Each successful removal fixture flags at least one feature.* |
| 48 | Extract | More than ten empty features → truncate the warning list | *Not directly tested. The empty-feature fixture has two empty features.* |
| 49 | Validation/compaction | Cancellation during validation, marking or feature compaction | *Not directly tested. Requires cancellation at those checkpoints.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Remove without fill` | new-for-V&V | Fixtures A and B: exact FeatureIds, unchanged cell arrays and compacted feature arrays; fill disabled. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)` | new-for-V&V | Fixture A: exact IDs and copy sources for single neighbors, distinct neighbors, ties and a second fill iteration (D1). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill copies every non-ignored Cell array` | new-for-V&V | Fixture A with an empty ignore list: both companion arrays follow the selected copy sources. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Fill cannot ignore the Feature Ids array (-45438 warning)` | new-for-V&V | Exactly one warning `-45438`; FeatureIds still fill when selected in the ignore list (D3). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill treats FeatureId 0 as a source and never a target` | new-for-V&V | Fixture B: background cells remain zero and can supply a removed cell (D2). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` | new-for-V&V | Fixture E: all 27 IDs, six face directions, a 4-to-2 majority and a two-iteration copy chain. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - all features flagged (-45433)` | new-for-V&V | Fill off/on dynamic sections: error `-45433` and unchanged arrays. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - FeatureId out of range (-45435)` | new-for-V&V | Negative and upper-bound IDs: error `-45435` before modification (D4). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - Feature Ids tuple count differs from the geometry (-45437)` | new-for-V&V | Geometry and FeatureIds tuple counts differ: error `-45437` before modification (D4). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - no fill progress (-45436)` | new-for-V&V | No unflagged feature owns cells: error `-45436`; removed IDs remain -1 (D3). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Extraction progress completes` | kept | Extraction progress never decreases and reaches 100 before success; extracted IDs equal `[3,3]`. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Extraction progress does not complete after cancellation` | kept | Cancellation at the first extraction progress message: error `-1`, no geometry and no completion message. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract crops the bounding box of each flagged feature` | new-for-V&V | Fixture B: dimensions, origin, spacing, all three cell arrays and copied feature arrays; no `tempBounds` (D8). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract zero-pads names and handles several features` | new-for-V&V | Features 3 and 11: two-digit names, exact crops and cell values; no geometry for unflagged features. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Extract - flagged feature with no cells is skipped with a warning` | new-for-V&V | Two empty features: one warning `-53905` naming both; only the populated feature is extracted (D6). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract then Remove with fill` | new-for-V&V | Fixture B: crop uses the original values; removal then fills and compacts the original geometry. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Stale Arrays Only When Modifying` | kept | Stale feature-array notice appears for Remove and Extract then Remove; absent for Extract. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Error - flag array parent is not an Attribute Matrix (-9895)` | new-for-V&V | Flag array under a plain DataGroup: preflight error `-9895`. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Warning - NeighborLists are removed (-5558)` | new-for-V&V | Remove and Extract then Remove dynamic sections: exactly one `-5558` warning and NeighborList deletion; Extract preserves it (D9). |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 4 Invariants - Small IN100 remove small features with fill` | new-for-V&V | Small IN100: flags 275 features and checks compaction, valid IDs, phase agreement, NeighborList removal and conservation of 8,535 filled cells. |
| `SimplnxCore::RemoveFlaggedFeatures: Test Remove Algorithm` | kept | Fill disabled: exact 4×4×1 FeatureIds and compacted feature values. |
| `SimplnxCore::RemoveFlaggedFeatures: Test Extract Algorithm` | kept | Extracted IDs `[3,3]`; original IDs and original feature tuples remain unchanged. |
| `SimplnxCore::RemoveFlaggedFeatures: fill direct and scanline parity` | kept | Direct and Scanline scenarios: exact IDs, two-component int32 copies, Boolean copies and compacted feature values. |
| `SimplnxCore::RemoveFlaggedFeatures: fill terminates with background zeros present` | kept | Watchdog guards termination; no negative IDs remain and only the two removed cells change. |
| `SimplnxCore::RemoveFlaggedFeatures: Test Extract then Remove Algorithm` | kept | Fill disabled: exact extracted IDs and removal/compaction in the original geometry. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Strict Feature Ids` | new-for-V&V | All three operations: Cell and Feature Attribute Matrix placement, shared paths, ignored-array validation and fill-disabled acceptance. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Shared Cell Paths and Selected Feature Matrix` | new-for-V&V | Both Cell paths: modifications use the geometry path, ignored values stay unchanged and only the selected Feature Attribute Matrix is compacted. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: SIMPL Backwards Compatibility` | kept | SIMPL 6.5 (UUID) and SIMPL 6.4 (Filter_Name) dynamic sections: converted values, paths and both ignored-array paths. |

## Exemplar archive

- **Archive:** `6_5_test_data_1_v2.tar.gz`
- **SHA512:** `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/6_5_test_data_1_v2.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/RemoveFlaggedFeaturesFilter.md` for the root cause, affected users and recommendation.

| Deviation | Observed difference |
|-----------|---------------------|
| `RemoveFlaggedFeaturesFilter-D3` | SIMPLNX stops when fill makes no progress and prevents FeatureIds from being ignored; DREAM.3D can loop forever. |
| `RemoveFlaggedFeaturesFilter-D4` | SIMPLNX rejects invalid IDs and tuple counts before removal; DREAM.3D can access memory out of bounds. |
| `RemoveFlaggedFeaturesFilter-D6` | SIMPLNX skips empty flagged features with a warning; DREAM.3D can produce a false one-cell crop. |
| `RemoveFlaggedFeaturesFilter-D7` | SIMPLNX uses configurable names with padded IDs and copies the Feature Attribute Matrix; DREAM.3D uses `Feature_<id>` and copies only cell data. |
