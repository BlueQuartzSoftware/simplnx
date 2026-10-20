# Deviations from DREAM.3D v6.5.171: DBSCANFilter

This file has two parts. Part 1 lists every documented behavioral difference between this SIMPLNX filter and its DREAM.3D v6.5.171 equivalent (D1–D3). Part 2 lists SIMPLNX defects and SIMPLNX behavior changes that are not differences from DREAM.3D v6.5.171 (D4–D8).

Entries are referenced by stable ID (`DBSCAN-D<N>`) from the V&V report and from public migration guidance. The Filter UUID fields are the permanent cross-reference anchors.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### DBSCAN-D1

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D1` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |

**Symptom:** Sparse datasets where individual data points each have ≥ minPoints neighbors within ε (and would form clusters under traditional DBSCAN) may produce **more noise (cluster 0) points in SIMPLNX** if those neighborhoods span multiple grid cells with fewer than minPoints points per cell.

**Evidence (DREAM.3D v6.5.171, 2026-10-20):** DREAM.3D v6.5.171, scikit-learn 1.7.1 and SIMPLNX ran on the six sklearn datasets in `dbscan_test.tar.gz`. The table gives the cluster count, the noise count and the sorted cluster sizes. An earlier run on a local proof version (2026-08-05) gave the same label for every point as DREAM.3D v6.5.171. Evidence: `dbscan_vv/ab_6_5_171_comparison_results.json`.

| Dataset | ε | minPts | DREAM.3D v6.5.171 | scikit-learn 1.7.1 | SIMPLNX |
|---|---|---|---|---|---|
| ansio | 0.15 | 4 | 6 clusters, 24 noise, [1, 5, 6, 152, 155, 157] | 6 clusters, 21 noise, [4, 5, 6, 152, 155, 157] | 3 clusters, 30 noise, [153, 156, 161] |
| blobs | 0.30 | 3 | 2 clusters, 7 noise, [164, 329] | 2 clusters, 7 noise, [164, 329] | 2 clusters, 7 noise, [164, 329] |
| noisy_circles | 0.30 | 3 | 2 clusters, 0 noise, [250, 250] | 2 clusters, 0 noise, [250, 250] | 2 clusters, 0 noise, [250, 250] |
| noisy_moons | 0.30 | 3 | 2 clusters, 0 noise, [250, 250] | 2 clusters, 0 noise, [250, 250] | 2 clusters, 0 noise, [250, 250] |
| no_structure | 0.30 | 3 | 1 cluster, 0 noise, [500] | 1 cluster, 0 noise, [500] | 1 cluster, 0 noise, [500] |
| varied | 0.18 | 3 | 11 clusters, 48 noise, [3, 3, 3, 3, 4, 4, 5, 5, 87, 166, 169] | 11 clusters, 47 noise, [3, 3, 3, 4, 4, 4, 5, 5, 87, 166, 169] | 3 clusters, 78 noise, [87, 166, 169] |

Four datasets match exactly in all three. For `ansio` and `varied`, DREAM.3D v6.5.171 and scikit-learn find the same number of clusters, and SIMPLNX finds 3. SIMPLNX does not find the micro-clusters of 6 points or fewer. Its large clusters match the large clusters of DREAM.3D v6.5.171 exactly for `varied` and within 6 points for `ansio`.

DREAM.3D v6.5.171 and scikit-learn differ by a small number of border points. DREAM.3D v6.5.171 uses `dist < epsilon`, and scikit-learn uses `dist <= epsilon`. The DREAM.3D v6.5.171 input is float32, and the scikit-learn input is float64. The `ansio` cluster of size 1 in DREAM.3D v6.5.171 comes from these differences. It does not change this deviation.

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

### DBSCAN-D2

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D2` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |

**Symptom:** When run with default parameters on data where multiple clusters exist, SIMPLNX typically assigns cluster IDs in a different order than legacy DREAM.3D. The cluster membership (which points belong together) is functionally identical, but the numeric labels differ.

**Root cause:** Algorithmic choice. SIMPLNX adds `LowDensityFirst` parse order (the new default) with no equivalent in legacy DREAM.3D. `LowDensityFirst` sorts core grids ascending by occupancy (less dense grids processed first) before the union-find merge phase, which changes the order in which cluster IDs are assigned. Legacy DREAM.3D used an effectively arbitrary (memory-layout-dependent) order for core point processing.

The `Random` parse order in SIMPLNX uses a time-based seed (non-deterministic), so even successive identical runs may produce different cluster ID numbering. `SeededRandom` provides reproducibility with an explicit user seed.

**Affected users:** Users who rely on specific cluster ID values (e.g., downstream filters that look for "cluster 1" by number rather than by properties). Cluster membership (which points are grouped together) is unaffected; only the numeric label assigned to each cluster changes.

**Recommendation:** Trust SIMPLNX. For deterministic output, use `LowDensityFirst` (default) or `SeededRandom`. Do not rely on specific cluster ID numbers in downstream pipelines; use cluster properties (size, centroid, etc.) for selection instead.

---

### DBSCAN-D3

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

## Part 2 — SIMPLNX defects and behavior changes

### DBSCAN-D4

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D4` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.2 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | Not fixed |

**Symptom:** SIMPLNX pipelines saved prior to PR #1421 that contain the parameter key `init_type_index` will silently lose their parse-order setting when loaded in SIMPLNX v2. The filter loads without error, but the parse order silently falls back to `LowDensityFirst` (the default). SIMPL (6.4/6.5) pipelines are **not** affected — SIMPL never had an `init_type_index` parameter.

**Root cause:** Algorithmic choice (parameter rename as part of rewrite). During PR #1421, the parameter key was renamed from `init_type_index` to `parse_order_index`. `parametersVersion()` returns `2` and its comment block describes an intended v1→v2 migration that would read the old key and map it to the new key — but `upgradeParametersImpl()` is **not implemented** in `DBSCANFilter`. As a result, any early SIMPLNX pipeline JSON containing `init_type_index` silently deserializes as the default `LowDensityFirst` value.

The `FromSIMPLJson` converter (`DBSCANFilter.cpp`, starting at line 242) is not affected: SIMPL 6.4 and 6.5 never had an `init_type_index` parameter, confirmed by `test/simpl_conversion/6_4/DBSCANFilter.json` and `test/simpl_conversion/6_5/DBSCANFilter.json`.

**Affected users:** SIMPLNX pipeline authors who explicitly saved a pipeline with a non-default parse order (`Random` or `SeededRandom`) using the v1 key `init_type_index` before PR #1421. These pipelines will silently default to `LowDensityFirst` without warning. Users who used the default `LowDensityFirst` or who created pipelines after PR #1421 are unaffected.

**Recommendation:** If you have early SIMPLNX pipelines that explicitly set a non-default parse order, verify the `parse_order_index` key is present in the pipeline JSON. Edit the pipeline to use `parse_order_index` if it still contains `init_type_index`.

---

### DBSCAN-D5

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D5` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1702, DREAM3D-NX v7.4.2 |

**Symptom:** For a given `SeededRandom` seed value, SIMPLNX now produces a different core-grid processing order — and therefore potentially different cluster-ID assignments — compared to earlier SIMPLNX builds (prior to this PR). Cluster *membership* (which points are grouped together) is unaffected for well-separated datasets; only cluster *ID numbering* and boundary-point assignment at cluster edges may differ.

**Root cause:** The hand-rolled shuffle used in earlier builds had three correctness defects: (1) the last index was never selected as a swap partner because `maxIdx = size - 1` combined with a half-open `uniform_real_distribution` excluded it; (2) the draw used the full `[0, size-1]` range rather than `[0, i]`, introducing the classic naive-shuffle bias; (3) `uniform_real_distribution<float64>` + `std::floor` adds floating-point rounding bias absent from `uniform_int_distribution`. These defects made the shuffle subtly non-uniform. The replacement `std::shuffle` (C++ standard library Fisher-Yates) is correct and unbiased, but produces a different permutation for any given seed.

**Affected users:** Users who relied on a specific `SeededRandom` seed to reproduce a particular cluster-ID assignment from a pre-PR build. The same seed now produces a correct unbiased shuffle instead of the previously biased one, so the resulting cluster-ID numbers will differ. Users running `LowDensityFirst` (the default) are entirely unaffected. Users running `Random` (time-based seed, non-deterministic by design) are unaffected in practice.

**Recommendation:** If reproducibility of specific cluster-ID values across builds is required, re-run with the same seed on the current build and use the new output as the reference. Cluster membership (grouping) is preserved for densely-clustered datasets; only the numeric label assigned to each cluster may change.

---

### DBSCAN-D6

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D6` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. |
| **Fixed in** | PR #1766, the DREAM3D-NX release after v7.4.2 |

**Symptom:** With **Use Mask Array** enabled, SIMPLNX previously accepted a multi-component or short **Cell Mask Array** in preflight. The Direct path read the mask by flat element, while the external Scanline path rejected it during execution with `-54062`. Both paths now reject these inputs before execution: component shape other than `{1}` produces `-208`, and unequal tuple counts produce `-7587`.

**Root cause:** The mask selection parameter did not restrict component shape, and preflight did not compare the tuple counts of **Cell Mask Array** and **Attribute Array to Cluster**. Validation now applies before algorithm dispatch. This is a within-SIMPLNX correction; no new comparison with DREAM.3D v6.5.171 is claimed.

**Affected users:** Users whose pipelines select a multi-component mask or a mask with a different number of tuples from the clustering DataArray. Previously accepted Direct executions may have used unintended mask elements or accessed beyond a short mask.

**Recommendation:** Select a boolean or uint8 mask DataArray with component shape `{1}` and one tuple per tuple in **Attribute Array to Cluster**. The preflight tuple-count error identifies both DataArrays and their tuple counts. Regression coverage: `SimplnxCore::DBSCAN: Reject multi-component mask in preflight` and `SimplnxCore::DBSCAN: Reject short mask in preflight` in `DBSCANTest.cpp`.

---

### DBSCAN-D7

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D7` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1702, DREAM3D-NX v7.4.2 |

**Symptom:** In DREAM3D-NX v7.0.0 through v7.4.1, the `Random` parse order gave the same result on every run. It gave the same result as `SeededRandom` with the default seed.

**Root cause:** Bug. `DBSCANFilter::executeImpl` made a time-based seed and wrote it to the seed array. It then gave `k_SeedValue_Key` to the algorithm, not the generated seed. The GUI shows `k_SeedValue_Key` only for `SeededRandom`, so `Random` always used the default seed `5489`. PR #1702 gives the generated seed to the algorithm.

**Affected users:** Users who selected `Random` and expected a different core-grid order on each run.

**Recommendation:** Trust SIMPLNX. The current `Random` parse order agrees with the documentation; use `SeededRandom` for repeatable results.

---

### DBSCAN-D8

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCAN-D8` |
| **SIMPLNX UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Legacy SIMPL UUID** | `c2d4f1e8-2b04-5d82-b90f-2191e8f4262e` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | PR #1702, DREAM3D-NX v7.4.2 |

**Symptom:** None observed. In DREAM3D-NX v7.2.2 through v7.4.1, a mask with no true values gave the correct warning `-85640`, but only by chance.

**Root cause:** Bug. When no point passed the mask, every grid bound stayed `quiet_NaN`. The algorithm then used `static_cast<usize>(NaN)` to calculate the grid dimensions, which is undefined behavior. arm64 gave 0 and x86-64 gave `INT64_MIN`; both gave an empty grid. PR #1702 adds an explicit check that returns warning `-85640` with a message about the mask. `Analytical Fixture F3 - All Points Masked` pins the result.

**Affected users:** Users whose mask has no true values. No incorrect output was found.

**Recommendation:** Trust SIMPLNX. The result no longer depends on undefined behavior.
