# V&V Report: ComputeFeatureCentroidsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `c6875ac7-8bdd-4f69-b6ce-82ac09bd3421` |
| SIMPLNX Human Name | Compute Feature Centroids |
| DREAM.3D v6.5.171 equivalent | `FindFeatureCentroids` (Generic plugin, SIMPL UUID `6f8ca36f-2995-5bd3-8672-6b0b80d5b2ca`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-07; second engineer: Matthew Marine, 2026-07-10 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of the non-periodic `FindFeatureCentroids` calculation in DREAM.3D v6.5.171. SIMPLNX adds periodic centroids, double-precision cell coordinates and bulk data access. |
| Oracle (confirmed) | Class 1 analytical means and Class 4 invariants are encoded in `test/ComputeFeatureCentroidsTest.cpp`. All pass in CI. |
| Code paths enumerated | 16 of 26 exercised by the five ordinary test cases. The table identifies validation, cancellation, I/O and periodic-axis gaps. |
| Tests today | 6 test cases: analytical centroids, periodic centroids, invalid IDs, bulk reads, SIMPL conversion and a hidden storage-contract fixture. |
| Exemplar archive | None. The tests construct inputs and expected values in C++. |
| Legacy comparison | Run against DREAM.3D v6.5.171 on 2026-10-22: 1 of 3 cases match; the differences come from `ComputeFeatureCentroidsFilter-D1` and `ComputeFeatureCentroidsFilter-D2`. A local proof version with the D1 fix matches DREAM3D-NX in 2 of 3 cases; the remaining case uses the DREAM3D-NX-only periodic option (D2). |
| Bug flags | None. |
| V&V phase | COMPLETE. |

## Summary

`ComputeFeatureCentroidsFilter` calculates each feature's mean cell-center position with compensated summation. Its optional periodic mode uses a circular mean on axes where the feature spans the image. Analytical fixtures and invariants verify the results; the V&V found and fixed two periodic-centroid defects.

## Algorithm Relationship

*Classification:* Port

*Evidence:* The non-periodic calculation preserves the Kahan sums and division by cell count in `Source/Plugins/Generic/GenericFilters/FindFeatureCentroids.cpp`; SIMPLNX adds double-precision coordinates, periodic means, validation and bulk reads.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Hand-derived arithmetic means check ordinary features; independently unwrapped coordinates check periodic fixtures D and E, while fixture F checks the arithmetic fallback and fixture D checks an unchanged non-spanning feature.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeFeatureCentroidsTest.cpp::Class 1 - Analytical Centroids` and `::Class 1/4 - Periodic Boundary` contain six fixtures A–F with absolute margin `1e-4`; `::Bulk boundary and failure propagation` adds a 65,537-cell analytical fixture. All pass in CI.

*Second-engineer review:* Michael Jackson, 2026-07-16 (recorded technical-authority review).

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fix for `ComputeFeatureCentroidsFilter-D3`. The fix for `ComputeFeatureCentroidsFilter-D4` will be in the DREAM3D-NX release after v7.4.2.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeFeatureCentroidsFilter-D3` | The periodic offset ignored spacing and mixed cell-index distances with physical coordinates. | DREAM3D-NX v7.2.1 through v7.4.1. DREAM.3D v6.5.171 was not affected. | Periodic calculations use physical coordinates and domain lengths that include spacing. |
| `ComputeFeatureCentroidsFilter-D4` | A constant half-span offset gave incorrect centroids for asymmetric wrapped features and could place them outside the domain. | DREAM3D-NX v7.2.1 through v7.4.2. DREAM.3D v6.5.171 was not affected. | A circular mean replaces the offset; a near-zero resultant preserves the arithmetic mean. |

## Code path coverage

16 of 26 paths exercised. The count uses the five ordinary CI test cases; the hidden storage-contract fixture is listed separately in the inventory.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.cpp` (218 lines), `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeFeatureCentroidsFilter.cpp` (165 lines), and `src/simplnx/Utilities/DataArrayUtilities.cpp` (515 lines; shared FeatureIds validation).

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Create a float32 centroid array with three components per feature | `Class 1 - Analytical Centroids` |
| 2 | Preflight | Missing feature AttributeMatrix → error `-12700` | *Not directly tested. Parameter validation rejects a missing selection before this guard.* |
| 3 | Validation | FeatureId exceeds the feature tuple count → error `-5351` | `Error - FeatureId exceeds Feature AM` checks an invalid result, without asserting the error code |
| 4 | Validation | Negative FeatureId → error `-5355` | *Not directly tested. No fixture supplies a negative ID.* |
| 5 | Validation | Propagate a FeatureIds bulk-read failure | `Bulk boundary and failure propagation` injects `-91911` on the second validation read and checks that output remains zero |
| 6 | Sweep | Cancellation before a chunk → return | *Not directly tested. No fixture injects cancellation.* |
| 7 | Sweep | Read a full 65,536-cell chunk and a partial tail | `Bulk boundary and failure propagation` checks centroids across the boundary |
| 8 | Sweep | Propagate a FeatureIds read failure during accumulation | *Not directly tested. The injected second-read failure occurs during validation.* |
| 9 | Sweep | Skip a negative FeatureId | *Not directly tested. Validation rejects negative IDs before the sweep.* |
| 10 | Sweep | Accumulate cells assigned to feature 0 | *Not directly tested. Ordinary fixtures leave feature 0 empty; only the hidden storage-contract fixture checks its nonzero centroid.* |
| 11 | Sweep | Track index ranges and accumulate X, Y and Z with Kahan sums | `Class 1 - Analytical Centroids`, fixtures A–C |
| 12 | Sweep | Periodic mode accumulates unit vectors | `Class 1/4 - Periodic Boundary`, fixtures D–F |
| 13 | Finalize | Nonempty feature → divide each sum by its count | `Class 1 - Analytical Centroids`, fixtures A–C |
| 14 | Finalize | Empty feature → retain zero centroid | `Class 1 - Analytical Centroids`, fixtures A–C |
| 15 | Periodic | Periodic mode off → retain arithmetic means | `Class 1/4 - Periodic Boundary`, fixtures D and E, non-periodic controls |
| 16 | Periodic | Empty feature → skip adjustment | `Class 1/4 - Periodic Boundary`, empty feature 0 in fixtures D–F |
| 17 | Periodic | Feature does not span an axis → retain its component | `Class 1/4 - Periodic Boundary`, fixture D, feature 2 |
| 18 | Periodic | Resultant below `1e-6` → retain arithmetic mean | `Class 1/4 - Periodic Boundary`, fixture F, X component |
| 19 | Periodic | Negative angle → add a full turn before conversion | `Class 1/4 - Periodic Boundary`, fixture D, X = 3.5 |
| 20 | Periodic | Nonnegative angle → convert directly to a physical coordinate | `Class 1/4 - Periodic Boundary`, fixture E, X = 11 |
| 21 | Periodic | No component adjusted → omit adjustment message | *Not directly tested. The one-cell Y and Z extents still trigger component adjustment in fixture F.* |
| 22 | Periodic | At least one component adjusted → send adjustment message | `Class 1/4 - Periodic Boundary`, fixtures D–F exercise the path without checking the message |
| 23 | Output | Publish finalized centroids with one bulk write | `Class 1 - Analytical Centroids` and `Bulk boundary and failure propagation` |
| 24 | Output | Propagate a centroid write failure | *Not directly tested. No fixture injects an output-store failure.* |
| 25 | Sweep | Empty image → skip all chunks | *Not directly tested. Every fixture has at least one cell.* |
| 26 | Periodic | Wrap a feature across a multi-cell Y or Z extent | *Not directly tested. Periodic fixtures use one-cell Y and Z extents.* |

## Test inventory

The five ordinary test cases run in in-core CI on Linux, macOS and Windows. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeFeatureCentroidsFilter: real HDF5 65536-block tail oracle` | new-for-V&V | Hidden. Defines exact centroids for feature 0, a two-cell feature across the block boundary and an empty feature, with periodic mode off and on. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Class 1 - Analytical Centroids` | new-for-V&V | Fixtures A–C check arithmetic means, empty features, a single-cell feature and 3D Z indexing at margin `1e-4`. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Class 1/4 - Periodic Boundary` | new-for-V&V | Fixtures D–F check X = 3.5 and X = 11 for wrapped features, an unchanged non-spanning feature and the domain-filling fallback. D and E replace the constant-offset expectations (`ComputeFeatureCentroidsFilter-D4`). |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Error - FeatureId exceeds Feature AM` | new-for-V&V | Supplies ID 5 with two feature tuples and requires an invalid execution result. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: Bulk boundary and failure propagation` | kept | Checks physical centroids across 65,536 cells plus a tail; checks validation-read error `-91911` and an unchanged zero output. |
| `SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility` | kept | Dynamic variants `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` check one filter, its UUID, empty comments and three converted argument values. |

## Exemplar archive

- **Archive:** None — inputs and analytical expectations are defined in the test source.
- **SHA512:** Not applicable; these tests consume no exemplar archive from `src/Plugins/SimplnxCore/test/CMakeLists.txt`.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeFeatureCentroidsFilter.md`

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeFeatureCentroidsFilter.md` for the source comparison, affected users and recommendations. Only Part 1 entries appear here.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeFeatureCentroidsFilter-D1` | SIMPLNX computes cell-center coordinates in float64; DREAM.3D v6.5.171 rounds them to float32 before accumulation. |
| `ComputeFeatureCentroidsFilter-D2` | SIMPLNX adds an optional periodic centroid calculation. DREAM.3D v6.5.171 has no periodic mode. |
