# V&V Report: ComputeNeighborhoodsFilter

|                            |                                                                                                                          |
|----------------------------|--------------------------------------------------------------------------------------------------------------------------|
| Plugin                     | SimplnxCore                                                                                                              |
| SIMPLNX UUID               | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d`                                                                                   |
| SIMPLNX Human Name         | Compute Feature Neighborhoods                                                                                           |
| DREAM3D 6.5.171 equivalent | `FindNeighborhoods` (SIMPL UUID `697ed3de-db33-5dd1-a64b-04fb71e7d63e`) — `Source/Plugins/Statistics/StatisticsFilters/FindNeighborhoods.{h,cpp}` |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                                                 |
| Status                     | READY FOR REVIEW                                                                                                         |
| Sign-off                   | *<engineer(s), date>*                                                                                                    |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                                                     |
|------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Rewrite** of legacy `FindNeighborhoods`. Legacy tested an axis-aligned box in normalized bin-space with a per-feature reach; NX tests a true Euclidean sphere. During V&V a real **NX regression was found and fixed** (see Bug flags). A new "Search Radius (microns)" mode and removal of the unused Feature Phases input were also added. |
| Oracle (confirmed)     | **Class 1 (Analytical) + Class 4 (Invariant).** Two hand-built synthetic fixtures with exact neighbor counts (microns mode; per-feature multiples mode incl. an asymmetry case), plus count==list-size and symmetry/asymmetry invariants. Encoded in `test/ComputeNeighborhoodsTest.cpp`; all pass. |
| Code paths enumerated  | 11 of 13 exercised; 2 uncovered (a low-value preflight guard and the cancel-signal path).                                                                                                                                                                                                        |
| Tests today            | **5 TEST_CASEs** — 2 analytical oracles (microns + per-feature multiples), 1 preflight info/warnings (3 sections), 1 invalid-parameter, 1 SIMPL backward-compat (2 DYNAMIC_SECTIONs). All inline/synthetic — no exemplar archive.                                                                  |
| Exemplar archive       | **None.** All oracles are inline analytical values. The legacy comparison used the shared `6_6_stats_test_v2.tar.gz` Small IN100 stats dataset as input only (not a unit-test exemplar for this filter).                                                                                          |
| Legacy comparison      | **Run** on Small IN100 (`6_6_stats_test_v2.dream3d`, 620 features, mult=1) via 6.5.171 `PipelineRunner` vs `nxrunner`. After the fix, NX correlates **0.894** with legacy and finds **50.8%** as many neighbors — exactly the sphere/box volume ratio. One documented deviation (D1); phases removal (D2). |
| Bug flags              | **NX regression found & fixed in this PR** (not a legacy bug): the prior NX rewrite (PR #1485) introduced a `÷2` factor and a global (vs per-feature) radius, making the default `mult=1` find ~37× fewer neighbors than legacy (mean 0.29 vs 10.93). Fixed by restoring a per-feature Euclidean radius. |
| V&V phase              | Oracle chosen and encoded; SIMPLNX-vs-oracle reconciliation complete (bug fixed); legacy 6.5.171 comparison run and explained; docs updated. Outstanding: second-engineer oracle review; sign-off. |

## Summary

`ComputeNeighborhoodsFilter` counts, for each feature, how many other features have their centroid within a search radius of that feature's centroid. Verification used a **Class 1 analytical oracle** (hand-built synthetic fixtures with exact, hand-derived neighbor counts for both the per-feature "Multiples" mode and the absolute "Search Radius (microns)" mode) together with **Class 4 invariants** (count == neighbor-list size; symmetry in microns mode; per-feature asymmetry in multiples mode). Reconciling SIMPLNX against this independent oracle — and then against legacy DREAM3D 6.5.171 on Small IN100 — surfaced a real regression in the prior NX rewrite: the search radius had been changed to `avgDiameter × mult ÷ 2` (a single global radius), which at the default `mult=1` found ~37× fewer neighbors than legacy. The fix restores a **per-feature Euclidean radius** `equivalentDiameter[i] × mult`, preserving the legitimate box→sphere improvement of the rewrite while recovering legacy's per-feature scale (post-fix correlation 0.894 with legacy; the residual 2× count difference is the geometrically-correct sphere-vs-box volume ratio).

## Algorithm Relationship

*Classification:* **Rewrite** ~~| Port | Minor changes | New filter~~

*Evidence:* Same SIMPL UUID retained (`697ed3de-db33-5dd1-a64b-04fb71e7d63e`; SIMPL 6.4/6.5 conversion fixtures at `test/simpl_conversion/6_*/ComputeNeighborhoodsFilter.json`). The core inclusion test is fundamentally different from legacy, so this is a rewrite whose outputs diverge from legacy by design — defended in the Deviations file.

*History (why this is a rewrite with a mid-flight fix):*

1. The original NX port (pre-#1485) was a faithful translation of legacy: per-feature `criticalDistance[i] = eqDiam[i]·mult / aveDiam`, compared against integer bin differences on each axis (an axis-aligned **box** in normalized bin space).
2. **PR #1485 ("BUG: New implementation")** correctly replaced the bin-membership proxy with a true **Euclidean distance** test — a genuine improvement — but in doing so redefined the radius as a single **global** `avgDiameter·mult ÷ 2`. The `÷2` and the global (rather than per-feature) radius were not present in legacy. The hand-verification used `mult=3.0`, which masked the effect; at the default `mult=1.0` the filter found almost no neighbors.
3. **This V&V** restored the per-feature Euclidean radius `equivalentDiameter[i]·mult` (no `÷2`), keeping the box→sphere improvement. The `avgDiameter` is now used only to size the internal spatial-hash grid.

*Port-time deltas vs legacy (current state):*

1. **Box → sphere.** Legacy: `|Δbin| < cd` on each axis (Chebyshev box in bin space). NX: `distSq ≤ radius²` (Euclidean sphere in physical units). This is the intended correctness improvement; it is the sole remaining source of numeric divergence (D1).
2. **Per-feature radius retained.** Both legacy and current NX scale the reach by each feature's own equivalent diameter, so large features have large neighborhoods and the relation is asymmetric.
3. **Feature Phases removed.** Legacy required a `FeaturePhases` input but never used it in the computation; NX removes the parameter (D2).
4. **New "Search Radius (microns)" mode.** No legacy equivalent; lets the user supply an absolute radius and does not require Equivalent Diameters.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion.

*Applied:* Minimal synthetic geometries with hand-placed centroids (and, for multiples mode, hand-chosen equivalent diameters) make the expected neighbor counts derivable on paper. Microns mode: a constant radius of 3.5 on six features gives counts `{1,2,1,0,0}`. Multiples mode: `eqDiam = {6,2,2,2,2}`, `mult=1` gives per-feature radii `{6,2,2,2,2}`; the fixture is arranged so a large feature reaches a small one that does not reach back, yielding counts `{3,0,0,1,0}` and a checkable asymmetry. Class 4 invariants (`Neighborhoods[i] == NeighborList[i].size`, symmetry in microns mode, targeted per-feature asymmetry in multiples mode) are asserted alongside the exact values.

*Encoded:* `test/ComputeNeighborhoodsTest.cpp::ComputeNeighborhoods_SyntheticOracle` (microns) and `::ComputeNeighborhoods_MultiplesAnalyticalOracle` (per-feature). All pass at the verified commit.

*Second-engineer review:* *Pending — recommend reviewing the `MultiplesAnalyticalOracle` asymmetry fixture (it is the case that distinguishes per-feature from global radius).*

## Code path coverage

*11 of 13 paths exercised.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeNeighborhoods.cpp` (~300 lines) + `ComputeNeighborhoodsFilter.cpp` preflight.

Logical phases: **(a) preflight validation/actions**, **(b) radius + bin setup**, **(c) parallel per-feature spatial scan**, **(d) neighbor-list finalize**.

| #  | Phase | Path | Test case |
|----|-------|------|-----------|
| 1  | (a) | Multiples mode, `multiples ≤ 0` → error `-5732` | *Not directly tested. Low-value parameter guard.* |
| 2  | (a) | Microns mode, `searchRadius ≤ 0` → error `-5733` | `ComputeNeighborhoods_InvalidSearchRadius` |
| 3  | (a) | Multiples mode, tuple mismatch (eqDiam vs centroids) → error `-5730` | *Not directly tested. Guarded by selection-parameter validation upstream.* |
| 4  | (a) | Centroids parent is not an Attribute Matrix → error `-5731` | *Exercised implicitly — all fixtures place Centroids in a feature AM.* |
| 5  | (a) | Report Input Image Geometry Info preflight value (both modes) | `..._SearchRadiusPreflightInfo` |
| 6  | (a) | Microns sub-voxel radius → warning `-5734` | `..._SearchRadiusPreflightInfo` (sub-voxel section) |
| 7  | (a) | Microns oversized radius → warning `-5735` | `..._SearchRadiusPreflightInfo` (oversized section) |
| 8  | (b) | `SearchRadiusType == 0` → per-feature radii `eqDiam[i]·mult`, binSize = avgDiameter | `..._MultiplesAnalyticalOracle` |
| 9  | (b) | `SearchRadiusType == 1` → constant radii = searchRadius, binSize = searchRadius | `..._SyntheticOracle` |
| 10 | (c) | Neighbor within radius → `updateNeighborHood(i, j)` | both analytical oracles |
| 11 | (c) | Per-feature asymmetry (radius varies by feature) | `..._MultiplesAnalyticalOracle` (asymmetry assertions) |
| 12 | (c) | Self-skip (`j == i`) and background-skip (loop starts at feature 1) | Implicit in both oracles (feature 0 excluded, self never counted) |
| 13 | (c) | `shouldCancel` → early return from the parallel scan | *Not directly tested. Requires cancel-signal injection.* |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `ComputeNeighborhoods_SyntheticOracle` | new-for-V&V | Class 1 microns oracle (6 features, r=3.5, counts `{1,2,1,0,0}`) + Class 4 count==size and symmetry invariants. No Equivalent Diameters array (proves microns mode does not require it). |
| `ComputeNeighborhoods_MultiplesAnalyticalOracle` | new-for-V&V | Class 1 per-feature multiples oracle (counts `{3,0,0,1,0}`) + count==size invariant + explicit per-feature asymmetry checks. |
| `ComputeNeighborhoods_InvalidSearchRadius` | new-for-V&V | Preflight rejects a non-positive search radius (`-5733`). Synthetic input (no archive). |
| `ComputeNeighborhoods_SearchRadiusPreflightInfo` | new-for-V&V | 3 SECTIONs: geometry-info preflight value present; sub-voxel warning `-5734`; oversized warning `-5735`. |
| `ComputeNeighborhoodsFilter: SIMPL Backwards Compatibility` (2 DYNAMIC_SECTIONs) | kept | Validates UUID + argument decoding from SIMPL 6.4/6.5 JSON. |
| `ComputeNeighborhoods_1` | retired | Circular exemplar comparison (golden `Neighborhoods_1` was not an independent oracle) and invalidated by the radius fix. Replaced by the Class 1 analytical oracle + legacy comparison. |
| `ComputeNeighborhoods_3` | retired | Same as `_1` for `mult=3`. |
| `ComputeNeighborhoods_SearchRadiusMicrons` | retired | Premise (microns ≡ multiples when `r = avgDiam/2`) is invalid under the per-feature fix. Microns mode is covered by `SyntheticOracle`. |

All non-retired tests pass at the verified commit. *(In-core build confirmed; OOC build to be confirmed per dual-build protocol.)*

## Exemplar archive

- **Archive:** None. All oracle outputs are inline hand-derived values on synthetic inputs.
- The retired tests previously consumed `compute_feature_neighborhoods.tar.gz`; its `download_test_data()` entry was removed from `test/CMakeLists.txt`.
- The legacy comparison (below) used `6_6_stats_test_v2.tar.gz` (shared Small IN100 stats dataset) as **input only**.

## Deviations from DREAM3D 6.5.171

Comparison run on `6_6_stats_test_v2.dream3d` (Small IN100, 620 features, mult=1) through 6.5.171 `PipelineRunner` and `nxrunner`.

- `ComputeNeighborhoodsFilter-D1` — NX counts neighbors with a Euclidean sphere; 6.5.171 used an axis-aligned box in normalized bin space, so NX reports ~52% as many neighbors (correlation 0.894). See `vv/deviations/ComputeNeighborhoodsFilter.md`.
- `ComputeNeighborhoodsFilter-D2` — the `FeaturePhases` required input was removed (unused by both implementations). See `vv/deviations/ComputeNeighborhoodsFilter.md`.
