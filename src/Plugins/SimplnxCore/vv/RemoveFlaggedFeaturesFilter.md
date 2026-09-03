# V&V Report: RemoveFlaggedFeaturesFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID | `6e8cc6ec-8b9b-402e-9deb-85bd1cdba743` |
| DREAM3D 6.5.171 equivalent | Two legacy filters. *Remove* maps to `RemoveFlaggedFeatures` (SIMPL UUID `a8463056-3fa7-530b-847f-7f4cb78b8602`, `Source/Plugins/Processing/ProcessingFilters/RemoveFlaggedFeatures.{h,cpp}`). *Extract* maps to `ExtractFlaggedFeatures` (SIMPL UUID `e0555de5-bdc6-5bea-ba2f-aacfbec0a022`, `Source/Plugins/Sampling/SamplingFilters/ExtractFlaggedFeatures.{h,cpp}`). |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | Michael A. Jackson <mike.jackson@bluequartz.net>, 2026-09-02 (author). Second engineer: pending second-engineer review. |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Minor changes** - SIMPLNX merges legacy `RemoveFlaggedFeatures` (Processing) and `ExtractFlaggedFeatures` (Sampling) into one filter with an operation selector. The removal and fill loop is a port that now lives in the shared `FeatureRemovalUtilities`; the extract path is rebuilt on the `ComputeFeatureRect` and `CropImageGeometry` sub-filters. |
| Oracle (confirmed) | **Class 1 analytical + Class 4 invariants** - three hand-derived fixtures (5x2x1, 4x4x1, 3x3x3) pin every filled cell to the exact neighbor it was copied from, plus a Small IN100 invariant test that fills 8,535 cells. Encoded in `test/RemoveFlaggedFeaturesTest.cpp`; all 18 test cases pass in `simplnx-Rel` and `simplnx-ooc-Rel`. |
| Code paths enumerated | 23 of 31 paths are assertion-covered. The 8 uncovered paths are cancellation and defensive sub-filter failure paths. |
| Tests today | 18 test cases: 8 Class 1 analytical (remove, fill, extract, extract-then-remove), 4 execute-error, 1 preflight-error, 1 preflight-warning, 1 fill-ignore-list warning, 1 empty-feature warning, 1 Class 4 Small IN100 invariant, 1 SIMPL 6.4/6.5 conversion. The 3 original tests are retired. **OOC caveat:** the `simplnx-ooc-Rel` preset registers no OOC backend, so that pass certifies compile and run in that configuration, not out-of-core data-path behavior. |
| Exemplar archive | No output archive. The inline oracle needs none. The Class 4 test reads the shared input archive `6_5_test_data_1_v2.tar.gz` (SHA512 `585b51ba...3027d6c`, sidecar exists). |
| Legacy comparison | **Run (2026-09-02)** - DREAM3D 6.5.171 and SIMPLNX were run on the 5x2x1, 4x4x1 and 3x3x3 fixtures with fill on and off, and on the 4x4x1 extract. Every removal array (25 array comparisons) matched the oracle and each other exactly. The extract geometry, origin and cell arrays matched. Seven deviations are documented. Six are bugs, all present in SIMPLNX before this branch and three (D3, D4, D6) shared with 6.5.171. D7 is an algorithmic choice. |
| Bug flags | `RemoveFlaggedFeaturesFilter-D1` through `-D6`. D1 and D2 are hangs that only SIMPLNX had; D3 and D4 are shared with 6.5.171; D5 is SIMPLNX only; D6 affects both differently. All are fixed in SIMPLNX on this branch (D1 was fixed in v7.4.2). |
| V&V phase | Discovery, oracle, reconciliation, algorithm review, tests, dual-build pass, legacy comparison, deviations and documentation are complete. Five scoped review passes (adversarial, senior engineer, CPU, memory, out-of-core) were run on the change set and their findings applied; see *Review findings*. Outstanding: second-engineer review of the oracle and this report at PR review. |

## Summary

`RemoveFlaggedFeaturesFilter` removes or extracts the **Features** marked true in a boolean feature array, optionally filling vacated **Cells** by iterative dilation from face neighbors, then compacts the feature Attribute Matrix. Verification uses three Class 1 analytical fixtures whose per-cell copy sources are hand-derived, a Class 4 Small IN100 invariant test, error and preflight tests, and SIMPL conversion checks. The removal and fill outputs are bit-identical to DREAM3D 6.5.171 on every fixture; the V&V found and fixed two SIMPLNX-only infinite loops (background cells, and distinct first-sighting neighbors as reported in issue #1698), added guards for failure modes shared with 6.5.171 (no fill progress, including the Feature Ids array listed as ignored; out-of-range FeatureId; tuple count that does not match the geometry), and fixed the extract path's swallowed sub-filter errors, its crash on a flagged feature with no cells, and the copy of its temporary bounds array into every extracted geometry.

## Algorithm Relationship

*Classification:* Port | **Minor changes** | Rewrite | New filter

*Evidence:* The SIMPLNX filter was introduced in PR #474 as a merge of the two legacy filters under one operation selector (*Remove*, *Extract*, *Extract then Remove*). The `Remove` branch follows legacy `RemoveFlaggedFeatures::remove_flaggedfeatures()` and `assign_badpoints()` step for step: mark, iteratively dilate, compact. PR #1700 moved that code into `FeatureRemovalUtilities::removeFlaggedFeatures()` so `KeepRemoveRankedFeaturesFilter` can share it. The `Extract` branch replaces legacy `ExtractFlaggedFeatures::find_feature_bounds()` plus its inline `CropImageGeometry` call with the `ComputeFeatureRectFilter` and `CropImageGeometryFilter` sub-filters.

*Port-time deltas (removal path):*

1. Face-neighbor bookkeeping uses the shared `NeighborUtilities` helpers instead of six inline boundary tests. The traversal order (-Z, -Y, -X, +X, +Y, +Z) and the boundary rules are the same, so the output is unchanged. Confirmed by the bit-identical A/B.
2. The neighbor tally is a per-cell `discoveredFeatures`/`numHits` pair instead of the legacy feature-indexed `n[]` vector. Before PR #1700 the SIMPLNX tally did not count the first sighting, which changed output by never filling some cells (D1). With the first sighting counted, the tally is equivalent to legacy.
3. The vacated-cell test was `featureName > 0` instead of the legacy `featurename < 0`, which made background cells (FeatureId 0) permanent loop targets (D2). This branch restores `>= 0 -> skip`, matching legacy.
4. Guards with no legacy counterpart were added on this branch: FeatureId range and tuple-count validation before any modification (D4), a no-progress check that counts the cells each fill pass actually fills (D3), and the Feature Ids array is always copied even when listed in the ignore list (D3). All turn undefined behavior or an infinite loop into a deterministic error or warning and do not change valid output.
5. The per-cell neighbor buffer is allocated only when fill is enabled (PR #1700). No output effect.
6. Progress messages are throttled (PR #1340). No output effect.

*Port-time deltas (extract path):*

7. Bounds come from `ComputeFeatureRectFilter` (min initialized to `UINT32_MAX`, max to 0) instead of the legacy `-1` sentinel. For a flagged feature with no cells the legacy filter crops a bogus 1x1x1 geometry at origin (-1, -1, -1) (D6); SIMPLNX now warns once, listing the skipped ids, and creates nothing for them. The temporary bounds array is copied into a local buffer and deleted before the first crop, so it is neither carried into the extracted geometries nor left behind on an error return.
8. The extracted geometry is named `<prefix>-<zero-padded id>` and carries the feature Attribute Matrix. Legacy names it `Feature_<id>` and copies only cell data (D7).
9. Sub-filter preflight and execute results were tested against the wrong variable and failures threw `std::runtime_error` (D5). This branch returns error results `-53901` through `-53904`.

*Material PRs since the filter was introduced:* #474 (introduced), #926 (NeighborList removal warning), #1017 (DataStore), #1278 (FeatureId range checks elsewhere), #1523 (shared face-neighbor helpers), #1590 (2D image handling), #1700 (shared `FeatureRemovalUtilities`, first-sighting fix). See the deviation entries for which of these changed output.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion.

Expected values are derived by hand from the fixture definitions and the documented face-neighbor rule, without using output from SIMPLNX or DREAM3D 6.5.171.

*Applied:* Every fixture carries three cell arrays: `FeatureIds`, `CellValue = 100 + cell index`, and `IgnoredValue = 500 + cell index`, plus a feature array `Int32DataSet = 1000 + feature id`. Because `CellValue` is unique per cell, the value found in a vacated cell after the fill identifies exactly which neighbor it was copied from, and copies that chain across iterations are visible. The derivations are written as comments beside each `REQUIRE` in the test.

| Fixture | Geometry | What it pins |
|---|---|---|
| A (issue #1698) | 5x2x1, remove features 2 and 3 | A vacated cell whose valid neighbors are all distinct features (first sighting must count), a cell with one valid neighbor, a cell with no valid neighbor in pass 1 that is filled in pass 2, a 1-vs-1 tie resolved to the first-seen feature, and a two-step copy chain (cell 4 receives cell 2's value through cell 3). Expected `FeatureIds = [1,1,1,1,1, 1,2,2,2,2]`. |
| B (background) | 4x4x1 with five FeatureId 0 cells, remove feature 3 | Background cells are never fill targets and are legal fill sources: cell 14 is filled from background cell 15 and ends at 0. Expected `FeatureIds = [0,1,1,1, 1,0,2,2, 2,2,0,1, 2,2,0,0]`. Also the extract fixture: feature 3 crops to a 2x1x1 geometry at origin (1, 3, 0). |
| E (3D) | 3x3x3, remove features 3 and 4 | All six face directions, a 4-vs-2 majority where the losing feature is seen first (cell 13 copies from +Z cell 22), and corner cell 0 enclosed by vacated cells so that it is filled only in pass 2 from cell 9, which itself was filled from cell 18. |
| Small IN100 | 100x100x100, remove 275 features with fewer than 100 cells, fill on | Class 4: every cell ends in a surviving feature, untouched cells follow the compaction renumbering exactly, feature arrays are copied not recomputed, each cell's phase equals its new feature's phase, every one of the 8,535 filled cells shares its final id with a face neighbor, and survivors grow by exactly the number of filled cells. NeighborLists are removed. |

*Encoded:* `src/Plugins/SimplnxCore/test/RemoveFlaggedFeaturesTest.cpp` - `Class 1 Oracle - *` (8 test cases) and `Class 4 Invariants - Small IN100 remove small features with fill`. All pass at the verified commit in the `simplnx-Rel` and `simplnx-ooc-Rel` builds.

*Second-engineer review:* pending second-engineer review (the PR review constitutes the review).

## Bugs found and fixed

*This branch fixes every defect in this table except D1, which PR #1700 fixed in DREAM3D-NX v7.4.2 before this V&V; this branch adds its regression test. The remaining fixes will be in the DREAM3D-NX release after v7.4.2.*

| Deviation | Defect | Affected released versions | Resolution in this branch |
|-----------|--------|----------------------------|---------------------------|
| `RemoveFlaggedFeaturesFilter-D1` | With *Fill-in Removed Features* on, the neighbor tally only recorded a fill source on the second sighting of a feature, so a vacated cell whose valid neighbors were all distinct features, or which had a single valid neighbor, was never filled and the dilation loop never terminated (GitHub issue #1698). | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D 6.5.171 was not affected. | Fixed in v7.4.2 by PR #1700 (first sighting counts as a hit). This branch adds the `Fill from distinct first-sighting neighbors (issue #1698)` regression test that reproduces the issue's 5x2x1 case. |
| `RemoveFlaggedFeaturesFilter-D2` | With fill on, any cell with FeatureId 0 was treated as an unresolved cell but never filled, so the dilation loop never terminated on any input that contained background cells. | DREAM3D-NX v7.0.0 through v7.4.2. DREAM.3D 6.5.171 was not affected. | The vacated-cell test is `featureName >= 0 -> skip`, matching 6.5.171. Pinned by `Fill treats FeatureId 0 as a source and never a target`, which hung before the fix. |
| `RemoveFlaggedFeaturesFilter-D3` | With fill on, when every cell belonged to a flagged feature and the only unflagged features owned no cells, no vacated cell had a fill source and the loop spun forever. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.2. | A pass that leaves vacated cells but fills none of them returns error `-45436`. The Feature Ids array is always copied even when listed in *Attribute Arrays to Ignore* (warning `-45438`), which closed a second route to the same hang. Pinned by `Execute Error - no fill progress (-45436)` and `Fill cannot ignore the Feature Ids array (-45438 warning)`. |
| `RemoveFlaggedFeaturesFilter-D4` | A FeatureId that was negative or not less than the feature tuple count indexed the flag vector out of bounds during marking. | DREAM.3D 6.5.171; DREAM3D-NX v7.0.0 through v7.4.2. | Every FeatureId is validated before any modification; a bad value returns error `-45435` naming the cell, value and valid range, and a tuple count that differs from the geometry cell count returns `-45437`. Pinned by `Execute Error - FeatureId out of range (-45435)` and `Execute Error - Feature Ids tuple count differs from the geometry (-45437)`. |
| `RemoveFlaggedFeaturesFilter-D5` | The extract path tested the preflight result where it should have tested the execute result of its sub-filters, so an execute failure was ignored, and a preflight failure threw `std::runtime_error` instead of returning an error. | DREAM3D-NX v7.0.0 through v7.4.2. Not applicable to 6.5.171. | Sub-filter preflight and execute results are checked and returned as errors `-53901` through `-53904` with the sub-filter's message. Covered by inspection; see Code path coverage rows 6, 7, 11, 12. |
| `RemoveFlaggedFeaturesFilter-D6` | A flagged feature that owns no cell has an empty bounding box. SIMPLNX passed it to the crop, which failed preflight and threw. DREAM.3D 6.5.171 crops a 1x1x1 geometry at origin (-1, -1, -1) containing cell 0. | DREAM.3D 6.5.171 (bogus geometry); DREAM3D-NX v7.0.0 through v7.4.2 (uncaught exception). | The features are skipped and one warning `-53905` lists them; no geometry is created for them. Pinned by `Extract - flagged feature with no cells is skipped with a warning`. |

A preflight cleanup that is not a deviation: the filter emitted its own NeighborList warning (`-11505`) and the shared helper emitted a second one (`-5558`) for the same arrays. The duplicate was removed; the test asserts exactly one warning.

## Review findings

Five review passes were run on the change set only (adversarial, nit-picky senior engineer, CPU performance, memory, out-of-core). Findings that changed the code:

- Adversarial: listing the Feature Ids array in *Attribute Arrays to Ignore* with fill on still hung, because the guard tested whether a source was chosen, not whether a cell was filled. The guard now counts filled cells and the Feature Ids array is never ignored (warning `-45438`). The Feature Ids tuple count is now checked against the geometry (`-45437`).
- Memory: every extracted geometry received a copy of the internal `tempBounds` array, and the array leaked on every error return. The bounds are copied to a local buffer and the array is deleted before the first crop. The dead `CreateArrayAction` in preflight (sized to the cell count) was removed.
- CPU: the per-vacated-cell `std::vector` allocations in the neighbor tally were replaced by fixed `std::array`s; the cell-array list is built once instead of once per pass; the validation pass polls the cancel flag. The `>= 0` fix also removes a per-pass allocation storm on background cells.
- Senior engineer: `-45436` reports the number of stuck cells; the empty-feature warning is aggregated; sub-filter error messages carry the bounds and a count of further errors; Doxygen completed on the new functions; inputs of the scan are `const`.
- Out-of-core: no access pattern of an existing loop changed; the validation pass is one extra chunk-sequential read. The `simplnx-ooc-Rel` caveat above was added.

## Code path coverage

23 of 31 paths are assertion-covered. The uncovered paths are cancellation and defensive failure paths of the sub-filters and the compaction helper.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RemoveFlaggedFeatures.cpp` and `src/Plugins/SimplnxCore/src/SimplnxCore/utils/FeatureRemovalUtilities.cpp`, plus preflight in `Filters/RemoveFlaggedFeaturesFilter.cpp`.

Phases: (a) preflight, (b) extract (bounding boxes, then one crop per flagged feature), (c) removal marking, (d) iterative fill, (e) compaction.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | (a) Preflight | Parent of the flag array is not an Attribute Matrix -> error `-9892` | `Preflight Error - flag array parent is not an Attribute Matrix (-9892)` |
| 2 | (b) Extract | Temporary `tempBounds` array is created by the `ComputeFeatureRect` sub-filter and deleted by the algorithm before the first crop, so no extracted geometry carries it | `Class 1 Oracle - Extract crops the bounding box of each flagged feature` (asserts `tempBounds` is absent from the source and from the extracted geometry) |
| 3 | (a) Preflight | *Remove* or *Extract then Remove* -> cell and feature groups marked modified, NeighborLists scheduled for deletion, one warning `-5558` | `Preflight Warning - NeighborLists are removed (-5558)` (`Functionality = 0`, `Functionality = 2`) |
| 4 | (a) Preflight | *Extract* -> no NeighborList warning, NeighborLists kept | `Preflight Warning - NeighborLists are removed (-5558)` (`Extract alone does not remove NeighborLists`) |
| 5 | (b) Extract | Flag array is not Bool or UInt8 -> error `-53900` | *Not directly tested. The parameter type check in preflight prevents it; defensive.* |
| 6 | (b) Extract | `ComputeFeatureRect` preflight fails -> error `-53901` | *Not directly tested. Requires an internal inconsistency the filter's own preflight already rejects; defensive.* |
| 7 | (b) Extract | `ComputeFeatureRect` execute fails -> error `-53902`, `tempBounds` removed | *Not directly tested. The sub-filter fails on a FeatureId beyond the feature tuple count. Extract runs before removal, so in *Extract* and *Extract then Remove* this path is reached before path 14; defensive.* |
| 8 | (b) Extract | One or more flagged features own no cell -> one warning `-53905` listing them, no geometry for them | `Extract - flagged feature with no cells is skipped with a warning` (two empty features, one warning) |
| 9 | (b) Extract | Flagged feature -> new geometry `<prefix>-<zero-padded id>` with the bounding-box dimensions, origin, all cell arrays and the feature Attribute Matrix | `Class 1 Oracle - Extract crops the bounding box of each flagged feature`, `Class 1 Oracle - Extract zero-pads names and handles several features`, `Class 1 Oracle - Extract then Remove with fill` |
| 10 | (b) Extract | Unflagged feature -> no geometry | `Class 1 Oracle - Extract zero-pads names and handles several features` |
| 11 | (b) Extract | Crop preflight fails -> error `-53903` | *Not directly tested. Reachable before this branch through path 8; now unreachable for valid bounds; defensive.* |
| 12 | (b) Extract | Crop execute fails -> error `-53904` | *Not directly tested. Defensive.* |
| 13 | (b) Extract | Cancel between crops -> return | *Not directly tested. Requires cancel-signal injection.* |
| 14 | (c) Marking | A FeatureId is negative or >= feature tuple count -> error `-45435`, nothing modified | `Execute Error - FeatureId out of range (-45435)` (both sections) |
| 14a | (c) Marking | Feature Ids tuple count differs from the geometry cell count -> error `-45437`, nothing modified | `Execute Error - Feature Ids tuple count differs from the geometry (-45437)` |
| 14b | (c) Marking | Fewer than two feature tuples -> error `-45433`, nothing modified | *Not directly tested. A one-tuple feature Attribute Matrix cannot hold a flagged feature; defensive.* |
| 15 | (c) Marking | Every feature flagged -> error `-45433`, nothing modified | `Execute Error - all features flagged (-45433)` (`Fill = false`, `Fill = true`) |
| 16 | (c) Marking | Fill off: flagged cell -> 0 | `Class 1 Oracle - Remove without fill` |
| 17 | (c) Marking | Fill on: flagged cell -> -1 | Every `Class 1 Oracle - Fill *` test |
| 18 | (d) Fill | Vacated cell with a single valid neighbor, or all-distinct neighbors: first sighting records the source | `Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)` (cells 3 and 9) |
| 19 | (d) Fill | Majority vote: the source moves only when a feature's tally strictly exceeds the previous maximum | `Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` (cell 13, 4 to 2) |
| 20 | (d) Fill | 1-vs-1 tie -> first-seen feature in -Z, -Y, -X, +X, +Y, +Z order | `Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)` (cells 3 and 4) |
| 21 | (d) Fill | Vacated cell with no non-negative neighbor this pass -> filled in a later pass from a neighbor that was itself filled | `(issue #1698)` (cell 4 via cell 3), `(3x3x3)` (cell 0 via cell 9 via cell 18) |
| 22 | (d) Fill | Background cell (FeatureId 0) is skipped as a target and counted as a source | `Class 1 Oracle - Fill treats FeatureId 0 as a source and never a target` (cell 14 -> 0) |
| 23 | (d) Fill | Vacated cells remain and the pass filled none -> error `-45436`, cells left at -1 | `Execute Error - no fill progress (-45436)` |
| 23a | (d) Fill | Feature Ids array listed in the ignore list -> removed from the list, warning `-45438`, fill proceeds | `Fill cannot ignore the Feature Ids array (-45438 warning)` |
| 24 | (d) Fill | Every non-ignored cell array is copied from the source cell; ignored arrays are untouched | `Class 1 Oracle - Fill copies every non-ignored Cell array` and the `IgnoredValue` checks in every fill test |
| 25 | (d) Fill | Face neighbors outside the volume are excluded | 5x2x1 and 4x4x1 fixtures (Y and Z faces), 3x3x3 fixture (all six faces) |
| 26 | (d) Fill | Cancel during a pass -> return | *Not directly tested. Requires cancel-signal injection.* |
| 27 | (e) Compaction | Surviving features renumbered contiguously from 1 in input order; feature arrays compacted in place; NeighborLists deleted | Every remove test (`CheckFeatureArraysCompacted`), `Class 4 Invariants - Small IN100 remove small features with fill` |
| 28 | (e) Compaction | `RemoveInactiveObjects` fails -> error `-45434` | *Not directly tested. Requires a feature group whose arrays do not match the flag count, which the preflight tuple check rejects; defensive.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Remove without fill` | new-for-V&V | Two sections (fixtures A and B). Asserts exact `FeatureIds`, unchanged `CellValue` and `IgnoredValue`, and the compacted `Int32DataSet` and flag arrays. 36 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill from distinct first-sighting neighbors (issue #1698)` | new-for-V&V | The issue's 5x2x1 case. Asserts exact `FeatureIds`, the copy source of each of the three filled cells via `CellValue`, the untouched ignored array and the compaction. Hung before v7.4.2. 19 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill copies every non-ignored Cell array` | new-for-V&V | Fixture A with an empty ignore list; asserts `IgnoredValue` follows the same copies as `CellValue`. 15 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill treats FeatureId 0 as a source and never a target` | new-for-V&V | Fixture B with fill; asserts the five background cells stay 0, cell 13 copies from cell 12 and cell 14 copies from background cell 15. Hung before this branch (D2). 19 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Fill majority vote and second iteration (3x3x3)` | new-for-V&V | Fixture E; asserts all 27 `FeatureIds`, the five copy sources including the two-pass chain into cell 0, ignored array and compaction. 19 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - all features flagged (-45433)` | new-for-V&V | `DYNAMIC_SECTION` for fill off and on; asserts the exact code and that no array was modified. 21 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - FeatureId out of range (-45435)` | new-for-V&V | Sections for a value equal to the tuple count and for a negative value; asserts the code and that `FeatureIds` is unmodified. Covers D4. 15 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - Feature Ids tuple count differs from the geometry (-45437)` | new-for-V&V | A 4-tuple Feature Ids array outside the 10-cell geometry; asserts the code and that the array is unmodified. Covers D4. 9 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Execute Error - no fill progress (-45436)` | new-for-V&V | 4x1x1 where the only unflagged feature owns no cell; asserts the code and that all cells are left at -1. Covers D3. 8 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Fill cannot ignore the Feature Ids array (-45438 warning)` | new-for-V&V | Fixture A with the Feature Ids array in the ignore list; asserts exactly one warning `-45438` and the same output as the issue #1698 test. Hung before this branch. Covers D3. 18 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract crops the bounding box of each flagged feature` | new-for-V&V | Fixture B extract; asserts the original is untouched, the new geometry's dimensions (2,1,1), origin (1,3,0), spacing, all three cell arrays, the carried feature Attribute Matrix, and that `tempBounds` is absent from both the source and the extracted geometry. 35 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract zero-pads names and handles several features` | new-for-V&V | 12-feature 1D fixture; asserts two-digit padding (`-03`, `-11`), per-feature dimensions, origin and cell values, and that unflagged features produce no geometry. 37 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Extract - flagged feature with no cells is skipped with a warning` | new-for-V&V | Two flagged features own no cell; asserts exactly one warning `-53905` that names both, that feature 3 is still extracted and that features 4 and 5 produce no geometry. Covers D6. 27 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 1 Oracle - Extract then Remove with fill` | new-for-V&V | Fixture B; asserts the extracted geometry (from the unmodified input) and the filled original in one run. 30 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Error - flag array parent is not an Attribute Matrix (-9892)` | new-for-V&V | Flag array placed in a plain `DataGroup`; asserts invalid preflight and the code. 5 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Preflight Warning - NeighborLists are removed (-5558)` | new-for-V&V | `DYNAMIC_SECTION` for *Remove* and *Extract then Remove* asserts exactly one warning with code `-5558` and that the NeighborList is gone after execution; a third section asserts *Extract* emits no warning and keeps it. 23 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: Class 4 Invariants - Small IN100 remove small features with fill` | new-for-V&V | Loads `6_5_test_data_1_v2`, flags the 275 features with fewer than 100 cells, fills, and checks the six invariants listed in the Oracle section over 1,000,000 cells with aggregated counts. 751 assertions. |
| `SimplnxCore::RemoveFlaggedFeaturesFilter: SIMPL Backwards Compatibility` | kept | Two `DYNAMIC_SECTION`s (`SIMPL 6.5 (UUID)`, `SIMPL 6.4 (Filter_Name)`); modified this cycle to also assert the two converted ignored-array paths. 23 assertions. |
| `SimplnxCore::RemoveFlaggedFeatures: Test Remove Algorithm` | retired | Ran *Remove* with fill off on the 4x4x1 fixture and checked `FeatureIds` and the compacted feature array. Replaced by `Class 1 Oracle - Remove without fill` (fixture B), which adds the cell-array and ignored-array checks. The fill path was never exercised. |
| `SimplnxCore::RemoveFlaggedFeatures: Test Extract Algorithm` | retired | Checked only two `FeatureIds` values in the extracted geometry. Replaced by `Class 1 Oracle - Extract crops the bounding box of each flagged feature`, which also checks dimensions, origin, spacing, every cell array and the feature Attribute Matrix. |
| `SimplnxCore::RemoveFlaggedFeatures: Test Extract then Remove Algorithm` | retired | Fill off; same two-value extract check. Replaced by `Class 1 Oracle - Extract then Remove with fill`. |

There are 18 active `TEST_CASE`s: 1 kept (modified) and 17 new for this V&V cycle; 3 retired. `KeepRemoveRankedFeaturesTest.cpp` gained one section (`Fill terminates when the input has background cells`) because that filter shares the fill utility and inherited D2.

## Exemplar archive

No output archive was created. Both Class 1 fixtures and their expected values are encoded inline.

The Class 4 test uses this shared input archive:

| Field | Value |
|---|---|
| Archive | `6_5_test_data_1_v2.tar.gz` |
| SHA512 | `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c` |
| CMake registration | `src/Plugins/SimplnxCore/test/CMakeLists.txt` |
| Provenance | `src/Plugins/SimplnxCore/vv/provenance/6_5_test_data_1_v2.md` |
| Role in this V&V | Input only. The test derives the flag array, the expected compaction and every invariant from the pre-filter arrays. No archived array is compared against filter output. |

## Deviations from DREAM3D 6.5.171

On 2026-09-02, DREAM3D 6.5.171 and SIMPLNX were run on legacy-native copies of fixtures A, B and E with fill on and off (five removal cases) and on fixture B with *Extract*. The inputs were authored with the shared h5py legacy writer so both runners read identical files. All 25 removal array comparisons (`FeatureIds`, `CellValue`, `IgnoredValue`, `Int32DataSet`, `Active`) matched the analytical oracle and each other exactly. The extracted geometry's dimensions, origin and three cell arrays matched. Three further fixtures were run to characterize the error-path deviations: the no-progress twin (D3), an out-of-range FeatureId (D4) and a flagged feature with no cells (D6). The pipelines, inputs, outputs, logs and comparison script are archived outside the repository in the V&V OneDrive library, folder `RemoveFlaggedFeaturesFilter` (59 files, uploaded 2026-09-03), per the archival policy.

See `vv/deviations/RemoveFlaggedFeaturesFilter.md` for the root cause, affected users and recommendation for each deviation.

| Deviation | Observed difference |
|-----------|---------------------|
| `RemoveFlaggedFeaturesFilter-D1` | SIMPLNX v7.0.0 through v7.4.1 never returned when a vacated cell's valid neighbors were all distinct features or a single neighbor; 6.5.171 completed. Fixed in v7.4.2. |
| `RemoveFlaggedFeaturesFilter-D2` | SIMPLNX through v7.4.2 never returned when the input contained FeatureId 0 cells and fill was on; 6.5.171 completed. Fixed on this branch. |
| `RemoveFlaggedFeaturesFilter-D3` | SIMPLNX returns `-45436` when a fill pass fills no cell, and always copies the Feature Ids array even when it is listed as ignored (warning `-45438`); 6.5.171 loops forever in both situations. |
| `RemoveFlaggedFeaturesFilter-D4` | SIMPLNX returns `-45435` for a FeatureId outside `[0, tuple count)` and `-45437` for a tuple count that differs from the geometry; 6.5.171 reads out of bounds and silently zeroed the cell in the A/B run. |
| `RemoveFlaggedFeaturesFilter-D5` | SIMPLNX returns `-53901` through `-53904` when an extract sub-filter fails; earlier SIMPLNX ignored execute failures and threw on preflight failures. Not applicable to 6.5.171. |
| `RemoveFlaggedFeaturesFilter-D6` | For a flagged feature with no cells SIMPLNX warns `-53905` and creates nothing; 6.5.171 creates a 1x1x1 geometry at origin (-1, -1, -1) holding cell 0. |
| `RemoveFlaggedFeaturesFilter-D7` | Extracted geometries are named `<prefix>-<zero-padded id>` and carry the feature Attribute Matrix; 6.5.171 names them `Feature_<id>` and copies only cell data. |

Behavior that is identical in both implementations and is recorded here because it surprises users: background cells (FeatureId 0) are legal fill sources. A vacated cell whose non-negative neighbors are all background becomes background. This is the documented isotropic-coarsening rule shared with `RequireMinimumSizeFeatures` and `RequireMinNumNeighbors`.
