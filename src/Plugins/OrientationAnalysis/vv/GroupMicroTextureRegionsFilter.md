# V&V Report: GroupMicroTextureRegionsFilter

|        |              |
|--------|--------------|
| Plugin | OrientationAnalysis |
| SIMPLNX UUID | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| SIMPLNX Human Name | Group MicroTexture Regions |
| DREAM.3D v6.5.171 equivalent | `GroupMicroTextureRegions` (private filter; not registered for pipeline use) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-08-27; second engineer: Joey Kleingers, 2026-09-20 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Minor changes. The filter retains the DREAM.3D v6.5.171 grouping method and changes defaults, random-seed controls, validation, and outputs. |
| Oracle (confirmed) | Classes 1 and 4: five analytical fixtures and output invariants in `test/GroupMicroTextureRegionsTest.cpp`. All pass in CI. |
| Code paths enumerated | 20 of 35 exercised. Gaps include cancellation, data-access failures, full cell batches, and selected edge conditions. |
| Tests today | 9 test cases cover analytical grouping, randomization, phase bounds, optional neighbors, and SIMPL conversion. All pass in CI. |
| Exemplar archive | None. The tests construct their inputs inline. |
| Legacy comparison | Not run against DREAM.3D v6.5.171: the filter is private. The 2026-08-27 local proof comparison predates changes to the current algorithm. |
| Bug flags | None |
| V&V phase | COMPLETE. Sign-off is recorded above. |

## Summary

`GroupMicroTextureRegionsFilter` groups neighboring hexagonal Features whose c-axes satisfy an angular tolerance. Five Class 1 fixtures and Class 4 invariants check the output, and a regression test checks phase-index errors. All pass in CI.

## Algorithm Relationship

*Classification:* Minor changes

*Evidence:* `GroupFeatures::execute()` and `GroupMicroTextureRegions::determineGrouping()` in DREAM.3D v6.5.171 supply the same neighbor traversal and c-axis tests; SIMPLNX adds seed controls, two-sided Laue checks, phase bounds, and revised outputs.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Pure-Bunge angles give known c-axis separations; explicit neighbor lists, phases, and unit volumes determine the expected groups.

*Encoded:* `src/Plugins/OrientationAnalysis/test/GroupMicroTextureRegionsTest.cpp` — `Class 1 Analytical (Pure-Phi Bunge)`, `(Tolerance Boundary)`, `(Running Average)`, `(Mixed-Laue Rejection)`, and `Non-contiguous neighbor grouping`: five fixtures, plus `RandomizeParentIds invariants`. All pass in CI.

*Second-engineer review:* Joey Kleingers, 2026-09-20; Adam Pilchak reviewed metallurgical intent on 2026-08-11.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `GroupMicroTextureRegionsFilter-D3` and `GroupMicroTextureRegionsFilter-D6`. The fix for `GroupMicroTextureRegionsFilter-D8` will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `GroupMicroTextureRegionsFilter-D3` | The running-average mode could join a cubic Feature to a hexagonal Feature. | No DREAM3D-NX release. DREAM.3D v6.5.171 was affected in its private filter source. | Both Features must have the hexagonal-high Laue class. |
| `GroupMicroTextureRegionsFilter-D6` | A clock-derived default seed made running-average partitions vary between runs. | No DREAM3D-NX release. DREAM.3D v6.5.171 was affected in its private filter source when running average was enabled. | Fixed-seed execution is enabled by default, and the seed is stored. |
| `GroupMicroTextureRegionsFilter-D8` | A positive phase beyond Crystal Structures could cause an out-of-bounds read. | DREAM3D-NX v7.4.2. DREAM.3D v6.5.171 was affected in its private filter source. | A participating reference or neighbor with an invalid positive phase returns `-87001`. |

## Code path coverage

20 of 35 paths exercised. Each uncovered path and its test gap are listed below.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/GroupMicroTextureRegions.cpp` (423 lines); `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/GroupMicroTextureRegionsFilter.cpp` (244 lines).

Short test names below refer to the full names in Test inventory.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Seed initialization | Use the supplied RNG seed. | All analytical tests; the Pure-Phi test also verifies the stored seed. |
| 2 | Seed initialization | Derive the seed from the system clock. | *Not directly tested. This nondeterministic path cannot have a fixed expected parent array.* |
| 3 | BFS traversal | Disable non-contiguous neighbors. | `Pure-Phi Bunge`; `Default contiguous-only neighbor mode` |
| 4 | BFS traversal | Enable a valid non-contiguous neighbor list. | `Non-contiguous neighbor grouping` |
| 5 | BFS traversal | Return `-99345` when an enabled non-contiguous list is missing. | *Not directly tested. Selection-parameter validation prevents a missing list during normal filter execution.* |
| 6 | BFS traversal | Traverse a contiguous neighbor edge. | `Pure-Phi Bunge`; `Tolerance Boundary`; `Running Average`; `Mixed-Laue Rejection` |
| 7 | BFS traversal | Traverse a non-contiguous neighbor edge. | `Non-contiguous neighbor grouping` |
| 8 | BFS traversal | Skip a negative or self neighbor. | *Not directly tested. The fixtures contain no negative or self entries.* |
| 9 | Grouping decision | Skip a neighbor that already has a parent. | `Pure-Phi Bunge` exercises reverse edges after a Feature joins a group. |
| 10 | Grouping decision | Skip a phase-0 reference or candidate. | *Not directly tested. Feature 0 is not present in the generated neighbor lists.* |
| 11 | Grouping decision | Reject different or non-hexagonal Laue classes. | `Mixed-Laue Rejection` verifies 20 cubic/hexagonal pairs. |
| 12 | Grouping decision | Compare against the touching Feature. | `Pure-Phi Bunge`; `Tolerance Boundary` |
| 13 | Grouping decision | Compare against and update the running average with unit volumes. | `Running Average`; `Mixed-Laue Rejection` |
| 14 | Grouping decision | Accept direct angular distance within tolerance. | `Pure-Phi Bunge`; `Running Average`; `Non-contiguous neighbor grouping` |
| 15 | Grouping decision | Accept the antipodal `pi - w` condition. | *Not directly tested. The analytical fixtures use c-axes in one hemisphere.* |
| 16 | Grouping decision | Reject angular distance outside tolerance. | `Pure-Phi Bunge`; `Tolerance Boundary`; `Running Average` |
| 17 | Finalization | Keep sequential parent IDs. | All analytical fixtures |
| 18 | Finalization | Randomize parent IDs. | `RandomizeParentIds invariants` |
| 19 | Finalization | Return `-87000` when fewer than two parent tuples exist. | *Not directly tested. Every fixture has Feature tuples and produces parents.* |
| 20 | Finalization | Resize the parent Attribute Matrix and backfill cell parent IDs. | `Pure-Phi Bunge`; `RandomizeParentIds invariants` |
| 21 | Grouping decision | Reject a positive reference phase beyond Crystal Structures with `-87001`. | `Phase Index Bounds` |
| 22 | Grouping decision | Reject a positive neighbor phase beyond Crystal Structures with `-87001`. | `Phase Index Bounds` |
| 23 | Grouping decision | Leave an isolated Feature's invalid phase unused. | `Phase Index Bounds` — isolated Feature section |
| 24 | Data access | Read Feature arrays into buffers and write Feature parent IDs successfully. | `Class 1 Analytical (Pure-Phi Bunge)` |
| 25 | Data access | Propagate a failed read of phases, crystal structures, quaternions, or volumes. | *Not directly tested. Requires a failing data store.* |
| 26 | Data access | Propagate a failed Feature-parent write. | *Not directly tested. Requires a failing data store.* |
| 27 | Cell mapping | Map a full 65,536-cell batch and advance to another batch. | *Not directly tested. All executing fixtures have fewer cells.* |
| 28 | Cell mapping | Map a partial final batch. | `Class 1 Analytical (Pure-Phi Bunge)`; `RandomizeParentIds invariants` |
| 29 | Cell mapping | Propagate a failed cell-array read or write. | *Not directly tested. Requires a failing data store.* |
| 30 | Cancellation | Stop before selecting the next group seed. | *Not directly tested. Requires cancellation injection.* |
| 31 | Cancellation | Stop after grouping and before output finalization. | *Not directly tested. Requires cancellation injection.* |
| 32 | Cancellation | Stop during cell-parent mapping. | *Not directly tested. Requires cancellation injection.* |
| 33 | Finalization | Propagate a parent Attribute Matrix resize error. | *Not directly tested. Requires a failing resize.* |
| 34 | Seed search | Stop when all Features have a parent. | `Pure-Phi Bunge`; `RandomizeParentIds invariants` |
| 35 | Seed search | Wrap the search index to zero after the final Feature. | *Not directly tested. No fixture isolates the wrap condition.* |

## Test inventory

All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Phase Index Bounds` | kept | Checks `-87001` for invalid reference and neighbor phases; permits an invalid phase on an isolated Feature. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Pure-Phi Bunge)` | new-for-V&V | Checks three groups, cell-to-Feature parent consistency, parent matrix size, absent `Active` output (D5), and stored seed. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: RandomizeParentIds invariants` | new-for-V&V | Checks group membership, group count, cell mapping, positive labels, same-seed determinism, and changed labels when randomization is enabled. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Tolerance Boundary)` | new-for-V&V | Checks 8-degree acceptance and 12-degree rejection at a 10-degree tolerance. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Running Average)` | new-for-V&V | Checks that the 0/9/18-degree chain gives two groups at a 10-degree tolerance with unit volumes and seed 42. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Mixed-Laue Rejection)` | new-for-V&V | Checks that all 20 aligned cubic/hexagonal pairs remain separate. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Non-contiguous neighbor grouping` | new-for-V&V | Checks grouping through the optional list with empty contiguous lists. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Default contiguous-only neighbor mode` | new-for-V&V | Checks successful preflight and execution with optional neighbors disabled; does not assert parent values. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: SIMPL Backwards Compatibility` | kept | Checks loaded parameters in `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections; the obsolete `ActiveArrayName` is ignored. |

## Exemplar archive

- **Archive:** None. All oracle inputs are constructed inline.
- **SHA512:** Not applicable. `src/Plugins/OrientationAnalysis/test/CMakeLists.txt` has no archive registration for this filter.
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/GroupMicroTextureRegionsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/GroupMicroTextureRegionsFilter.md` for the root cause, affected users, and recommendation. These differences follow from the private DREAM.3D v6.5.171 source.

| Deviation | Observed difference |
|-----------|---------------------|
| `GroupMicroTextureRegionsFilter-D2` | DREAM.3D randomizes parent labels by default. SIMPLNX exposes label randomization and fixed-seed controls. |
| `GroupMicroTextureRegionsFilter-D3` | SIMPLNX checks both Features' Laue classes in running-average mode. DREAM.3D checks only the candidate's class. |
| `GroupMicroTextureRegionsFilter-D4` | SIMPLNX defaults to running-average comparison and a 20-degree tolerance. DREAM.3D defaults to touching-Feature comparison and 1 degree. |
| `GroupMicroTextureRegionsFilter-D5` | SIMPLNX omits the unused `Active` output and retains the parent Attribute Matrix. |
| `GroupMicroTextureRegionsFilter-D7` | Float32 tolerance conversion in SIMPLNX can change a decision at the acceptance boundary. |
