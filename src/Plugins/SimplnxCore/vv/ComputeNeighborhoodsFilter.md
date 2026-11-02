# V&V Report: ComputeNeighborhoodsFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| SIMPLNX Human Name | Compute Feature Neighborhoods |
| DREAM.3D v6.5.171 equivalent | `FindNeighborhoods` (Statistics plugin, SIMPL UUID `697ed3de-db33-5dd1-a64b-04fb71e7d63e`) |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-07-02; second engineer: Nathan Young, 2026-07-10 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Rewrite of `FindNeighborhoods` in DREAM.3D v6.5.171. SIMPLNX uses a Euclidean sphere with a per-feature or absolute radius instead of a box in integer bin coordinates. |
| Oracle (confirmed) | Class 1 counts and Class 4 invariants use two fixtures in `test/ComputeNeighborhoodsTest.cpp`. All pass in CI. |
| Code paths enumerated | 16 of 21 exercised by the five ordinary test cases. Gaps cover cancellation, empty input, background-only input, and external list storage. |
| Tests today | Six test cases: two analytical, two preflight, one SIMPL conversion, and one hidden HDF5 fixture. The five ordinary cases cover both radius modes and input validation. |
| Exemplar archive | None. Tests construct synthetic inputs and use inline analytical expectations. |
| Legacy comparison | Run against DREAM.3D v6.5.171 on 2026-10-22: 1 of 3 cases match; the differences come from ComputeNeighborhoodsFilter-D1. A local proof version with those fixes matches DREAM3D-NX in 3 of 3 cases, using constant legacy diameters for the DREAM3D-NX-only absolute-radius case. |
| Bug flags | None |
| V&V phase | Verification records, second-engineer review and the legacy comparison are complete. |

## Summary

`ComputeNeighborhoodsFilter` counts and lists feature centroids within each feature's search radius. Class 1 fixtures check exact counts, and Class 4 invariants check list sizes and symmetric or asymmetric membership. The V&V fixed the global-radius regression and background-feature processing; DREAM3D-NX v7.4.2 contains both fixes.

## Algorithm Relationship

*Classification:* Rewrite

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps `FindNeighborhoods` to this filter; the legacy source compares integer bin differences, while `ComputeNeighborhoods.cpp` compares squared Euclidean distance with each feature's squared radius.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant) companions.

*Applied:* Hand-placed centroids give counts `{0,1,2,1,1,0,1}` at radius 3.5 and `{0,3,0,0,1,0}` with per-feature diameters; list sizes, background exclusion, boundary inclusion, and symmetric or asymmetric membership are asserted.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeNeighborhoodsTest.cpp::SimplnxCore::ComputeNeighborhoods_SyntheticOracle` and `SimplnxCore::ComputeNeighborhoods_MultiplesAnalyticalOracle` — two fixtures. All pass in CI.

*Second-engineer review:* Nathan Young, 2026-07-10.

## Bugs found and fixed

All defects in this table are fixed. DREAM3D-NX v7.4.2 contains the fixes for `ComputeNeighborhoodsFilter-D4` and `ComputeNeighborhoodsFilter-D5`.

| Deviation | Defect | Affected released versions | Resolution |
|-----------|--------|----------------------------|------------|
| `ComputeNeighborhoodsFilter-D4` | A single radius of `averageDiameter * multiplier / 2` replaced each feature's own radius and changed neighbor membership. | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. | Multiples mode uses `equivalentDiameter[i] * multiplier`; the average diameter sizes the search bins only. |
| `ComputeNeighborhoodsFilter-D5` | Background feature 0 could search for neighbors and obtain a nonzero count, while its output list stayed empty. | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. | Search sources and candidates exclude feature 0; its count is zero and its list is empty. |

## Code path coverage

16 of 21 paths exercised. Each uncovered row states the required fixture or signal.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeNeighborhoods.cpp` (322 lines) and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeNeighborhoodsFilter.cpp` (269 lines).

Test names below omit the `SimplnxCore::` prefix. The two analytical cases are `ComputeNeighborhoods_SyntheticOracle` and `ComputeNeighborhoods_MultiplesAnalyticalOracle`.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Preflight | Multiples mode, multiplier ≤ 0 → error `-5732` | `ComputeNeighborhoods_InvalidSearchRadius` — zero multiplier |
| 2 | Preflight | Microns mode, radius ≤ 0 → error `-5733` | `ComputeNeighborhoods_InvalidSearchRadius` — zero radius |
| 3 | Preflight | Diameter and centroid tuple counts differ → error `-5730` | `ComputeNeighborhoods_InvalidSearchRadius` — tuple mismatch |
| 4 | Preflight | Centroids parent is not an Attribute Matrix → error `-5731` | `ComputeNeighborhoods_InvalidSearchRadius` — loose centroids |
| 5 | Preflight | Report geometry information; accept a radius without warnings | `ComputeNeighborhoods_SearchRadiusPreflightInfo` — reasonable radius |
| 6 | Preflight | Microns radius is below the minimum voxel spacing → warning `-5734` | `ComputeNeighborhoods_SearchRadiusPreflightInfo` — sub-voxel radius |
| 7 | Preflight | Microns radius exceeds the largest geometry extent → warning `-5735` | `ComputeNeighborhoods_SearchRadiusPreflightInfo` — oversized radius |
| 8 | Setup | Multiples mode computes per-feature radii and averages non-background diameters for bin size | `ComputeNeighborhoods_MultiplesAnalyticalOracle` |
| 9 | Setup | Microns mode uses one radius and does not require Equivalent Diameters | `ComputeNeighborhoods_SyntheticOracle` |
| 10 | Search | A candidate lies within the radius → increment count and append its ID | Both analytical cases |
| 11 | Search | Different per-feature radii permit asymmetric membership | `ComputeNeighborhoods_MultiplesAnalyticalOracle` — feature 1 lists feature 2, but feature 2 does not list feature 1 |
| 12 | Search | Skip self and exclude background as a source and candidate | Both analytical cases — exact counts include background; feature 0 coincides with feature 1 |
| 13 | Search | Cancel during the per-feature scan → return | *Not directly tested. Requires cancel-signal injection during the scan.* |
| 14 | Search | Squared distance equals squared radius → include the candidate | `ComputeNeighborhoods_SyntheticOracle` — features 4 and 6 are exactly 3.5 apart |
| 15 | Storage | External NeighborList output crosses a three-tuple chunk boundary | *Not directly tested. Requires the hidden `ComputeNeighborhoods: real HDF5 list-chunk oracle` fixture.* |
| 16 | Setup | No feature tuples → return before radius setup | *Not directly tested. No fixture has zero feature tuples.* |
| 17 | Setup | Only background exists → skip averaging and all feature searches | *Not directly tested. No fixture contains only feature 0.* |
| 18 | Setup | Cancel after bin construction → return before searching | *Not directly tested. Requires cancel-signal injection before dispatch.* |
| 19 | Search | Bin has no candidate features → continue | Both analytical cases — sparse centroid layouts |
| 20 | Search | Candidate is outside the feature's radius → exclude it | `ComputeNeighborhoods_MultiplesAnalyticalOracle` — feature 2 excludes feature 1 |
| 21 | Finalize | Write every neighbor list, including empty lists and background | Both analytical cases — count/list-size checks at every index |

## Test inventory

The five ordinary cases use analytical inputs or conversion JSON. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeNeighborhoods_SyntheticOracle` | new-for-V&V | Seven features; radius 3.5; exact counts `{0,1,2,1,1,0,1}`. Checks list sizes, symmetry, inclusive boundary, background exclusion, and operation without Equivalent Diameters. |
| `SimplnxCore::ComputeNeighborhoods_MultiplesAnalyticalOracle` | new-for-V&V | Six features; exact counts `{0,3,0,0,1,0}`. Checks list sizes, asymmetric and symmetric pairs, and background exclusion; covers D4 and D5. |
| `SimplnxCore::ComputeNeighborhoods_InvalidSearchRadius` | new-for-V&V | Four sections assert errors `-5733`, `-5732`, `-5730`, and `-5731` for zero radius, zero multiplier, tuple mismatch, and centroids outside an Attribute Matrix. |
| `SimplnxCore::ComputeNeighborhoods_SearchRadiusPreflightInfo` | new-for-V&V | Three sections check geometry information, no warning for radius 5, warning `-5734` for radius 0.5, and warning `-5735` for radius 1000. |
| `SimplnxCore::ComputeNeighborhoods: real HDF5 list-chunk oracle` | new-for-V&V | Hidden fixture with nine features and tuple shape `{3,1,3}`. Both radius modes assert exact directed lists, counts, storage types, and tuple dimensions across the features 2/3 chunk boundary. |
| `SimplnxCore::ComputeNeighborhoodsFilter: SIMPL Backwards Compatibility` | kept | `SIMPL 6.5 (UUID)` and `SIMPL 6.4 (Filter_Name)` sections check filter identity, empty comments, and six converted argument values. |

## Exemplar archive

- **Archive:** None. All current correctness fixtures use synthetic inputs and inline expected values.
- **SHA512:** Not applicable. `src/Plugins/SimplnxCore/test/CMakeLists.txt` has no archive registration for this filter.
- **Provenance:** Not applicable. Fixture construction and analytical derivations are in `src/Plugins/SimplnxCore/test/ComputeNeighborhoodsTest.cpp`.

## Deviations from DREAM.3D v6.5.171

See `vv/deviations/ComputeNeighborhoodsFilter.md` for the source evidence, affected users, and recommendation. Only Part 1 entries are listed here.

| Deviation | Observed difference |
|-----------|---------------------|
| `ComputeNeighborhoodsFilter-D1` | SIMPLNX uses an inclusive Euclidean radius; DREAM.3D v6.5.171 uses strict comparisons in integer bin coordinates. SIMPLNX also offers an absolute-radius mode. |
| `ComputeNeighborhoodsFilter-D2` | SIMPLNX removes the unused Feature Phases input. |
| `ComputeNeighborhoodsFilter-D3` | SIMPLNX rejects non-positive multipliers; DREAM.3D v6.5.171 accepts them. The absolute-radius mode also rejects non-positive radii. |
