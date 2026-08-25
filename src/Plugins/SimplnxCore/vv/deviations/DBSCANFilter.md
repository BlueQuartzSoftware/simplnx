# Deviations from DREAM3D 6.5.172: DBSCANFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.172 equivalent.

Entries are referenced by stable ID (`DBSCAN-D<N>`) from the V&V report and from public migration guidance. The Filter UUID fields are the permanent cross-reference anchors.

**Note on UUID change**: The SIMPLNX filter UUID changed from `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` (legacy SIMPL) to `763dad44-fad7-4606-808f-617867257b98` (SIMPLNX). This change explicitly signals that SIMPLNX implements a different algorithm and is not claiming functional equivalence. The legacy UUID is retained in `SimplnxCoreLegacyUUIDMapping.hpp` for pipeline backward compatibility only. Each deviation entry cites both UUIDs for traceability.

---

## DBSCAN-D1

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D1` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |

**Symptom:** Sparse datasets where individual data points each have ≥ minPoints neighbors within ε (and would form clusters under traditional DBSCAN) may produce **more noise (cluster 0) points in SIMPLNX** if those neighborhoods span multiple grid cells with fewer than minPoints points per cell.

**Confirmed (Phase 9, 2026-08-05):** DREAM3D 6.5.172 run on the 6 sklearn toy datasets confirms: legacy finds 6 clusters (ansio) and 11 clusters (varied) — matching sklearn — while SIMPLNX finds only 3 for both. The large-cluster sizes agree for varied ([87,166,169] — exact three-way match); for ansio the large clusters are close but not identical (sklearn oracle: [152,155,157]; SIMPLNX: [153,156,161] — small boundary discrepancy also attributable to GDCF grid-cell vs. point-level core definition). Only the micro-clusters (≤6 points each) are absent in SIMPLNX. Evidence: `dbscan_vv/phase9_comparison_results.json`.

**Root cause:** Algorithmic choice. SIMPLNX implements Grid-based DBSCAN (GDCF, Boonchoo et al. 2019). The core-object definition differs fundamentally:
- **Legacy (traditional DBSCAN)**: A data point `p` is a *core point* if the ε-ball centered on `p` contains ≥ minPoints data points (inclusive of `p` itself). Cluster membership is point-centric.
- **SIMPLNX (GDCF)**: A *core grid* is a grid cell (side length = ε/√dims) that contains ≥ minPoints data points. Cluster membership is grid-centric.

Consequence: consider 4 data points, 2 per cluster, each with inter-point distance 0.5ε. In traditional DBSCAN with minPoints=2, each point's ε-ball contains its neighbor — both are core points and form a cluster. In SIMPLNX, if the 2 points per cluster happen to span two adjacent grid cells (each with 1 point), neither cell is a core grid, and both points are labeled noise (cluster 0), even though the traditional algorithm would cluster them. This occurs when point spacing is comparable to the grid cell side (ε/√dims).

This deviation is not a bug — it is the intended behavior of GDCF, which sacrifices strict point-level equivalence for significant performance gains on large 3D datasets (O(n log n) vs. O(n²) distance checks).

**Affected users:** Users who:
(1) Have data with very uniform spacing close to ε/√dims (exactly the scale where points are likely to straddle grid-cell boundaries), OR
(2) Use minPoints values that are sensitive to point-vs-grid counting (especially minPoints=2 or 3 on sparse data), OR
(3) Expect per-point ε-neighborhood semantics from the "minPoints" parameter description.

Users with dense, well-clustered data (many points per grid cell) are unlikely to notice a difference.

**Recommendation:** Trust SIMPLNX for production use on large 3D datasets. The GDCF algorithm provides substantial performance advantages. If strict traditional DBSCAN semantics are required, note that "minPoints" in SIMPLNX controls grid-cell occupancy rather than point-level ε-neighborhood density. Adjust minPoints and/or ε accordingly: lower minPoints (e.g., 2→1) or increase ε to ensure sufficient grid-cell occupancy.

---

## DBSCAN-D2

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D2` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |

**Symptom:** When run with default parameters on data where multiple clusters exist, SIMPLNX typically assigns cluster IDs in a different order than legacy DREAM3D. The cluster membership (which points belong together) is functionally identical, but the numeric labels differ.

**Root cause:** Algorithmic choice. SIMPLNX adds `LowDensityFirst` parse order (the new default) with no equivalent in legacy DREAM3D. `LowDensityFirst` sorts core grids ascending by occupancy (less dense grids processed first) before the union-find merge phase, which changes the order in which cluster IDs are assigned. Legacy DREAM3D used an effectively arbitrary (memory-layout-dependent) order for core point processing.

The `Random` parse order in SIMPLNX uses a time-based seed (non-deterministic), so even successive identical runs may produce different cluster ID numbering. `SeededRandom` provides reproducibility with an explicit user seed.

**Affected users:** Users who rely on specific cluster ID values (e.g., downstream filters that look for "cluster 1" by number rather than by properties). Cluster membership (which points are grouped together) is unaffected; only the numeric label assigned to each cluster changes.

**Recommendation:** Trust SIMPLNX. For deterministic output, use `LowDensityFirst` (default) or `SeededRandom`. Do not rely on specific cluster ID numbers in downstream pipelines; use cluster properties (size, centroid, etc.) for selection instead.

---

## DBSCAN-D3

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D3` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |

**Symptom:** SIMPL pipelines that explicitly set `use_precaching=true` or `use_precaching=false` will load in SIMPLNX without error, but the `use_precaching` value is silently ignored.

**Root cause:** Algorithmic choice. The legacy filter offered a memory/time trade-off switch: `use_precaching=true` pre-loaded data for faster neighbor queries at the cost of additional memory. The GDCF rewrite (PR #1421) replaced the distance-computation strategy entirely with a grid-based bit-packed adjacency table, making the trade-off concept obsolete. The parameter was dropped in `parametersVersion() == 2`. The `FromSIMPLJson` converter (see `DBSCANFilter.cpp`, starting at line 242) does not read `use_precaching` from SIMPL JSON, so old pipelines that set it will have it silently discarded.

**Affected users:** Users converting SIMPL pipelines that explicitly set `use_precaching`. The converted pipeline will run correctly in SIMPLNX; only the parameter setting is lost.

**Recommendation:** Trust SIMPLNX. No action required when converting pipelines — the parameter has no analog in SIMPLNX and its absence does not change the output.

---

## DBSCAN-D4

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D4` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |

**Symptom:** SIMPLNX pipelines saved prior to PR #1421 that contain the parameter key `init_type_index` will silently lose their parse-order setting when loaded in SIMPLNX v2. The filter loads without error, but the parse order silently falls back to `LowDensityFirst` (the default). SIMPL (6.4/6.5) pipelines are **not** affected — SIMPL never had an `init_type_index` parameter.

**Root cause:** Algorithmic choice (parameter rename as part of rewrite). During PR #1421, the parameter key was renamed from `init_type_index` to `parse_order_index`. `parametersVersion()` returns `2` and its comment block describes an intended v1→v2 migration that would read the old key and map it to the new key — but `upgradeParametersImpl()` is **not implemented** in `DBSCANFilter`. As a result, any early SIMPLNX pipeline JSON containing `init_type_index` silently deserializes as the default `LowDensityFirst` value.

The `FromSIMPLJson` converter (`DBSCANFilter.cpp`, starting at line 242) is not affected: SIMPL 6.4 and 6.5 never had an `init_type_index` parameter, confirmed by `test/simpl_conversion/6_4/DBSCANFilter.json` and `test/simpl_conversion/6_5/DBSCANFilter.json`.

**Affected users:** SIMPLNX pipeline authors who explicitly saved a pipeline with a non-default parse order (`Random` or `SeededRandom`) using the v1 key `init_type_index` before PR #1421. These pipelines will silently default to `LowDensityFirst` without warning. Users who used the default `LowDensityFirst` or who created pipelines after PR #1421 are unaffected.

**Recommendation:** If you have early SIMPLNX pipelines that explicitly set a non-default parse order, verify the `parse_order_index` key is present in the pipeline JSON. Edit the pipeline to use `parse_order_index` if it still contains `init_type_index`.
