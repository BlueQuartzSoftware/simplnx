# V&V Report: GroupMicroTextureRegionsFilter

|           |                          |
|-----------|--------------------------|
| Plugin    | OrientationAnalysis      |
| SIMPLNX UUID | `3f695987-81b1-47c3-8cff-b49cfa219be0` |
| DREAM3D 6.5.171 equivalent | `GroupMicroTextureRegions` (private filter; not registered for pipeline use) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson <mike.jackson@bluequartz.net> — 2026-08-27 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port with minor algorithm improvements and updates for the SIMPLNX API.** The port retains the legacy BFS grouping algorithm and adds reproducibility controls, two-sided Laue validation, and explicit output handling. |
| Oracle (confirmed) | **Class 1 (Analytical) with Class 4 (Invariant) support.** Five inline analytical fixtures derive expected groups from pure-Bunge c-axis angles, phase classes, volumes, and neighbor lists. |
| Code paths enumerated | **14 of 20 exercised.** Six defensive or low-value paths remain uncovered and are listed below. |
| Tests today | **8 ctest cases and 225 assertions.** Tests cover both comparison targets, tolerance acceptance and rejection, mixed-Laue rejection, non-contiguous neighbors, parent-ID randomization, output invariants, and SIMPL conversion. |
| Exemplar archive | **None for this filter.** The oracle data is generated inline in the unit test. |
| Legacy comparison | **Run — DREAM3D-NX versus DREAM.3D 6.6.382 (`107b8d51b`).** DREAM.3D 6.5.171 could not instantiate the filter because it was not registered. |
| Bug flags | **None.** D6 is fixed in this branch. |
| V&V phase | **COMPLETE.** Second-engineer review and sign-off remain. |

## Summary

`GroupMicroTextureRegionsFilter` groups neighboring hexagonal Features whose c-axes satisfy a selected angular tolerance. Verification uses inline Class 1 analytical fixtures and Class 4 invariants. The V&V found and fixed one reproducibility defect and documents six active deviations from DREAM.3D 6.5.171.

## Algorithm Relationship

**Port with minor algorithm improvements and updates for the SIMPLNX API.**

*Evidence:* SIMPLNX preserves the legacy random-seed selection, BFS traversal, contiguous and optional non-contiguous neighbor processing, c-axis comparison, volume-weighted running average, parent assignment, and optional label shuffle. The SIMPL UUID maps to the SIMPLNX UUID.

Material updates:

1. **SIMPLNX API and validation.** SIMPLNX uses selection parameters, output actions, `DataPath` values, and a separate algorithm class. It inlines the legacy `GroupFeatures` base-class control flow.
2. **Reproducibility controls.** SIMPLNX exposes parent-ID randomization and seed controls. It uses one generator per execution. See D2 and D6.
3. **Corrected Laue validation.** SIMPLNX validates both Features when the running average is enabled. See D3.
4. **Output and precision updates.** SIMPLNX retires the unused `Active` output and uses float32 tolerance conversion. See D5 and D7.

## Oracle

*Class:* **1 (Analytical)** with **4 (Invariant)** support.

*Applied:* Pure-Bunge angles `(0, Phi, 0)` give a sample-frame c-axis of `(0, sin Phi, cos Phi)`. Therefore, the c-axis distance is derived directly from the selected `Phi` values. Fixed phase classes, volumes, and neighbor lists then give the expected group partition without using DREAM.3D output.

*Encoded:* `test/GroupMicroTextureRegionsTest.cpp` contains five Class 1 fixtures: `Pure-Phi Bunge`, `Tolerance Boundary`, `Running Average`, `Mixed-Laue Rejection`, and `Non-contiguous neighbor grouping`. Class 4 assertions verify group count, equivalence classes, cell-to-Feature parent consistency, parent Attribute Matrix size, parent-ID positivity, seed roundtrip, randomization invariance, and same-seed determinism. All eight ctest cases pass.

*Second-engineer review:* Adam L. Pilchak reviewed the metallurgical intent on 2026-08-11. Software implementation and test review are pending PR review.

## Bugs found and fixed

This branch fixes the defect in this table. The fix will be in the DREAM3D-NX release after v7.4.1.

| Deviation | Defect | Affected released versions | Resolution in this branch |
|---|---|---|---|
| `GroupMicroTextureRegionsFilter-D6` | The default used a clock-derived seed while the running-average comparison was enabled by default. Repeated runs of the same pipeline could produce different region partitions. | DREAM3D-NX v7.0.0 through v7.4.1. | `UseSeed` is enabled by default. The filter stores the seed so the run can be repeated. |

## Code path coverage

14 of 20 paths exercised. Six defensive or low-value gaps remain.

Source: `src/Plugins/OrientationAnalysis/src/OrientationAnalysis/Filters/Algorithms/GroupMicroTextureRegions.cpp` (333 lines).

The algorithm has four logical phases: seed initialization, BFS traversal, grouping decisions, and output finalization.

| # | Phase | Path | Test case |
|---|---|---|---|
| 1 | Seed initialization | Use the supplied RNG seed. | All analytical tests; the Pure-Phi test also verifies the stored seed. |
| 2 | Seed initialization | Derive the seed from the system clock. | *Not directly tested. This nondeterministic path cannot have a fixed expected parent array.* |
| 3 | BFS traversal | Disable non-contiguous neighbors. | `Pure-Phi Bunge`; `Default contiguous-only neighbor mode` |
| 4 | BFS traversal | Enable a valid non-contiguous neighbor list. | `Non-contiguous neighbor grouping` |
| 5 | BFS traversal | Return `-99345` when an enabled non-contiguous list is missing. | *Not directly tested. Selection-parameter validation prevents a missing list during normal filter execution.* |
| 6 | BFS traversal | Traverse a contiguous neighbor edge. | `Pure-Phi Bunge`; `Tolerance Boundary`; `Running Average`; `Mixed-Laue Rejection` |
| 7 | BFS traversal | Traverse a non-contiguous neighbor edge. | `Non-contiguous neighbor grouping` |
| 8 | BFS traversal | Skip an invalid or self neighbor. | *Not directly tested. Valid generated neighbor lists do not contain invalid or self entries.* |
| 9 | Grouping decision | Skip a neighbor that already has a parent. | `Pure-Phi Bunge` exercises reverse edges after a Feature joins a group. |
| 10 | Grouping decision | Skip a phase-0 reference or candidate. | *Not directly tested. Feature 0 is not present in the generated neighbor lists.* |
| 11 | Grouping decision | Reject different or non-hexagonal Laue classes. | `Mixed-Laue Rejection` verifies 20 cubic/hexagonal pairs. |
| 12 | Grouping decision | Compare against the touching Feature. | `Pure-Phi Bunge`; `Tolerance Boundary` |
| 13 | Grouping decision | Compare against and update the volume-weighted running average. | `Running Average`; `Mixed-Laue Rejection` |
| 14 | Grouping decision | Accept direct angular distance within tolerance. | `Pure-Phi Bunge`; `Running Average`; `Non-contiguous neighbor grouping` |
| 15 | Grouping decision | Accept the antipodal `pi - w` branch. | *Not directly tested. The analytical fixtures use c-axes in one hemisphere.* |
| 16 | Grouping decision | Reject angular distance outside tolerance. | `Pure-Phi Bunge`; `Tolerance Boundary`; `Running Average` |
| 17 | Finalization | Keep sequential parent IDs. | All analytical fixtures |
| 18 | Finalization | Randomize parent IDs. | `RandomizeParentIds invariants` |
| 19 | Finalization | Return `-87000` when fewer than two parent tuples exist. | *Not directly tested. Valid fixtures contain real Features and produce at least one parent plus the reserved tuple.* |
| 20 | Finalization | Resize the parent Attribute Matrix and backfill cell parent IDs. | `Pure-Phi Bunge`; `RandomizeParentIds invariants` |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Pure-Phi Bunge)` | new-for-V&V, modified | Verifies three analytical groups, cell-to-Feature parent consistency, parent Attribute Matrix size, retired `Active` output absence (D5), and seed roundtrip. 39 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: RandomizeParentIds invariants` | new-for-V&V | Verifies equivalence-class preservation, group count, cell-to-Feature consistency, positivity, same-seed determinism, and shuffle execution. 49 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Tolerance Boundary)` | new-for-V&V | Verifies one 8 degree acceptance and one 12 degree rejection at a 10 degree tolerance. 21 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Running Average)` | new-for-V&V | Verifies that a 0/9/18 degree chain produces two groups under the running-average comparison for every seed order. 20 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Class 1 Analytical (Mixed-Laue Rejection)` | new-for-V&V | Verifies that 20 aligned cubic/hexagonal pairs remain separate. A temporary D3 restoration caused 14 pair assertions to fail. 37 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Non-contiguous neighbor grouping` | new-for-V&V | Verifies grouping through an optional non-contiguous list when all contiguous lists are empty. 18 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Default contiguous-only neighbor mode` | new-for-V&V | Verifies that the default contiguous-only configuration executes successfully. 2 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: SIMPL Backwards Compatibility` | kept, modified | Converts the SIMPL 6.4 and 6.5 fixtures after the obsolete `ActiveArrayName` mapping was removed. 39 assertions. |
| `OrientationAnalysis::GroupMicroTextureRegionsFilter: Valid Filter Execution` (`[UNIMPLEMENTED][!mayfail]`) | retired | The test used empty paths and could not execute the algorithm. The analytical fixtures replace it. |

## Exemplar archive

- **Archive:** None. The test creates all oracle inputs inline.
- **SHA512:** N/A
- **Provenance:** `src/Plugins/OrientationAnalysis/vv/provenance/GroupMicroTextureRegionsFilter.md`

## Deviations from DREAM.3D 6.5.171

DREAM.3D 6.5.171 contains the filter source but does not register the filter. Therefore, a direct pipeline comparison is not possible. The empirical A/B comparison used DREAM.3D 6.6.382, built from commit `107b8d51b`, because that version registers the filter. Both implementations ran only Group MicroTexture Regions on byte-identical input. The independent Class 1 and Class 4 oracle establishes correctness. The DREAM.3D 6.6.382 comparison provides deviation and migration evidence.

See `vv/deviations/GroupMicroTextureRegionsFilter.md` for the root cause, affected users, and recommendation for each deviation.

| Deviation | Observed difference |
|---|---|
| `GroupMicroTextureRegionsFilter-D2` | DREAM.3D always randomizes parent labels. DREAM3D-NX uses reproducible sequential labels by default and provides optional seeded randomization. |
| `GroupMicroTextureRegionsFilter-D3` | With the running average enabled, DREAM.3D can group a non-hexagonal Feature with a hexagonal Feature. DREAM3D-NX validates both Laue classes and rejects the pair. |
| `GroupMicroTextureRegionsFilter-D4` | DREAM.3D defaults to the touching-Feature comparison. DREAM3D-NX defaults to the running-average comparison. |
| `GroupMicroTextureRegionsFilter-D5` | DREAM.3D creates an unused `Active` array. DREAM3D-NX retires this output and retains the parent Attribute Matrix. |
| `GroupMicroTextureRegionsFilter-D6` | Released DREAM3D-NX versions used a clock-derived seed by default. The verified branch enables a fixed seed by default. |
| `GroupMicroTextureRegionsFilter-D7` | DREAM.3D and DREAM3D-NX use different pi precision when converting the tolerance to radians. A candidate at the float32 boundary can produce a different result. |
