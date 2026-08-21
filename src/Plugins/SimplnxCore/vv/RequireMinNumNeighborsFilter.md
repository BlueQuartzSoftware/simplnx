# V&V Report: RequireMinNumNeighborsFilter

| | |
|---|---|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `4ab5153f-6014-4e6d-bbd6-194068620389` |
| DREAM3D 6.5.171 equivalent | `MinNeighbors` (SIMPL UUID `dab5de3c-5f81-5bb5-8490-73521e1183ea`) - `Source/Plugins/Processing/ProcessingFilters/MinNeighbors.{h,cpp}` |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Jared Duffey, 07-31-2026. Second engineer: Michael A. Jackson <mike.jackson@bluequartz.net>, 2026-08-07. |

## At a glance

| Aspect | Current state |
|---|---|
| Algorithm Relationship | **Minor changes** - the NX port retains the legacy selection, coarsening, and feature-removal while incorporating bug fixes and minor code improvements. |
| Oracle (confirmed) | **Class 1 analytical + Class 4 invariants** - two mode-specific 4x1x1 inputs and one common 6x6x6 input check exact cell copying, phase selection, iterative coarsening, and feature compaction. |
| Code paths enumerated | 13 of 18 paths are assertion-covered; cancellation and defensive error paths are not covered. |
| Tests today | 11 active test cases: two analytical oracle tests, four execute-error tests, negative-FeatureId reassignment, two preflight errors, the Small IN100 regression, and SIMPL 6.4/6.5 conversion; two stale disabled tests are retired. |
| Exemplar archive | The inline oracle needs no output archive; retained input archive `6_5_test_data_1_v2.tar.gz` has SHA512 `585b51ba...3027d6c` and a provenance sidecar. |
| Legacy comparison | **Complete (2026-07-31)** - DREAM3D 6.5.171 and SIMPLNX were run on the 4x1x1 and 6x6x6 analytical fixtures in all-phases and single-phase modes; all 22 array comparisons and 2,646 exact values matched the independent expectations. |
| Bug flags | `RequireMinNumNeighborsFilter-D1`, `RequireMinNumNeighborsFilter-D2`, `RequireMinNumNeighborsFilter-D3` |
| V&V phase | **COMPLETE.** All V&V phases complete; second-engineer review signed off by Michael A. Jackson, 2026-08-07. |

## Summary

`RequireMinNumNeighborsFilter` removes features below a neighbor-count threshold, fills their cells from dominant valid face neighbors, and compacts feature-level arrays. Verification uses two independently derived analytical tests with Class 4 invariants, focused error tests, a Small IN100 regression, and SIMPL conversion checks. Both analytical fixture families matched DREAM3D 6.5.171 in all-phases and single-phase modes; three legacy bugs affecting negative or out-of-range FeatureIds and stalled coarsening are corrected in SIMPLNX and documented as D1-D3.

## Algorithm Relationship

*Classification:* Port | **Minor changes** | Rewrite | New filter

*Evidence:* The NX algorithm retains the legacy `MinNeighbors` selection, voxel reassignment, and inactive-feature-removal. It replaces direct neighbor indexing with shared utilities and incorporates fixes for ignored arrays, data-array updates, invalid feature IDs, and non-terminating coarsening. During this V&V cycle, negative FeatureIds skip initial marking and enter the reassignment pass, out-of-range IDs return `-55567` before they are used for `activeObjects` indexing, and an iteration that cannot fill any remaining negative cell returns `-55572`.

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

Expected values are derived from the fixture definitions and the documented face-neighbor traversal, without using output from SIMPLNX or DREAM3D 6.5.171.

### Applied: 4x1x1 analytical variants

Both variants use:

- Geometry dimensions `4x1x1`.
- Minimum-neighbor threshold `2`, selected phase `1`, and four feature tuples numbered 0 through 3.
- Cell `FeatureIds = [2, 1, 1, 3]`.
- Copied cell values `[20, 101, 102, 30]`.
- Ignored cell values `[200, 101, 102, 300]`.
- Feature `Phases = [0, 1, 2, 1]`.

Feature 1 owns the two middle cells and is rejected in both variants. The left rejected cell has feature 2 as its only retained face neighbor, and the right rejected cell has feature 3 as its only retained face neighbor. Tuple copying therefore produces `[20, 20, 30, 30]`. Removing input feature 1 compacts input feature 2 to output ID 1 and input feature 3 to output ID 2, producing final `FeatureIds = [1, 1, 2, 2]`. The ignored array is unchanged.

The variants intentionally use different `NumNeighbors` inputs:

| Variant | `ApplyToSinglePhase` | Input `NumNeighbors` | Selection derivation | Expected compacted `NumNeighbors` | Expected compacted `Phases` |
|---|---:|---|---|---|---|
| All-phase | `false` | `[0, 0, 3, 3]` | Feature 1 is below 2; features 2 and 3 meet the threshold. | `[0, 3, 3]` | `[0, 2, 1]` |
| Single-phase | `true` | `[0, 0, 0, 3]` | Feature 1 is selected phase 1 and below 2; feature 2 remains active because it is phase 2 even though its count is 0; feature 3 meets the threshold. | `[0, 0, 3]` | `[0, 2, 1]` |

These are two distinct analytical inputs, not one input with only the phase-mode argument changed.

### Applied: discriminating 6x6x6 analytical fixture

Coordinates use `0 <= x,y,z <= 5` and linear cell index `i = z * 36 + y * 6 + x`. The fixture uses the same input in both phase modes:

- Every cell starts as feature 2.
- Feature 1 replaces the `3x3x3` cube `1 <= x,y,z <= 3` and the four cells `(5,5,5)`, `(4,5,5)`, `(0,5,5)`, and `(5,0,5)`. Feature 1 therefore owns 31 rejected cells.
- Feature 3 owns seed `(5,5,4)`.
- Feature 4 owns seed `(5,4,5)`.
- Feature 5 owns no cells.
- Feature `NumNeighbors = [0, 0, 3, 4, 5, 0]`.
- Feature `Phases = [0, 1, 1, 1, 1, 2]`.
- Minimum-neighbor threshold is `3`; selected phase is `1`.
- `CopiedScalar[i] = 10000 + i`.
- `CopiedVector[i] = [i + 0.25, i + 0.5, i + 0.75]` as `float32[3]`.
- `IgnoredValues[i] = 20000 + i`.

Feature 1 is rejected in both modes. The 26 cube-surface cells and four additional rejected cells can copy from retained face neighbors during the first coarsening pass; only cube center `(2,2,2)` remains for the second pass. The analytically derived rejected-set sizes are therefore 31, then 1, then 0.

The face-neighbor traversal is `-Z, -Y, -X, +X, +Y, +Z`. The source changes only when a feature's vote count is strictly greater than the prior maximum:

- Tie cell `(5,5,5)` sees feature 3 at `-Z` and feature 4 at `-Y`. The 1-vs-1 tie retains the first vote, so it copies from feature-3 seed `(5,5,4)`.
- Additional rejected cells `(4,5,5)`, `(0,5,5)`, and `(5,0,5)` copy from `(3,5,5)`, `(1,5,5)`, and `(5,1,5)`, respectively.
- Cube center `(2,2,2)` chooses its `+Z` neighbor during the second pass. That neighbor copied from exterior cell `(2,2,4)` during the first pass, so the center's final scalar and vector values originate at `(2,2,4)`.
- For every other rejected cube cell, the original source is selected by the following first-matching rule: if `z == 3`, use `(x,y,4)`; else if `y == 3`, use `(x,4,z)`; else if `x == 3`, use `(4,y,z)`; else if `x == 1`, use `(0,y,z)`; else if `y == 1`, use `(x,0,z)`; otherwise use `(x,y,0)`. This is the closed-form consequence of the traversal order when every exterior vote is for feature 2.
- Every nonrejected cell uses itself as its source.

After feature compaction, the feature-3 seed and tie cell have output ID 2, the feature-4 seed has output ID 3, and every other cell has output ID 1. The copied scalar and three vector components equal their formulas evaluated at the source cell above. `IgnoredValues` remains `20000 + i`.

| Mode | Removed input features | Retained input-to-output mapping | Expected compacted `NumNeighbors` | Expected compacted `Phases` |
|---|---|---|---|---|
| All-phase | 1 and zero-cell feature 5 | `2 -> 1`, `3 -> 2`, `4 -> 3` | `[0, 3, 4, 5]` | `[0, 1, 1, 1]` |
| Single-phase | 1 only; feature 5 remains because it is phase 2 | `2 -> 1`, `3 -> 2`, `4 -> 3`, `5 -> 4` | `[0, 3, 4, 5, 0]` | `[0, 1, 1, 1, 2]` |

Because feature 5 owns no cells, both modes have identical final cell arrays even though their compacted feature arrays have different tuple counts.

### Applied: Class 4 invariants

- Every final `FeatureIds` value is nonnegative, in range, and contiguous after compaction.
- Ignored cell values are unchanged.
- Both 6x6x6 phase modes produce identical final cell `FeatureIds`.
- Every cell and feature array continues to inherit its parent Attribute Matrix tuple dimensions.

### Encoded

- `test/RequireMinNumNeighborsTest.cpp::"SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle"` - two mode-specific 4x1x1 inputs; each asserts 18 exact array elements plus eight final-ID invariant predicates.
- `test/RequireMinNumNeighborsTest.cpp::"SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture"` - one common input executed in both phase modes; asserts 2,610 exact cell and feature values across six arrays, identical final cell `FeatureIds`, and inherited tuple dimensions.
- Both test cases pass in the current in-core Debug build.

*Second-engineer review:* **Signed off by Michael A. Jackson, 2026-08-07.** The V&V work was authored by Jared Duffey (PR #1694), so the second-engineer review is independent of the author.

## Code path coverage

13 of 18 paths are assertion-covered. Cancellation and defensive infrastructure/error paths are documented gaps.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/RequireMinNumNeighbors.cpp` (362 lines), plus preflight validation in `Filters/RequireMinNumNeighborsFilter.cpp`.

The filter selects features to retain, marks rejected-feature voxels, iteratively copies a dominant face neighbor into each rejected voxel, and removes inactive feature tuples.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | Preflight | Feature-level `NumNeighbors` and, when enabled, `FeaturePhases` tuple counts disagree - return `-252`. | `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)` |
| 2 | Preflight | FeatureIds tuple count differs from the selected Image Geometry cell count - return `-55571`. | `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds tuple count mismatch (-55571)` |
| 3 | Preflight | NeighborList-removal preflight warning is produced when feature NeighborLists are present. | `SimplnxCore::RequireMinNumNeighborsFilter` - asserts only one `-5558` warning. |
| 4 | Setup | Cell-data array discovery fails - return `-5556`. | *Not directly tested. Defensive check.* |
| 5 | Selection | Single-phase mode names a phase absent from `FeaturePhases` - return `-5555`. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)` |
| 6 | Selection | Every eligible feature is below the neighbor threshold - return `-55569` before mutation. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)` |
| 7 | Selection | All-phase or selected-phase feature meets threshold; nonselected phases remain active, including a zero-cell feature below the threshold. | `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` (`ApplyToSinglePhase=0/1`) and `SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture` |
| 8 | Cancellation | Cancellation before mutation or within coarsening returns without further changes. | *Not directly tested. Requires cancel signal injection.* |
| 9 | Marking | Rejected feature IDs are marked and retained feature IDs remain available for coarsening. | Both analytical oracle tests assert the exact post-marking/coarsening result. |
| 10 | Reassignment | Rejected cells honor X/Y/Z boundary validity and `-Z,-Y,-X,+X,+Y,+Z` traversal, preserve the first feature in a 1-vs-1 tie, and repeat until an enclosed rejected cell can be filled. | `SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture` - exact source-index assertions cover all axes, the tie, boundary cells, and the second-pass cube center. |
| 11 | Reassignment | Feature ID is outside the feature-array range - return `-55567` before it is used for indexing. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)` |
| 12 | Reassignment | Negative Feature IDs skip initial marking and are reassigned from valid face neighbors. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned` |
| 13 | Reassignment | Negative Feature IDs remain but an iteration finds no non-negative face neighbor to copy - return `-55572` instead of looping indefinitely. | `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - no coarsening progress (-55572)` |
| 14 | Copy | A rejected cell copies every nonignored cell-data tuple from its selected neighbor while ignored arrays remain unchanged. | Both analytical oracle tests; the 6x6x6 fixture checks scalar `int32`, three-component `float32`, ignored `int32`, and `FeatureIds` tuples for all 216 cells in both modes. |
| 15 | Copy | A destination tuple index is outside the cell array, FeatureIds array, or neighbor map range - return `-55568` before indexing. | *Not directly tested. AttributeMatrix construction and the FeatureIds/Image Geometry tuple-count preflight reject mismatched tuple shapes before execution. Defensive check.* |
| 16 | Copy | A source tuple index is outside the cell array or FeatureIds array range - return `-55573` before indexing. | *Not directly tested. Valid face-neighbor computation constrains source indexes to the Image Geometry cell range. Defensive check.* |
| 17 | Finalize | Inactive feature tuples are removed and remaining FeatureIds are remapped; NeighborLists are removed. | `SimplnxCore::RequireMinNumNeighborsFilter`, both analytical oracle tests, and their exact compacted `NumNeighbors`, `Phases`, and `FeatureIds` assertions. |
| 18 | Finalize | Inactive-feature removal fails - return `-55570`. | *Not directly tested. Defensive check.* |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `SimplnxCore::RequireMinNumNeighborsFilter: Analytical Oracle` | new-for-V&V | Class 1/4 harness for two generated 4x1x1 variants. Each variant checks five arrays, 18 exact element values, eight final-ID invariants, feature-array tuple counts, and inherited tuple dimensions. |
| `ApplyToSinglePhase=0` | new-for-V&V | `DYNAMIC_SECTION` generated by `GENERATE(false, true)`; uses input `NumNeighbors [0,0,3,3]` and verifies all-phase selection and compaction. |
| `ApplyToSinglePhase=1` | new-for-V&V | `DYNAMIC_SECTION` generated by `GENERATE(false, true)`; uses input `NumNeighbors [0,0,0,3]` and verifies that nonselected phase-2 feature 2 remains active. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Discriminating 6x6x6 Analytical Fixture` | new-for-V&V | Runs one common input in both phase modes. Checks six arrays with 2,610 exact element comparisons, a 1-vs-1 traversal-order tie, X/Y/Z boundaries, second-pass copying, ignored values, zero-cell feature retention, equal cell output across modes, and tuple dimensions. |
| `SimplnxCore::RequireMinNumNeighborsFilter` | kept | Modified: loads `6_5_test_data_1_v2.tar.gz`, recomputes neighbors, derives the expected retained `NumElements` sequence from the pre-filter input and threshold, checks all 791 retained values, asserts the single `-5558` preflight warning, and verifies tuple dimensions. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - tuple count mismatch (-252)` | kept | Creates 5-tuple `NumNeighbors` and 4-tuple `Phases` arrays, then asserts invalid preflight and exact code `-252`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Preflight Error - FeatureIds tuple count mismatch (-55571)` | new-for-V&V | Creates a 5-cell geometry with four `FeatureIds` tuples, then asserts invalid preflight and exact code `-55571`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - feature ID out of range (-55567)` | new-for-V&V | Supplies `FeatureIds [1,4,2,3]` for four feature tuples, asserts valid preflight, invalid execution, and exact code `-55567`; covers D2. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute - negative feature ID is reassigned` | new-for-V&V | Supplies `FeatureIds [-1,1,1,2]`, asserts successful execution, and checks all four final IDs equal `[1,1,1,2]`; covers D1. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - unavailable phase (-5555)` | new-for-V&V | Selects absent phase 5, then asserts valid preflight, invalid execution, and exact code `-5555`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - all features rejected (-55569)` | new-for-V&V | Gives every nonzero feature zero neighbors with threshold 1, then asserts valid preflight, invalid execution, and exact code `-55569`. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Execute Error - no coarsening progress (-55572)` | new-for-V&V | Uses four negative cells with an otherwise active feature, asserts one execution error with code `-55572`, and verifies all four `FeatureIds` remain `-1`; covers D3. |
| `SimplnxCore::RequireMinNumNeighborsFilter: SIMPL Backwards Compatibility` | kept | Loads the two conversion fixtures below and verifies one converted filter, UUID, comments, and seven converted arguments in each section. |
| `SIMPL 6.5 (UUID)` | kept | `DYNAMIC_SECTION` using `test/simpl_conversion/6_5/RequireMinNumNeighborsFilter.json`; verifies UUID-based 6.5 pipeline conversion. |
| `SIMPL 6.4 (Filter_Name)` | kept | `DYNAMIC_SECTION` using `test/simpl_conversion/6_4/RequireMinNumNeighborsFilter.json`; verifies name-based 6.4 pipeline conversion. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Bad Phase Number` | retired | Removed from `#if 0`; it used stale parameter keys and could not compile. The active `unavailable phase (-5555)` test replaces its intended error coverage. |
| `SimplnxCore::RequireMinNumNeighborsFilter: Phase Array` | retired | Removed from `#if 0`; it used stale parameter keys and could not compile. Current analytical variants cover the all-phase and single-phase selection behavior. |

There are 11 active `TEST_CASE`s: 3 kept and 8 new for this V&V cycle.

## Exemplar archive

No new output archive was created for this V&V cycle because both Class 1 fixtures and their expected values are encoded inline.

The retained Small IN100 regression uses this shared input archive:

| Field | Value |
|---|---|
| Archive | `6_5_test_data_1_v2.tar.gz` |
| SHA512 | `585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c` |
| CMake registration | `src/Plugins/SimplnxCore/test/CMakeLists.txt` |
| Provenance | `src/Plugins/SimplnxCore/vv/provenance/6_5_test_data_1_v2.md` |
| Role in this V&V | Input only. The test derives expected retained `NumElements` values from pre-filter arrays and the independently recomputed neighbor threshold. |

No archived array is used as the canonical correctness oracle for this filter.

## Deviations from DREAM3D 6.5.171

On 2026-07-31, DREAM3D 6.5.171 and SIMPLNX were run on the 4x1x1 and discriminating 6x6x6 analytical fixtures in both all-phases and single-phase modes. All 22 array comparisons and 2,646 exact values matched between the two implementations and the independent analytical expectations.

The complete pipelines, shared inputs, outputs, logs, comparison tables, and checksums were uploaded to OneDrive on 2026-07-31 in `FilterVerification_RequireMinNumNeighborsFilter`. These valid fixtures do not trigger the D1-D3 error-condition deviations documented below.

Three user-visible differences occur for negative or out-of-range FeatureIds or a coarsening pass that cannot make progress:

- `RequireMinNumNeighborsFilter-D1` - SIMPLNX safely accepts a negative FeatureId and reassigns it from a valid face neighbor; DREAM3D 6.5.171 uses the negative value as an out-of-bounds `activeObjects` index.
- `RequireMinNumNeighborsFilter-D2` - SIMPLNX returns `-55567` for an out-of-range non-negative FeatureId before indexing; DREAM3D 6.5.171 uses the value as an out-of-bounds `activeObjects` index.
- `RequireMinNumNeighborsFilter-D3` - SIMPLNX returns `-55572` when remaining negative cells have no non-negative face neighbor; DREAM3D 6.5.171 continues the coarsening loop indefinitely.

See `vv/deviations/RequireMinNumNeighborsFilter.md` for root cause, affected users, and migration recommendations.
