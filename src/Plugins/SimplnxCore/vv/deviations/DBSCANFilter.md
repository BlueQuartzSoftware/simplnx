# Deviations from DREAM3D 6.5.171: DBSCANFilter

This file lists every documented behavioral difference between this SIMPLNX filter and its DREAM3D 6.5.171 equivalent.

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

**Confirmed (Phase 9, 2026-08-05):** DREAM3D 6.5.172 run on the 6 sklearn toy datasets confirms: legacy finds 6 clusters (ansio) and 11 clusters (varied) — matching sklearn — while SIMPLNX finds only 3 for both. The large-cluster sizes agree exactly ([87,166,169] for varied; ~[152,155,157] for ansio); only the micro-clusters (≤6 points each) are absent in SIMPLNX. Evidence: `dbscan_vv/phase9_comparison_results.json`.

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

**Root cause:** Algorithmic choice. The legacy filter offered a memory/time trade-off switch: `use_precaching=true` pre-loaded data for faster neighbor queries at the cost of additional memory. The GDCF rewrite (PR #1421) replaced the distance-computation strategy entirely with a grid-based bit-packed adjacency table, making the trade-off concept obsolete. The parameter was dropped in `parametersVersion() == 2`. The `FromSIMPLJson` converter (see `DBSCANFilter.cpp` lines 248–267) does not read `use_precaching` from SIMPL JSON, so old pipelines that set it will have it silently discarded.

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

**Symptom:** The parse order parameter was renamed from `init_type_index` (SIMPL / SIMPLNX v1) to `parse_order_index` (SIMPLNX v2). SIMPL pipelines using the old key name load correctly.

**Root cause:** Algorithmic choice (parameter rename as part of rewrite). The SIMPL backward compatibility converter (`FromSIMPLJson`) reads the old `InitType` key (`init_type_index`) and maps it to the new `parse_order_index` key. The SIMPLNX parameter version migration (v1→v2 in `parametersVersion() == 2`) handles pipelines that already used the SIMPLNX key name `init_type_index` before the rename.

**Affected users:** Users who hand-authored or scripted pipeline JSON using the old parameter key. The conversion is transparent and handled automatically.

**Recommendation:** Trust SIMPLNX. The conversion is handled automatically in both `FromSIMPLJson` (SIMPL pipelines) and the v1→v2 parameter migration (early SIMPLNX pipelines).
