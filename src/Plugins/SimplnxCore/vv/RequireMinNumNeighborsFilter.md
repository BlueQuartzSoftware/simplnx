# V&V Report: RequireMinNumNeighborsFilter

| | |
|---|---|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| DREAM3D 6.5.171 equivalent | `MinNeighbors` (SIMPL UUID `dab5de3c-5f81-5bb5-8490-73521e1183ea`) - `Source/Plugins/Processing/ProcessingFilters/MinNeighbors.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | READY FOR REVIEW |
| Sign-off | *<engineer(s), date>* |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **Minor changes** - the NX port retains the legacy selection, coarsening, and feature-removal while incorporating bug fixes and minor code improvements. |
| Oracle (confirmed) | **Class 1 analytical + Class 4 invariants** - two 4x1x1 fixtures check cell and compacted feature arrays. |
| Code paths enumerated | 12 of 16 paths are assertion-covered; cancellation and defensive error paths are not covered. |
| Tests today | 9 active test cases: analytical all/single-phase execution, three execute errors, negative-FeatureId reassignment, two preflight errors, Small IN100 regression test, and SIMPL conversion. |
| Exemplar archive | No V&V output archive is needed because the oracle is inline; the retained `6_5_test_data_1_v2.tar.gz` input archive supports only the Small IN100 regression test. |
| Legacy comparison | **Run** - NX and DREAM3D 6.5.171 matched all five arrays for the oracle fixture. |
| Bug flags | None |
| V&V phase | Ready for review |

## Summary

`RequireMinNumNeighborsFilter` removes features below a neighbor-count threshold, fills their voxels from dominant valid face neighbors, and compacts feature-level arrays. Verification uses a hand-derived analytical fixture with invariants, focused error tests, a Small IN100 regression test, SIMPL conversion checks, and a DREAM3D 6.5.171 comparison. The analytical all-phase comparison is exactly matched; report status remains DRAFT for the outstanding gates listed below.

## Algorithm Relationship

*Classification:* Port | **Minor changes** | Rewrite | New filter

*Evidence:* The NX algorithm retains the legacy `MinNeighbors` selection, voxel reassignment, and inactive-feature-removal. It replaces direct neighbor indexing with shared utilities and incorporates fixes for ignored arrays, data-array updates, and invalid feature IDs. During this V&V cycle, negative FeatureIds skip initial marking and enter the reassignment pass, while out-of-range IDs return `-55567` before they are used for `activeObjects` indexing.

*PR(s):*

- PR #154 ("Added MinNeighbors filter") - introduced the filter as `MinNeighbors` in `ComplexCore`.
- PR #183 ("Added ability to specify initial value of DataStore") - updated data-store initialization APIs.
- PR #226 ("Add optional parameters that skip validation when inactive") - made optional-parameter validation conditional.
- PR #266 ("Refactored geometry hierarchy") - adapted the filter to the geometry-hierarchy refactor.
- PR #272 ("Filter Parameter Organization and Code Cleanup") - reorganized parameters and cleaned filter code.
- PR #275 ("Filter and Unit Test Fixes") - applied general filter and test corrections.
- PR #299 ("Geometry: Update classes to reuse functionality from higher level classes and rename methods") - updated geometry API usage.
- PR #346 ("DataPathSelectionUpdates") - migrated data-path selection parameters.
- PR #349 ("DOCS: Update paths and CMake codes to prepare for documentation updates") - updated documentation and build-path references.
- PR #351 ("Add Filter Comments") - added filter comment support.
- PR #378 ("MultiArraySelectionParameter Updates") - updated multi-array selection parameter handling.
- PR #437 ("BUG: Fix filter default tags") - corrected default filter tags.
- PR #671 ("API: Add C++ Class Name to All Default Tags") - added the C++ class name to default tags.
- PR #684 ("ENH: Improve the Tuple Count validation error reporting") - improved tuple-count validation diagnostics.
- PR #735 ("ENH: Create DataModifiedAction that marks DataObjects as being modified by a filter") - adopted modified-data action tracking.
- PR #779 ("ENH: Implement SIMPL pipeline conversion") - added SIMPL pipeline conversion.
- PR #801 ("ENH: Rename complex to simplnx") - moved the filter from `ComplexCore` to `SimplnxCore`.
- PR #874 ("ENH: Refactor the Parameter Keys to make them consistent and easy to learn") - renamed parameter keys.
- PR #926 ("BUG: Filters that delete NeighborLists from the DataStructure send strong warning messages") - added NeighborList-removal warnings.
- PR #931 ("ENH: All filter's class names end with \"Filter\".") - renamed `MinNeighbors` to `MinNeighborsFilter`.
- PR #934 ("BUG: Pipeline and Filter human facing label cleanup") - corrected the filter's user-facing label.
- PR #956 ("ENH: Rename Filters that start with Find/Generate/Calculate to Compute") - renamed `MinNeighborsFilter` to `RequireMinNumNeighborsFilter`.
- PR #980 ("ENH: Update docs for filters that change FeatureIds to warn user of invalid feature attribute matrix") - added invalid feature-attribute-matrix guidance.
- PR #1017 ("ENH/BUG: Data Array to Store, Speed Optimizations, Code Cleanup (SimplnxCore)") - migrated array access to data stores and cleaned code.
- PR #1082 ("SIMPLConversion header optimization") - optimized SIMPL conversion includes.
- PR #1088 ("Added versioning to filter parameters and json") - added parameter and JSON versioning.
- PR #1238 ("ENH: Added pipeline relative path support") - added relative-path pipeline support.
- PR #1249 ("COMP: Misc. compiler warning cleanups.") - resolved compiler warnings.
- PR #1278 ("BUG: Ensure FeatureId arrays are range checked against the Feature Attribute Matrix.") - added FeatureId range validation.
- PR #1310 ("BUG: Fix RequireMinNumNeighbors Not Using Ignore Paths") - corrected ignored voxel-array handling and introduced the separate algorithm class.
- PR #1320 ("BUG: RequireMinNumNeighbors DataArray Update fixes") - corrected cell-data updates during reassignment.
- PR #1343 ("BUG: Fix out-of-bounds array access in RequireMinNumNeighbors Filter") - added invalid FeatureId error handling.
- PR #1377 ("STY: Ensure all code arguments are consistent across filters") - standardized filter argument style.
- PR #1439 ("ENH/API: Multi-Dimensional Tuple Support for StringArray and NeighborList") - adapted to multidimensional NeighborList APIs.
- PR #1523 ("ENH: Factor out the 6-face neighbor code that is systemic through out the code base") - extracted shared six-face neighbor utilities.
- PR #1535 ("ENH: Remove redundant preflight checks that are already done in the parameter") - removed duplicated preflight checks.
- PR #1590 ("ENH: Standardize 2D Image Handling") - standardized shared image-neighbor behavior.
- PR #1682 ("BUG: Fix SIMPL pipeline conversion bugs and Write Image naming") - corrected optional SIMPL conversion argument handling.

## Oracle

*Class:* **1 (Analytical)** primary + **4 (Invariant)** companion.

*Applied:* A hand-built 4x1x1 fixture has FeatureIds `[2, 1, 1, 3]`. Feature 1 is below the threshold and its two rejected voxels have unique valid left and right face neighbors, so coarsening must copy feature 2 then feature 3. After inactive-feature removal remaps IDs, the exact FeatureIds are `[1, 1, 2, 2]`; a copied cell array is `[20, 20, 30, 30]`, while an ignored array remains `[200, 101, 102, 300]`. The compacted all-phase feature arrays are NumNeighbors `[0, 3, 3]` and Phases `[0, 2, 1]`; in single-phase mode NumNeighbors is `[0, 0, 3]` because nonselected feature 2 remains active. The fixture runs in both modes.

*Encoded:* `test/RequireMinNumNeighborsTest.cpp::SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` - two fixtures, both passing in the available in-core build. Class 4 asserts nonnegative, in-range final FeatureIds; Class 1 asserts exact compacted NumNeighbors and Phases values.

*Second-engineer review:* pending second-engineer review.

## Code path coverage

12 of 16 paths are assertion-covered. Cancellation and defensive infrastructure/error paths are documented gaps.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RequireMinNumNeighbors.cpp` (296 lines), plus preflight validation in `Filters/RequireMinNumNeighborsFilter.cpp`.

The filter selects features to retain, marks rejected-feature voxels, iteratively copies a dominant face neighbor into each rejected voxel, and removes inactive feature tuples.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | Preflight | Feature-level `NumNeighbors` and, when enabled, `FeaturePhases` tuple counts disagree - return `-252`. | `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)` |
| 2 | Preflight | FeatureIds tuple count differs from the selected Image Geometry cell count - return `-55571`. | `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds tuple count mismatch (-55571)` |
| 3 | Preflight | NeighborList-removal preflight warning is produced when feature NeighborLists are present. | `SimplnxCore::RequireMinNumNeighborsFilter` - asserts only one `-5558` warning. |
| 4 | Setup | Cell-data array discovery fails - return `-5556`. | *Not directly tested. Defensive check.* |
| 5 | Selection | Single-phase mode names a phase absent from `FeaturePhases` - return `-5555`. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)` |
| 6 | Selection | Every eligible feature is below the neighbor threshold - return `-55569` before mutation. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)` |
| 7 | Selection | All-phase or selected-phase feature meets threshold; nonselected phases remain active. | `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` - `ApplyToSinglePhase=0` and `ApplyToSinglePhase=1` |
| 8 | Cancellation | Cancellation before mutation or within coarsening returns without further changes. | *Not directly tested. Requires cancel signal injection.* |
| 9 | Marking | Rejected feature IDs are marked and retained feature IDs remain available for coarsening. | `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` |
| 10 | Reassignment | Rejected voxel chooses the most frequent valid face-neighbor feature, including boundary-face rejection. | `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` |
| 11 | Reassignment | Feature ID is outside the feature-array range - return `-55567` before it is used for indexing. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)` |
| 12 | Reassignment | Negative Feature IDs skip initial marking and are reassigned from valid face neighbors. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned` |
| 13 | Copy | A rejected voxel copies every nonignored cell-data tuple from its selected neighbor. | `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` |
| 14 | Copy | A cell array with an out-of-range source or destination tuple returns `-55568`. | *Not directly tested. AttributeMatrix construction rejects mismatched tuple shapes before execution. Defensive check.* |
| 15 | Finalize | Inactive feature tuples are removed and remaining FeatureIds are remapped; NeighborLists are removed. | `SimplnxCore::RequireMinNumNeighborsFilter` and `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` |
| 16 | Finalize | Inactive-feature removal fails - return `-55570`. | *Not directly tested. Defensive check.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` | new-for-V&V | Builds the Class 1+4 fixture and checks exact FeatureIds, copied and ignored arrays, ID invariants, and feature-array compaction. |
| `SimplnxCore::RequireMinNumNeighborsFilter` | kept | Loads `6_5_test_data_1_v2.tar.gz`, computes neighbors, runs the filter, and checks feature tuples plus every `NumElements` value. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)` | kept | Asserts invalid preflight and `-252` code for mismatched feature-level arrays. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds tuple count mismatch (-55571)` | new-for-V&V | Asserts invalid preflight and exact `-55571` code. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)` | new-for-V&V | Asserts error for rejected out of range IDs. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned` | new-for-V&V | Asserts a negative FeatureId is accepted and reassigned from its valid face neighbor. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)` | new-for-V&V | Asserts exact unavailable-phase error. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)` | new-for-V&V | Asserts exact all-rejected error. |
| `SimplnxCore::RequireMinNumNeighborsFilter: SIMPL Backwards Compatibility` | kept | SIMPL json backwards compatibility check. |

## Exemplar archive

No new exemplar archive was created for this V&V cycle: the Class 1 oracle is encoded entirely as inline expected values in the test source. 

## Deviations from DREAM3D 6.5.171

No deviations observed. The all-phase analytical fixture compared FeatureIds, copied and ignored cell arrays, NumNeighbors, and Phases; DREAM3D 6.5.171 and NX matched all five arrays.
