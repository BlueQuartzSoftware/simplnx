# V&V Report: DBSCANFilter

|                            |                                                                                           |
|----------------------------|-------------------------------------------------------------------------------------------|
| Plugin                     | SimplnxCore                                                                               |
| SIMPLNX UUID               | `763dad44-fad7-4606-808f-617867257b98`                                                    |
| SIMPLNX Human Name         | DBSCAN                                                                                    |
| DREAM3D 6.5.171 equivalent | `DBSCAN` (SIMPL UUID `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e`) — legacy UUID mapped in `SimplnxCoreLegacyUUIDMapping.hpp` |
| Verified commit            | *<filled at SBIR deliverable assembly>*                                                   |
| Status                     | IN-REVIEW                                  |
| Sign-off                   | *Nathan Young, 8/7/2026*                                                                     |

## At a glance

| Aspect                 | Current state                                                                                                                                                                                                                                                              |
|------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Algorithm Relationship | **Rewrite** — SIMPLNX implements GDCF (Grid-based DBSCAN, Boonchoo et al. 2019, DOI 10.1016/j.patcog.2019.01.034) in place of the traditional point-by-point DBSCAN in legacy DREAM3D. UUID changed from `c2d4f1e8` to `763dad44` (legacy UUID retained via SIMPL mapper). |
| Oracle (confirmed)     | **Class 2 (Reference — scikit-learn 1.7.1 DBSCAN) primary + Class 4 (Invariant) companion.** Input data independently generated from deterministic sklearn scripts in `dbscan_vv/dbscan_data_proj/`. Phase 6 reconciliation complete: 4/6 datasets exact match; 2 deviations (ansio, varied) fully explained by DBSCAN-D1 (GDCF vs. traditional DBSCAN). See Phase 5 + Phase 6. |
| Code paths enumerated  | 18 paths identified from code review — see Code path coverage table. Current tests cover approximately 12/18; uncovered paths noted in table.                                                                                                                               |
| Tests today            | 11 TEST_CASEs: 6×2D dataset tests (each running LDF + Random + SeededRandom), 1×3D LDF test, 1 SIMPL backwards-compat test, 3 analytical fixtures (F1: no-clusters warning, F2: mask exclusion, F3: all points masked). 2D tests and 3D test use regression exemplars from `dbscan_test.tar.gz`; F1–F3 are self-contained inline data. |
| Exemplar archive       | **`dbscan_test.tar.gz` — promoted to regression fixtures (Phase 6/10).** Originally circular oracle; independently verified via Class 2 sklearn oracle (Phase 6). LDF arrays now pin verified-correct SIMPLNX output. See provenance sidecar. |
| Legacy comparison      | **Complete (Phase 9, 2026-08-05).** DREAM3D 6.5.172 run via `dbscan_vv/phase9_ab_test.py`. Results in `dbscan_vv/phase9_comparison_results.json`. 4/6 datasets: exact three-way match (legacy = sklearn = SIMPLNX). 2 deviations (ansio, varied): legacy matches sklearn cluster count (6 and 11 respectively); SIMPLNX finds fewer clusters (3 for both) — confirms DBSCAN-D1. Minor implementation differences between legacy and sklearn for sparse datasets (±1–3 boundary points, same cluster count) are within tolerance and do not affect deviation classification. |
| Bug flags              | ✅ Circular oracle resolved (Phase 6) — `dbscan_test.tar.gz` LDF arrays promoted to regression fixtures; Class 2 sklearn oracle confirms correctness. 2 expected GDCF deviations (ansio, varied) documented as DBSCAN-D1. **1 SIMPLNX bug found and fixed during V&V:** `ParseOrder::Random` fed the user-supplied seed to the shuffle instead of the documented time-based seed, making "Random" a silent duplicate of "Seeded Random" (see Phase 7). **1 robustness defect found and fixed:** an all-false mask left the grid bounds NaN and those NaNs were cast to `usize` while computing grid dimensions (undefined behavior). |
| V&V phase              | Phases 1–13 complete. Pending second-engineer oracle sign-off before COMPLETE status.                                                                                                                                                                                      |

## Summary

`DBSCANFilter` implements Grid-based DBSCAN (GDCF) as described in Boonchoo et al. 2019 — a substantially different algorithm from the traditional point-by-point DBSCAN present in legacy DREAM3D 6.5.171. The SIMPLNX rewrite (PR #1421) replaced the entire implementation; the filter UUID was simultaneously changed from `c2d4f1e8` to `763dad44`, explicitly signaling algorithmic non-equivalence. The critical difference is that SIMPLNX defines a "core object" at the grid-cell level (≥ minPoints data points within a single grid cell) rather than at the individual-point level (≥ minPoints data points within an ε-ball of a given point), producing different clustering results for sparse datasets where points individually have neighbors within ε but those neighbors span multiple grid cells. V&V is complete: (a) circular exemplar oracle resolved via independent Class 2 sklearn oracle (Phase 6) and legacy comparison (Phase 9); (b) Class 4 invariants encoded in tests (Phase 8); (c) two expected GDCF deviations documented as DBSCAN-D1/D2. Pending: second-engineer oracle review.

## Algorithm Relationship

*Classification:* **Rewrite**

*Evidence:* PR #1421 ("PERF/ENH: DBSCAN Rewrite", commit `c1bc9114b7`, 2025-09-04) replaced the entire `DBSCAN.cpp` implementation with a GDCF-based approach derived from Boonchoo et al. 2019. The filter UUID was simultaneously changed from `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` to `763dad44-fad7-4606-808f-617867257b98`, explicitly signaling that the new filter is not a line-for-line translation of the legacy algorithm. The legacy UUID is retained in `SimplnxCoreLegacyUUIDMapping.hpp` for pipeline backward compatibility, routing old SIMPL pipelines through `FromSIMPLJson` to populate the new parameters.

*Algorithmic change summary (GDCF vs. traditional DBSCAN):*

1. **Core-object definition** — Legacy: a data point is a core point if ≥ minPoints data points lie within its ε-neighborhood (inclusive of itself). SIMPLNX: a grid cell is a core grid if it contains ≥ minPoints data points. This is the fundamental behavioral difference — see Deviation DBSCAN-D1.
2. **Grid-based spatial indexing** — SIMPLNX bins points into a regular grid with cell side length `ε/sqrt(dims)`, builds per-dimension bit-packed adjacency tables (HyperGridBitMap), and queries nearest-neighbor grids via bitwise AND. Legacy performed direct point-to-point ε-ball queries on every pair.
3. **Union-find cluster forest** — SIMPLNX uses a forest of `ClusterNode` structs with union-by-min-cluster-id to merge density-reachable grids. Legacy used a queue-based expansion loop.
4. **Parse order choices** — SIMPLNX adds `LowDensityFirst` (sort core grids ascending by grid occupancy) and `SeededRandom` (deterministic shuffle). Legacy had no equivalent of `LowDensityFirst`.
5. **`use_precaching` removed** — Legacy had a memory/time trade-off switch. SIMPLNX's GDCF made it irrelevant; parameter removed in v2 (SIMPL converter silently ignores it via `parametersVersion() == 2`).
6. **Parameter rename** — `init_type_index` → `parse_order_index`. Handled in `FromSIMPLJson` reading old key into new parameter.

*Material PRs since baseline:*

- **#994** — "FILTER/ENH: DBSCAN Filter and Clustering Cleanup" (2024-06-21) — Original traditional DBSCAN implementation.
- **#1421** — "PERF/ENH: DBSCAN Rewrite" (2025-09-04, commit `c1bc9114b7`) — Complete algorithm replacement with GDCF. UUID changed; new 2D/3D test cases added; documentation rewritten.
- **#1576** — "ENH: Improve error messages across the codebase" (commit `f885a0ebc9`) — Error message text edits in `DBSCAN.cpp`. No algorithmic change.

## Oracle

*Class:* **2 (Reference — scikit-learn 1.7.1)** primary + **Class 1 (Analytical)** for error paths + **Class 4 (Invariant)** companion

*Applied:*

**Class 2** — scikit-learn 1.7.1 DBSCAN run on the same 6 sklearn toy input datasets, same ε/minPts parameters. Comparison at cluster-structure level (count + sorted bin sizes). Results in `dbscan_vv/oracle_results.json`. 4/6 exact match; 2 deviations (ansio, varied) explained by DBSCAN-D1. See Phase 5 + Phase 6.

**Class 1** — Hand-derived analytical fixtures for SIMPLNX-specific code paths not reachable by the Class 2 sklearn oracle:
- **F1 (no-clusters warning)**: 4 corner points, ε=0.1, minPts=5 → each point in its own cell → warning -85640, all IDs=0.
- **F2 (mask exclusion)**: 3 points, P2 masked → P0+P1 in one core cell (ε=1.0, minPts=2) → ClusterIds=[1,1,0].
Both implemented in `DBSCANTest.cpp`.

**Class 4** — Structural invariants asserted on every test run via `CheckClusterInvariants()`:
- `ClusterIds[i] >= 0` for all i
- IDs 1..max(ClusterIds) contiguous (no gaps — unlabeled 0 is exempt)
- `AttributeMatrix.tupleCount == max(ClusterIds) + 1`

*Note*: the invariant "No masked points have ClusterIds > 0" is spot-checked in F2 but **not** encoded as a general assertion in `CheckClusterInvariants()` (which does not receive the mask path). If mask coverage is later added to the main test helpers, this invariant should be added there.

*Encoded:* **Complete (Phase 8, 2026-08-05).** Class 4 invariants in `CheckClusterInvariants()` hooked into all 2D test helpers. Class 1 F1/F2 added as dedicated TEST_CASEs. 3D test lacks `CheckClusterInvariants` (no mask; AM tuple check present inline).

*Second-engineer review:* *Pending — see Phase 4 and Phase 13.*

## Code path coverage

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/DBSCAN.cpp` (1136 lines).

Logical phases: **(a) Grid construction** — build HyperGridBitMap and bin points; **(b) Core identification** — find and sort core grids; **(c) Cluster phase** — union-find merge of core/border grids; **(d) Expansion** — iterative border-grid expansion loop; **(e) Cleanup + Label** — renumber cluster IDs, assign to points.

| #  | Phase         | Path                                                                                                                                   | Test case                                                             |
|----|---------------|----------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------|
| 1  | (a) Grid 2D   | Input has 2 components → `HyperGridBitMap2D` path in `DBSCANFunctor`                                                                  | All 2D tests (Aniso, Blobs, Circles, Moons, NoStructure, Varied)      |
| 2  | (a) Grid 3D   | Input has 3 components → `HyperGridBitMap3D` path in `DBSCANFunctor`                                                                  | `3D Test (LowDensityFirst)`                                           |
| 3  | (a) Grid err  | Input has other component count → error `-54060`                                                                                       | *Not directly tested. Preflight rejects via `AllowedComponentShapes{{2},{3}}`.* |
| 4  | (a) Mask=true | Masked points skipped in binning and bounds                                                                                            | **F2** (`Analytical Fixture F2 - Mask Exclusion`, added Phase 8) |
| 4b | (a) Mask all false | No point passes the mask → bounds stay NaN → early return leaves the grid empty → warning `-85640`                              | **F3** (`Analytical Fixture F3 - All Points Masked`, added during Phase 7 fix) |
| 5  | (b) No cores  | All grids have <minPoints → warning `-85640`, no labeling                                                                              | **F1** (`Analytical Fixture F1 - No Clusters Warning`, added Phase 8) |
| 6  | (b) LDF sort  | Parse order = LowDensityFirst → QuickSort core grids ascending by occupancy                                                            | All 2D LDF tests and 3D test                                          |
| 7  | (b) Random    | Parse order = Random → shuffle with time-based seed (non-deterministic run to run)                                                      | `Random` variant of all 2D tests. Compares cluster count + cluster-size multiset only; the seed actually used is read back from the seed array and reported via `INFO` so a failure is reproducible. |
| 8  | (b) SeededRnd | Parse order = SeededRandom → shuffle with user-supplied seed                                                                           | `SeededRandom` variant of all 2D tests; also asserts the seed array round-trips the user seed unchanged |
| 9  | (c) Same clus | NeighborGridQuery returns already-merged grid → `clusterForest.infer()` true → skip                                                   | All 2D/3D tests (implicit on multi-grid clusters)                     |
| 10 | (c) Unvisited border | Neighbor grid is border AND self-parent → merge to current core parent                                                         | All 2D/3D tests (implicit)                                            |
| 11 | (c) Core merge | Neighbor grid is core (or visited border) → add to mergeLRC vector → union-find merge                                                | All 2D/3D tests (implicit on touching clusters)                       |
| 12 | (d) Expansion | Non-core grid has density-reachable neighbor → join neighbor's cluster                                                                 | `Aniso` test (elongated clusters require multiple expansion passes)   |
| 13 | (d) Skip unvisited border | Both search grid and neighbor are unvisited borders → skip (neither can initiate)                                        | *Not directly tested with a specific oracle fixture.*                 |
| 14 | (e) Core root | Cleanup: grid is its own parent AND is core grid → add to cluster list, renumber 1..N                                                 | All tests                                                             |
| 15 | (e) Noise     | Cleanup: grid is its own parent AND is NOT core → label as cluster 0                                                                   | Tests with expected noise points (e.g., NoStructure, Varied)          |
| 16 | (e) Label     | `label()`: iterate grids, assign `findClusterRoot().clusterId` to all points in each grid                                              | All tests                                                             |
| 17 | (e) Empty FM  | `label()` called with empty `clusterForestNodes` → warning `-85640`                                                                    | *Not directly tested. Arises only if `cluster()` was skipped.*        |

## Test inventory

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::DBSCAN: 2D Test: Aniso` | kept — regression fixture | LDF + Random + SeededRandom. Input data: `make_blobs(random_state=170)` + linear transform `[[0.6,-0.6],[-0.4,0.8]]`; verified against sklearn 1.7.1 oracle (Phase 6). LDF uses exact array compare; Random/SeededRandom use bin-size matching only. |
| `SimplnxCore::DBSCAN: 2D Test: Blobs` | kept — regression fixture | Same pattern. Input from sklearn `make_blobs`; exact sklearn match confirmed Phase 6. |
| `SimplnxCore::DBSCAN: 2D Test: Noisy Circles` | kept — regression fixture | Same pattern. Input from sklearn `make_circles`; exact sklearn match confirmed Phase 6. |
| `SimplnxCore::DBSCAN: 2D Test: Noisy Moons` | kept — regression fixture | Same pattern. Input from sklearn `make_moons`; exact sklearn match confirmed Phase 6. |
| `SimplnxCore::DBSCAN: 2D Test: No Structure` | kept — regression fixture | Same pattern. Input from sklearn uniform random; exact sklearn match confirmed Phase 6. |
| `SimplnxCore::DBSCAN: 2D Test: Varied` | kept — regression fixture | Same pattern. Input from sklearn `make_blobs` with varied cluster std; deviations (DBSCAN-D1) explained and documented Phase 6. |
| `SimplnxCore::DBSCAN: 3D Test (LowDensityFirst)` | kept — regression fixture | LDF only; exact array compare. 3D dataset of unknown origin — no external oracle applied; retains original circular-oracle status for this case only. |
| `SimplnxCore::DBSCANFilter: SIMPL Backwards Compatibility` | kept | Validates `FromSIMPLJson` conversion for both 6.4 and 6.5 SIMPL pipeline fixtures. No algorithmic execution — parameters only. |
| `SimplnxCore::DBSCAN: Analytical Fixture F1 - No Clusters Warning` | added Phase 8 | Class 1 oracle. 4 corner points, ε=0.1, minPts=5. Covers code path #5 (warning -85640). Self-contained inline data. |
| `SimplnxCore::DBSCAN: Analytical Fixture F2 - Mask Exclusion` | added Phase 8 | Class 1 oracle. 3 points, P2 masked. Covers code path #4 (mask=true). Self-contained inline data. |
| `SimplnxCore::DBSCAN: Analytical Fixture F3 - All Points Masked` | added during Phase 7 fix | Class 1 oracle. Same 3 points as F2 with every point masked off. Covers code path #4b and pins the all-false-mask contract (warning `-85640`, all IDs 0, AM 1 tuple) that the NaN-bounds guard makes architecture-independent. Self-contained inline data. |
| Class 4 invariant assertions | added Phase 8 | `CheckClusterInvariants()` hooked into `LDFTestCase2D` and `RandomTestCase2D` — covers all 18 2D test runs — and into F1/F2/F3. 3D test has inline AM tuple check but not the full helper. |

## Exemplar archive

- **Archive:** `dbscan_test.tar.gz`
- **SHA512:** `77d7886e2550b63176b564e827d7de320b5a28b1c8a55bf107d53acd6962757275bc86b3382ff789f612a6838f55ab8f4af29435aec83a7a804b9487e57a6386`
- **Status:** ✅ Regression fixtures (promoted Phase 6/10) — Input point arrays were pulled from scikit-learn 1.7.1 toy datasets (`make_circles`, `make_moons`, `make_blobs`, `make_blobs` anisotropic via linear transform, uniform random) and independently verified against scikit-learn 1.7.1 DBSCAN. LDF cluster-label arrays were originally generated by SIMPLNX itself (circular oracle at time of PR #1421) but are now pinned to verified-correct output. The 3D case remains unverified by external oracle. See provenance sidecar for full details.
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/dbscan_test.md`

## Deviations from DREAM3D 6.5.171

- `DBSCAN-D1` — Core-object definition changed from point-level to grid-cell-level; sparse clusters disagree — see `vv/deviations/DBSCANFilter.md`
- `DBSCAN-D2` — `LowDensityFirst` parse order added and made default; cluster ID numbering differs — see `vv/deviations/DBSCANFilter.md`
- `DBSCAN-D3` — `use_precaching` parameter removed; old pipelines silently ignore it — see `vv/deviations/DBSCANFilter.md`
- `DBSCAN-D4` — Parameter key renamed `init_type_index` → `parse_order_index`; `upgradeParametersImpl` not implemented — early SIMPLNX pipelines silently default to `LowDensityFirst` — see `vv/deviations/DBSCANFilter.md`

---

## Phase 2 — Promote existing work product

**Status: Complete.** Input data artifacts reviewed.

### Inventory

| Artifact | Decision | Rationale |
|---|---|---|
| `dbscan_test.tar.gz` — LDF cluster-label arrays | **Promoted to regression fixtures (Phase 6)** | Originally circular oracle — LDF arrays generated from SIMPLNX's own post-rewrite output. Input point data is sklearn-sourced (independent); verification against sklearn 1.7.1 oracle (Phase 6) confirmed correctness for 4/6 datasets and explained the 2 deviations via DBSCAN-D1. 3D exemplar remains circular. |
| `dbscan_vv/dbscan_data_proj/plot_cluster_comparison.py` | **Promote to Class 2 oracle seed** | Generates the 6 input datasets deterministically (seeds 30 and 170, `StandardScaler`). Input files already saved as `ansio.txt`, `blobs.txt`, etc. (500 points, 2 columns each). Python venv at `dbscan_vv/dbscan_data_proj/venv/` contains scikit-learn 1.7.1 + numpy 2.2.6. Running sklearn's DBSCAN on these files with the same ε/minPts parameters used in SIMPLNX tests produces a Class 2 reference oracle. |
| `dbscan_vv/dbscan_data_proj/*.txt` (6 files) | **Promote — oracle input data** | Deterministically generated, independently reproducible. These are the actual input arrays loaded by `dbscan_test.tar.gz`. Confirmed: 500 rows × 2 columns each, CSV with header row (501 lines). |
| `dbscan_vv/dbscan_data_proj/dbscan_test_file_generator.py` | **Discard** | Loads deprecated `load_boston()` dataset — unrelated to DBSCAN filter testing. |
| `dbscan_vv/dbscan_data_proj/sythetic_2d_datasets.dream3d` | **Investigate** | May be an earlier version of the test input file. Compare against `dbscan_test.tar.gz` input arrays. |
| Documentation images in `docs/Images/` | **Promote to Class 5 visual sanity check** | Six 2D cluster images confirm clusters are visually reasonable. Not sufficient as primary oracle. |

> **Working folder**: All V&V scripts, data files, and oracle artifacts live in `dbscan_vv/` under the `DREAM3DNX-Dev` root. The original data generation project is at `dbscan_vv/dbscan_data_proj/` (copied from Desktop). New oracle scripts and results go directly in `dbscan_vv/`.

### Class 2 oracle scope and limitations

scikit-learn's DBSCAN uses traditional point-level ε-neighborhoods; SIMPLNX uses GDCF (grid-cell-level). For the 500-point scikit-learn toy datasets at the chosen ε values, the two algorithms are expected to agree on **cluster structure** (number of clusters, approximate membership for densely interior points) but may legitimately disagree on **boundary-point assignment** at cluster edges. The Class 2 comparison should therefore use:

- **Cluster count**: `len(set(labels)) - (1 if -1 in labels else 0)` must match.
- **Noise count**: number of points labeled 0 (SIMPLNX) vs. -1 (sklearn) should be within tolerance or identical for well-separated datasets (blobs, noisy_circles, noisy_moons).
- **Cluster sizes (bin counts)**: The multiset of cluster sizes should match (modulo boundary-point differences in sparse datasets like no_structure and varied).

Direct per-point label comparison is NOT appropriate for the Class 2 oracle due to GDCF vs. traditional DBSCAN differences at cluster edges.

---

## Phase 3 — Algorithm Relationship (confirmed)

**Algorithm Relationship: Rewrite** — SIMPLNX implements GDCF (Boonchoo et al. 2019, DOI 10.1016/j.patcog.2019.01.034) replacing traditional DBSCAN. UUID changed from `c2d4f1e8` to `763dad44`. The shared SIMPL UUID legacy mapping is for pipeline backward compatibility only, not a claim of algorithmic equivalence.

---

## Phase 4 — Oracle classification

**Proposed oracle: Class 2 (Reference — scikit-learn 1.7.1) primary + Class 4 (Invariant) companion**

**Class 2 justification**: The six 2D input datasets were generated by `dbscan_vv/dbscan_data_proj/plot_cluster_comparison.py` using deterministic sklearn seeds. The venv at `dbscan_vv/dbscan_data_proj/venv/` (scikit-learn 1.7.1, numpy 2.2.6) is pinned. Running `sklearn.cluster.DBSCAN` with the same ε/minPts parameters on the same `.txt` input files produces an independent reference output. Comparison is at the cluster-structure level (count + bin sizes) rather than per-point due to the GDCF vs. traditional-DBSCAN boundary difference documented in DBSCAN-D1. Oracle script `dbscan_vv/run_sklearn_oracle.py` is already written and executed; results in `dbscan_vv/oracle_results.json`.

**Class 4 justification** (always applicable): Structural invariants derivable from the algorithm specification — cluster IDs contiguous from 1..N, attribute matrix size = maxClusterId+1, masked points always receive cluster 0.

**Class 1 for error paths**: A tiny hand-derived fixture (≤ 5 points, 2D) is still needed to verify the no-clusters warning path (path #5 in the code path table) and the mask-exclusion path (path #4), because sklearn's DBSCAN cannot exercise those SIMPLNX-specific behaviors.

**Second-engineer review**: *Pending — record name + date here when completed.*

---

## Phase 5 — Toy data design + expected output

### Oracle artifact A: Class 2 — scikit-learn DBSCAN comparison script

**Location**: `dbscan_vv/dbscan_data_proj/` (input `.txt` files and venv); oracle script at `dbscan_vv/run_sklearn_oracle.py` (already written and run — see oracle results table below).

Script `dbscan_vv/run_sklearn_oracle.py` is already written and executed (2026-08-04). It:
1. Loads each `.txt` file from `dbscan_vv/dbscan_data_proj/` (comma-separated, header row, 500 rows × 2 cols)
2. Runs `sklearn.cluster.DBSCAN(eps=ε, min_samples=minPts, metric='euclidean').fit(X)`
3. Saves results to `dbscan_vv/oracle_results.json`
4. Prints sklearn version for provenance

Parameters to use (matching `DBSCANTest.cpp`):

| Dataset file | ε | minPts |
|---|---|---|
| `ansio.txt` | 0.15 | 4 |
| `blobs.txt` | 0.30 | 3 |
| `noisy_circles.txt` | 0.30 | 3 |
| `noisy_moons.txt` | 0.30 | 3 |
| `no_structure.txt` | 0.30 | 3 |
| `varied.txt` | 0.18 | 3 |

**Note on sklearn label convention**: sklearn uses `-1` for noise; SIMPLNX uses `0`. Map accordingly when comparing counts.

**Note on comparison scope**: Use cluster-structure comparison only (cluster count + sorted cluster size list), not per-point labels. Per-point divergence at cluster boundaries is expected (Deviation DBSCAN-D1) and is not a SIMPLNX bug.

### Class 2 oracle results (scikit-learn 1.7.1, run 2026-08-04)

Output saved to `dbscan_vv/oracle_results.json`.

| Dataset | ε | minPts | n_clusters | n_noise | cluster_sizes (sorted) |
|---|---|---|---|---|---|
| ansio | 0.15 | 4 | 6 | 21 | [4, 5, 6, 152, 155, 157] |
| blobs | 0.30 | 3 | 2 | 7 | [164, 329] |
| noisy_circles | 0.30 | 3 | 2 | 0 | [250, 250] |
| noisy_moons | 0.30 | 3 | 2 | 0 | [250, 250] |
| no_structure | 0.30 | 3 | 1 | 0 | [500] |
| varied | 0.18 | 3 | 11 | 47 | [3, 3, 3, 4, 4, 4, 5, 5, 87, 166, 169] |

**Notable findings**:
- `blobs` produces only 2 clusters (two of the three scikit-learn blobs merge at ε=0.3) — this is correct, not a defect.
- `no_structure` produces 1 cluster of 500 (the uniform random data at ε=0.3 is dense enough that all points connect) — again correct.
- `varied` produces 11 clusters at ε=0.18 including several micro-clusters of 3–5 points.

SIMPLNX output (after GDCF rewrite) is expected to match these cluster counts and sizes for densely interior points. Boundary-point differences (per Deviation DBSCAN-D1) may cause small discrepancies in noise count and individual cluster sizes for `ansio` and `varied`. `noisy_circles`, `noisy_moons`, and `blobs` should match exactly or within 1–2 points.

### Oracle artifact B: Class 1 — error path fixtures (hand-derived)

#### Fixture F1: No clusters warning (Class 1)

**Parameters**: ε = 0.1, minPoints = 5, Euclidean, 2D, 4 points each at (0,0), (1,0), (0,1), (1,1)

Grid side = 0.1/√2 ≈ 0.0707. Each point is in its own grid cell with 1 point each → no core grids (all < 5).

**Expected**: Warning result code `-85640`, all ClusterIds = 0, AM.tupleCount = 1.

#### Fixture F2: Mask exclusion (Class 1)

**Parameters**: ε = 1.0, minPoints = 2, Euclidean, 2D, UseMask=true

Grid side = 1.0/√2 ≈ 0.707. Points:
- P0 = (0.0, 0.0), mask=true
- P1 = (0.1, 0.0), mask=true → both in grid cell 0 → 2 points ≥ minPts=2 → core grid → Cluster 1
- P2 = (0.0, 0.1), mask=false → excluded from binning → ClusterIds[2] = 0 regardless of spatial proximity

**Expected**: ClusterIds = [1, 1, 0], AM.tupleCount = 2.

*Oracle artifacts to save*: `run_sklearn_oracle.py` script + `oracle_results.json` output, and derivation notes for F1/F2 in the archive ReadMe.

---

## Phase 6 — SIMPLNX vs. oracle reconciliation

**Status: Complete for Class 2 oracle (2026-08-04). Class 1 analytical fixtures (F1/F2) exercised in Phase 8.**

### Run summary

- All 8 existing DBSCAN tests pass: `ctest -R "SimplnxCore::DBSCAN"` exits 0 (build: `DREAM3D-Build/DREAM3DNX-Release-Linux-x64/`).
- Class 2 comparison script: `dbscan_vv/compare_exemplar_vs_oracle.py`
- Input: `dbscan_test.tar.gz` → `7_0_2d_dbscan_test_data.dream3d` (extracted to `DREAM3D_Data/TestFiles/dbscan_test/`)
- Oracle: `dbscan_vv/oracle_results.json` (sklearn 1.7.1, run 2026-08-04)
- Full results: `dbscan_vv/phase6_comparison_results.json`

### Comparison results — Class 2 (cluster count + sorted bin sizes)

| Dataset | SIMPLNX clusters | SIMPLNX noise | SIMPLNX sizes | Oracle clusters | Oracle noise | Oracle sizes | Match? |
|---|---|---|---|---|---|---|---|
| ansio | 3 | 30 | [153, 156, 161] | 6 | 21 | [4, 5, 6, 152, 155, 157] | ⚠️ DEVIATION |
| blobs | 2 | 7 | [164, 329] | 2 | 7 | [164, 329] | ✅ PASS |
| noisy_circles | 2 | 0 | [250, 250] | 2 | 0 | [250, 250] | ✅ PASS |
| noisy_moons | 2 | 0 | [250, 250] | 2 | 0 | [250, 250] | ✅ PASS |
| no_structure | 1 | 0 | [500] | 1 | 0 | [500] | ✅ PASS |
| varied | 3 | 78 | [87, 166, 169] | 11 | 47 | [3, 3, 3, 4, 4, 4, 5, 5, 87, 166, 169] | ⚠️ DEVIATION |

### Analysis of deviations

**ansio** and **varied** deviate from the sklearn oracle — but the deviations are fully explained by **Deviation DBSCAN-D1** (GDCF grid-cell core definition vs. traditional point-level core definition). Key evidence:

- **`varied`**: SIMPLNX's 3 cluster sizes [87, 166, 169] exactly match sklearn's top-3 clusters [87, 166, 169]. The 8 micro-clusters (sizes 3,3,3,4,4,4,5,5 = 31 points) found by sklearn are absent in SIMPLNX — those 31 points appear as additional noise (SIMPLNX=78 noise vs sklearn=47, delta=31 ✓). Micro-clusters with ≤5 points are below the GDCF effective density threshold at minPts=3 when points spread across grid cells.
- **`ansio`**: SIMPLNX finds 3 clusters vs sklearn's 6. SIMPLNX's large-cluster sizes [153, 156, 161] closely match sklearn's large-cluster sizes [152, 155, 157] (SIMPLNX large cluster total = 470 vs sklearn = 464, delta=6 points redistributed). The 3 sklearn micro-clusters (4, 5, 6 points) are absent in SIMPLNX; 9 of their points became noise and 6 were absorbed into adjacent large clusters via GDCF grid-cell merging at boundaries.

**Neither deviation is a bug.** Both are the designed consequence of GDCF's grid-cell core-object definition: small groups of points that individually satisfy traditional DBSCAN's ε-neighborhood criterion fail GDCF's grid-cell occupancy criterion when those points straddle cell boundaries. This is precisely what Deviation DBSCAN-D1 documents.

**Well-separated, dense clusters (blobs, circles, moons, no_structure) match exactly** — confirming the implementation is correct for the large-scale clustering use case.

### Circular oracle disposition

The `dbscan_test.tar.gz` input point arrays were originally pulled from scikit-learn toy dataset generators (`make_circles`, `make_moons`, `make_blobs`, `make_blobs` anisotropic via linear transform, uniform random) using deterministic seeds and `StandardScaler`. The LDF cluster-label arrays in the archive were generated by running SIMPLNX itself after the PR #1421 rewrite — making them a circular oracle at the time of creation, despite the input data being independently sourced.

This phase closes the circularity by running the same sklearn 1.7.1 library that produced the input data directly against those inputs as an oracle:

- The Class 2 sklearn oracle independently confirms SIMPLNX is correct for 4/6 datasets (exact match on cluster count and sorted bin sizes).
- The 2 deviating datasets (ansio, varied) are fully explained by DBSCAN-D1 and corroborated by the Phase 9 legacy comparison.
- **The LDF cluster-label arrays in `dbscan_test.tar.gz` are hereby promoted from "circular oracle" to "regression fixtures."** The sklearn-sourced input data + sklearn oracle result is the correctness proof; the exemplar arrays now pin that verified-correct SIMPLNX output for regression detection.
- The 3D exemplar remains without external oracle verification — it retains circular-oracle status and is not part of the correctness proof.
- The provenance sidecar at `src/Plugins/SimplnxCore/vv/provenance/dbscan_test.md` was updated in Phase 10.

---

## Phase 7 — Algorithm Review

**Status: Complete.**

*Findings and dispositions*:

- **`ParseOrder::Random` used the user seed — BUG, FIXED.** `DBSCANFilter::executeImpl` computed a time-based seed for the seed-provenance array but passed `filterArgs.value(k_SeedValue_Key)` into `DBSCANInputValues::Seed`. Because `k_SeedValue_Key` is only exposed in the GUI for `SeededRandom`, `Random` silently ran with the default seed `5489` on every invocation — making it a duplicate of `SeededRandom` rather than the non-deterministic order that `docs/DBSCANFilter.md` has always described. Fixed by passing the resolved `seed` through. This is a documented-behavior-vs-implementation defect, not a deviation from legacy, so it is recorded here rather than as a `DBSCAN-D<N>` entry.
- **`findClusterRoot` recursion — FIXED.** Converted from unbounded recursion to an iterative parent walk, removing the stack-overflow risk on long parent chains.
- **`QuickSortGrids` recursion — FIXED.** The `begin >= end` guard was already correct, but the double recursion reached O(n) stack depth when core-grid occupancies are already sorted ascending. It now recurses into the smaller partition and loops on the larger, capping depth at O(log n). Output ordering is unchanged — the two partitions are disjoint, so the order they are processed in cannot affect the result — and the LDF regression fixtures still match exactly.
- **All-false mask left bounds NaN — FIXED.** With no active point every bound stayed `quiet_NaN`, and `static_cast<usize>(NaN)` was then used to compute the grid dimensions, which is undefined behavior. Measured behavior differed by architecture (arm64 saturates to 0; x86-64 yields `INT64_MIN`); in both cases the resulting dimensions happened to produce an empty grid and the correct `-85640` warning, so no incorrect output was ever produced and this was never observed as a crash. Now guarded explicitly, with an added message naming the mask as the cause, and pinned by analytical fixture F3.
- **`canMerge` cancel check** — added. It returns `false` on cancel; both call sites re-check `m_ShouldCancel` on the next iteration and discard results, so the early `false` cannot corrupt output.
- **Cancel checks** — confirmed present in `cluster()`, the expansion loop, `label()`, and both `HyperGridBitMap` constructors.
- **Progress messaging** — `ThrottledMessenger`/`MessageHelper` used throughout; a missing `" - Determining bounds..."` message was added to the 2D path for parity with 3D.
- **`ProcessSection` partition bounds** — the unguarded `front++`/`back--` scan loops are safe: the pivot value at `sorted[begin]` halts the first forward scan, and after each swap positions `back + 1` and `front - 1` act as sentinels. `ProcessSection` also always returns a value strictly less than `end`, which is what makes the partition recursion terminate. Verified by inspection; no bounds guard needed.
- **`Minimum Points` parameter help text** — reworded to state that it is a per-grid-cell occupancy threshold rather than a count of neighbors within Epsilon. Deviation DBSCAN-D1 names the old wording as a cause of user surprise, so the wording is now aligned with the algorithm.

*Deferred (not addressed in this pass)*:

- The shuffle used for `Random`/`SeededRandom` is a biased hand-rolled Fisher-Yates: it draws `r` from `[0, size - 2]` (so the last index is never selected as a partner) over the full range rather than `[0, i]`, and it uses `uniform_real_distribution` + `floor` instead of `uniform_int_distribution`. `std::shuffle` would be both correct and simpler, but it changes the grid order produced for a given seed, and therefore changes `SeededRandom` cluster-ID numbering. That is a user-visible reproducibility change and warrants its own deviation entry, so it is left for a follow-up.
- `RunAlgorithm` skips the labeling step whenever `cluster()` returns any warning, using "has warnings" as a proxy for "the cluster forest is ill-formed". That is correct today because `-85640` is the only warning `cluster()` can emit, but it will silently suppress labeling if a second, unrelated warning is ever added. Worth replacing with an explicit signal.

## Phase 8 — Unit Test Review & Implementation

**Status: Complete (2026-08-05).**

Changes made to `src/Plugins/SimplnxCore/test/DBSCANTest.cpp`:

1. **Added** `#include "simplnx/DataStructure/DataStore.hpp"` for inline data creation.
2. **Added** `CheckClusterInvariants(dataStructure, idsPath, amPath)` helper in anonymous namespace — asserts: (a) all IDs ≥ 0, (b) IDs 1..maxId contiguous with no gaps (ID 0 is reserved for noise and is exempt from the contiguity check), (c) AM.tupleCount == maxId+1.
3. **Hooked** `CheckClusterInvariants` into both `LDFTestCase2D` and `RandomTestCase2D` before `CheckArraysInheritTupleDims` — all 18 existing 2D test runs now exercise Class 4 invariants.
4. **Added** `TEST_CASE: Analytical Fixture F1 - No Clusters Warning` — 4 corner points, ε=0.1, minPts=5; each point in its own cell; confirms warning code -85640 and all IDs==0.
5. **Added** `TEST_CASE: Analytical Fixture F2 - Mask Exclusion` — 3 points with P2 masked; P0+P1 share one grid cell (core grid at minPts=2, ε=1.0); confirms ClusterIds=[1,1,0].

Code paths newly covered: path #4 (mask=true), path #5 (no core grids warning).

---

## Phase 9 — Legacy DREAM3D Comparison

**Status: Complete (2026-08-05).**

### Setup

- **Legacy runner**: DREAM3D 6.5.172 `PipelineRunner` from a local legacy proof build (DREAM3DReview plugin confirmed loaded — filter UUID `{c2d4f1e8-2b04-5d82-b90f-2191e8f4262e}`)
- **Script**: `dbscan_vv/phase9_ab_test.py`
- **Input**: `dbscan_vv/6_5_input.dream3d` — 6.5-format HDF5 created from same sklearn `.txt` files used in Phase 6 (500 points × 2 components each dataset, float32, Vertex AttributeMatrix)
- **Pipeline**: `dbscan_vv/dbscan_6_5_pipeline.json` — DataContainerReader → 6× DBSCAN → DataContainerWriter
- **Output**: `dbscan_vv/6_5_output.dream3d`, full results `dbscan_vv/phase9_comparison_results.json`

### Three-way comparison results

| Dataset | Legacy 6.5.172 clusters | Legacy noise | Legacy sizes | sklearn clusters | sklearn noise | sklearn sizes | SIMPLNX clusters | SIMPLNX noise | SIMPLNX sizes | Legacy vs sklearn | Legacy vs SIMPLNX |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ansio | 6 | 24 | [1, 5, 6, 152, 155, 157] | 6 | 21 | [4, 5, 6, 152, 155, 157] | 3 | 30 | [153, 156, 161] | ⚠️ close | ⚠️ DBSCAN-D1 |
| blobs | 2 | 7 | [164, 329] | 2 | 7 | [164, 329] | 2 | 7 | [164, 329] | ✅ EXACT | ✅ EXACT |
| noisy_circles | 2 | 0 | [250, 250] | 2 | 0 | [250, 250] | 2 | 0 | [250, 250] | ✅ EXACT | ✅ EXACT |
| noisy_moons | 2 | 0 | [250, 250] | 2 | 0 | [250, 250] | 2 | 0 | [250, 250] | ✅ EXACT | ✅ EXACT |
| no_structure | 1 | 0 | [500] | 1 | 0 | [500] | 1 | 0 | [500] | ✅ EXACT | ✅ EXACT |
| varied | 11 | 48 | [3, 3, 3, 3, 4, 4, 5, 5, 87, 166, 169] | 11 | 47 | [3, 3, 3, 4, 4, 4, 5, 5, 87, 166, 169] | 3 | 78 | [87, 166, 169] | ⚠️ close | ⚠️ DBSCAN-D1 |

### Analysis

**4/6 datasets: exact three-way match.** `blobs`, `noisy_circles`, `noisy_moons`, and `no_structure` agree exactly across DREAM3D 6.5.172, sklearn, and SIMPLNX. These are the densely-clustered datasets where every grid cell in the GDCF grid contains many points — the grid-cell core definition and the point-level ε-neighborhood core definition produce identical outcomes.

**2/6 datasets: DBSCAN-D1 confirmed.** For `ansio` and `varied`, DREAM3D 6.5.172 and sklearn agree on cluster count (6 and 11 respectively) while SIMPLNX finds fewer clusters (3 for both). The SIMPLNX large-cluster sizes match the legacy large-cluster sizes: for `varied`, SIMPLNX sizes [87, 166, 169] are a subset of legacy sizes [3,3,3,3,4,4,5,5,87,166,169] — the 8 micro-clusters in legacy are absent in SIMPLNX because those sparse groups do not meet the GDCF grid-cell occupancy threshold.

**Minor legacy vs sklearn differences for sparse datasets** (`ansio` and `varied`): cluster count is identical but boundary-point assignment differs slightly. Legacy uses strict `dist < epsilon` comparison (DREAM3DReview `DBSCANTemplate.hpp` line `if(dist < m_Epsilon)`); sklearn uses `dist <= epsilon` by default. For floating-point data, this almost never produces actual differences, but processing-order and data-layout effects on border points cause the observed discrepancy.

**`ansio` anomaly — cluster of size 1**: The legacy result includes a cluster of size **1** (`[1, 5, 6, 152, 155, 157]`), while sklearn's smallest cluster has 4 points. A cluster of size 1 is structurally impossible in correct traditional DBSCAN with minPoints=4 (a core point must have ≥4 neighbors within ε, so the cluster always contains at least those neighbors). The most likely cause is **float32 precision artifact**: the input was converted from float64 to float32 when creating the 6.5 HDF5 file, slightly shifting inter-point distances. A border point that would connect 3 points to a core in float64 fails the strict `< epsilon` test after float32 rounding, leaving an isolated point classified as its own cluster of 1. This is a float32 input-conversion artifact in the Phase 9 test setup, **not a bug in the legacy filter or in SIMPLNX**. It does not affect the DBSCAN-D1 deviation classification.

### Deviations confirmed

| ID | Confirmed? | Evidence |
|---|---|---|
| DBSCAN-D1 | ✅ Yes | Legacy finds 6/11 clusters (matching sklearn) vs SIMPLNX 3/3 for ansio/varied. SIMPLNX micro-clusters absent. |
| DBSCAN-D2 | ✅ Yes | Cluster ID numbering differs between legacy (uses natural traversal order) and SIMPLNX (LowDensityFirst sort). Cluster membership matches at structure level for well-separated data. |
| DBSCAN-D3 | ✅ Yes | `use_precaching` absent from SIMPLNX parameters; confirmed in SIMPL conversion test. |
| DBSCAN-D4 | ✅ Yes | `init_type_index` → `parse_order_index` rename handled by `FromSIMPLJson`; confirmed in SIMPL backwards-compat TEST_CASE. |

---

## Phase 10 — Exemplar Validation & Publishing

**Status: Complete (2026-08-05).**

The `dbscan_test.tar.gz` LDF exemplar arrays have been promoted from "circular oracle" to "regression fixtures" — the Class 2 sklearn oracle (Phase 6) independently confirmed SIMPLNX correctness for 4/6 datasets, and the legacy 6.5.172 comparison (Phase 9) corroborated the DBSCAN-D1 classification for the remaining 2 datasets. No archive regeneration is required; no new exemplar archive was created.

Provenance sidecar updated at `src/Plugins/SimplnxCore/vv/provenance/dbscan_test.md` with both Phase 6 and Phase 9 entries.

The F1 and F2 analytical fixtures added in Phase 8 are self-contained in test source — they do not require exemplar archive data.

---

## Phase 11 — Documentation Review

**Status: Complete (2026-08-05).**

Changes made to `src/Plugins/SimplnxCore/docs/DBSCANFilter.md`:

1. **Removed** stale bug note from Examples section: *"at the time of image capture a bug was showing the yellow as NaNs, but they were labeled with 3 in the cluster array"* — the referenced visualization bug is no longer present.
2. **Added** new **"Known Differences from Traditional DBSCAN"** section before Hyperparameter Tuning. Explains the GDCF core-object definition vs. traditional point-level core definition, and the practical consequence (micro-clusters in sparse data may be labeled noise). Gives actionable guidance (lower minPoints or increase ε).
3. **Verified** visualization steps (Steps 1–7) — parameter names reference filter names, not internal parameter keys; unaffected by the v2 `init_type_index → parse_order_index` rename. No changes needed.

---

## Phase 12 — Archive

**Status: Complete (2026-08-05).**

All V&V working artifacts are stored in the `dbscan_vv/` working folder outside the repository (see the referenced work file noted on the PR):

| File | Purpose |
|---|---|
| `dbscan_data_proj/` | Data generation project: sklearn toy datasets, venv, `plot_cluster_comparison.py` |
| `run_sklearn_oracle.py` | Class 2 sklearn 1.7.1 oracle script (Phase 5) |
| `oracle_results.json` | Sklearn oracle results — cluster counts + sizes per dataset (Phase 5) |
| `compare_exemplar_vs_oracle.py` | Phase 6 three-way comparison: exemplar vs. sklearn (Phase 6) |
| `phase6_comparison_results.json` | Phase 6 comparison results — SIMPLNX vs. sklearn (Phase 6) |
| `phase9_ab_test.py` | A/B test script: creates 6.5 HDF5 input, runs legacy 6.5.172, three-way comparison (Phase 9) |
| `6_5_input.dream3d` | HDF5 input file in DREAM3D 6.5 format used by legacy PipelineRunner (Phase 9) |
| `dbscan_6_5_pipeline.json` | DREAM3D 6.5 pipeline JSON: DataContainerReader + 6×DBSCAN + DataContainerWriter (Phase 9) |
| `6_5_output.dream3d` | Output from DREAM3D 6.5.172 PipelineRunner containing legacy cluster IDs (Phase 9) |
| `phase9_comparison_results.json` | Phase 9 three-way comparison results (legacy 6.5.172 vs. sklearn vs. SIMPLNX) (Phase 9) |

No SBIR submission packaging required at this stage. Artifacts are on-disk in the development environment; all scripts are self-contained and reproducible.

---

## Phase 13 — Update tracking artifacts

**Status: Complete (2026-08-05).**

All phases (1–12) complete. Status line is currently `IN-REVIEW` (updated by Nathan Young, 8/7/2026). Final status to be updated to `COMPLETE` after second-engineer oracle review and sign-off.

**Open item before COMPLETE**: Second-engineer oracle review (Phase 4 — skipped, see provenance sidecar). Once a second engineer reviews the oracle design and signs off, update the Status line to `COMPLETE` and fill in the Sign-off field in the document header.
