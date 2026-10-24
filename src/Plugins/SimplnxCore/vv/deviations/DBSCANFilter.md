# Deviations from DREAM.3D v6.5.171: DBSCANFilter

This file has two parts. Part 1 lists every documented behavioral difference between this SIMPLNX filter and its DREAM.3D v6.5.171 equivalent (D1–D3). Part 2 lists SIMPLNX defects and SIMPLNX behavior changes that are not differences from DREAM.3D v6.5.171 (D4–D8).

Entries are referenced by stable ID (`DBSCANFilter-D<N>`) from the V&V report and from public migration guidance. The Filter UUID is the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### DBSCANFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D1` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |

**Symptom:** SIMPLNX can label sparse points as noise (cluster 0) when traditional DBSCAN puts them in clusters. This occurs when a point neighborhood spans grid cells with fewer than the minimum number of points per cell.

**Root cause:** Algorithmic choice. DREAM.3D uses the number of points within epsilon of a point to identify a core point. SIMPLNX uses GDCF (Boonchoo et al. 2019): a cell with side length epsilon divided by the square root of the dimension count is a core grid when its occupancy reaches the minimum point count. Adjacent cells below that count do not form a core grid, even when a point neighborhood has enough points. The rewrite preserves density-based clustering but changes the core criterion.

**Affected users:** Users with sparse data, point spacing close to the grid-cell size, or a requirement for traditional point-level DBSCAN neighborhoods.

**Recommendation:** Trust SIMPLNX when grid-cell density is the required criterion. Lower the minimum point count or increase epsilon to increase grid occupancy. These changes do not make the two core criteria equivalent.

**Evidence:** The saved comparison in `vv_work:DBSCANFilter/ab_6_5_171_comparison_results.json` uses DREAM.3D v6.5.171 output and SIMPLNX exemplar labels. Four datasets have equal cluster counts, noise counts and sorted sizes. For `ansio` and `varied`, SIMPLNX has 3 clusters; DREAM.3D v6.5.171 has 6 and 11, respectively. The complete table is in `../provenance/dbscan_test.md`. The script does not run the current SIMPLNX algorithm.

---

### DBSCANFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D2` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |

**Symptom:** SIMPLNX can assign different cluster IDs from DREAM.3D when the input has multiple clusters. Numeric labels can differ even when membership is the same. The core-object difference in `DBSCANFilter-D1` can also change membership.

**Root cause:** Algorithmic choice. SIMPLNX adds the default `LowDensityFirst` order, which processes core grids in ascending occupancy order. This changes the order in which the filter assigns cluster IDs. `Random` uses a time-based seed; `SeededRandom` uses the user seed.

**Affected users:** Users whose downstream filters select a cluster by its numeric ID.

**Recommendation:** Trust SIMPLNX. Use `LowDensityFirst` or `SeededRandom` for repeatable results. Select clusters by properties such as size or centroid instead of their numeric IDs.
---

### DBSCANFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D3` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |

**Symptom:** SIMPL pipelines that explicitly set `use_precaching=true` or `use_precaching=false` will load in SIMPLNX without error, but the `use_precaching` value is silently ignored.

**Root cause:** Algorithmic choice. DREAM3D-NX v7.2.2 replaced the distance-query strategy with a grid-based adjacency table and removed `use_precaching`. The filter no longer has this memory/time option. `DBSCANFilter::FromSIMPLJson()` does not read the parameter.

**Affected users:** Users who convert pipelines that set `use_precaching`. The converted pipeline does not retain this setting.

**Recommendation:** Trust SIMPLNX. No action required when converting pipelines — the parameter has no analog in SIMPLNX and its absence does not change the output.

**Evidence:** PR #1421 introduced the GDCF rewrite. `DBSCANFilter::parametersVersion()` records removal of `use_precaching`; `FromSIMPLJson()` does not read that parameter.

---

## Part 2 — SIMPLNX defects and behavior changes

### DBSCANFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D4` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.2 through v7.4.2. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | Not fixed |

**Symptom:** Pipelines saved in DREAM3D-NX v7.0.0 through v7.2.1 with the parameter key `init_type_index` lose their parse-order setting when loaded in DREAM3D-NX v7.2.2 or later. The filter loads without error, but the parse order silently falls back to `LowDensityFirst` (the default). DREAM.3D pipelines are not affected; the legacy filter had no `init_type_index` parameter.

**Root cause:** Bug. DREAM3D-NX v7.2.2 renamed `init_type_index` to `parse_order_index`. The filter has no `upgradeParametersImpl()` override to copy the old value, so it uses the default `LowDensityFirst`. The DREAM.3D converter is not affected because the legacy filter had no `init_type_index` parameter.

**Affected users:** SIMPLNX pipeline authors who explicitly saved a pipeline with a non-default parse order (`Random` or `SeededRandom`) using `init_type_index` in DREAM3D-NX v7.0.0 through v7.2.1. These pipelines will silently default to `LowDensityFirst` without warning. Users who used the default `LowDensityFirst` or who saved pipelines with `parse_order_index` in DREAM3D-NX v7.2.2 or later are unaffected.

**Recommendation:** If you have early SIMPLNX pipelines that explicitly set a non-default parse order, verify the `parse_order_index` key is present in the pipeline JSON. Edit the pipeline to use `parse_order_index` if it still contains `init_type_index`.

**Evidence:** PR #1421 renamed the parameter. The local release sources `upstream/release/v7.2.1` and `upstream/release/v7.2.2` use `init_type_index` and `parse_order_index`, respectively. `DBSCANFilter` has no `upgradeParametersImpl()` override. The conversion fixtures in `test/simpl_conversion/6_4/DBSCANFilter.json` and `test/simpl_conversion/6_5/DBSCANFilter.json` have no `init_type_index` parameter.

---

### DBSCANFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D5` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** DREAM3D-NX v7.4.2 changes the shuffle used by `SeededRandom` from DREAM3D-NX v7.2.2 through v7.4.1. The same seed can give different cluster IDs or different assignments of border points.

**Root cause:** Bug. The previous shuffle excluded the last index as a swap partner and drew each partner from the full range instead of the remaining range. It also converted floating-point draws to indices. These choices biased the permutations. The filter now uses `std::shuffle`.

**Affected users:** Users who need to reproduce a cluster-ID assignment from DREAM3D-NX v7.2.2 through v7.4.1 with `SeededRandom`. `LowDensityFirst` does not use the shuffle.

**Recommendation:** Trust SIMPLNX. Run the same seed in DREAM3D-NX v7.4.2 or later and use that output as the reference for repeatable results.

**Evidence:** PR #1702 replaced the shuffle. The source at `upstream/release/v7.4.2` calls `std::shuffle`; `upstream/release/v7.4.1` has the previous implementation.

---

### DBSCANFilter-D6

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D6` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.2. |
| **Fixed in** | The DREAM3D-NX release after v7.4.2 |

**Symptom:** With **Use Mask Array** enabled, SIMPLNX previously accepted a multi-component or short **Cell Mask Array** in preflight. The Direct path read the mask by flat element, while the external Scanline path rejected it during execution with `-54062`. Both paths now reject these inputs before execution: component shape other than `{1}` produces `-208`, and unequal tuple counts produce `-7587`.

**Root cause:** Bug. The mask parameter did not restrict component shape, and preflight did not compare mask and input tuple counts. Validation now applies before algorithm selection. The evidence covers the SIMPLNX correction only.

**Affected users:** Users who select a mask with multiple components or a tuple count different from the clustering array. The Direct algorithm could read unintended mask elements or access data beyond a short mask.

**Recommendation:** Select a boolean or uint8 mask DataArray with component shape `{1}` and one tuple per tuple in **Attribute Array to Cluster**. The preflight tuple-count error identifies both DataArrays and their tuple counts.

**Evidence:** PR #1766 contains the mask correction. Commit `65a3038e` includes the component-shape and tuple-count checks and the `Reject multi-component mask in preflight`, `Reject short mask in preflight` and `Reject short StringArray mask in preflight` cases in `DBSCANTest.cpp`. The source at `upstream/release/v7.4.2` lacks these checks. This documentation checkout also lacks the correction and those three tests.

---

### DBSCANFilter-D7

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D7` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.0.0 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** In DREAM3D-NX v7.0.0 through v7.4.1, the `Random` parse order gave the same result on every run. It gave the same result as `SeededRandom` with the default seed.

**Root cause:** Bug. `DBSCANFilter::executeImpl` made a time-based seed and wrote it to the seed array. It then gave `k_SeedValue_Key` to the algorithm, not the generated seed. The GUI shows `k_SeedValue_Key` only for `SeededRandom`, so `Random` always used the default seed `5489`. The filter now gives the generated seed to the algorithm.

**Affected users:** Users who selected `Random` and expected a different core-grid order on each run.

**Recommendation:** Trust SIMPLNX. The current `Random` parse order agrees with the documentation; use `SeededRandom` for repeatable results.

**Evidence:** PR #1702 corrected seed forwarding. The source at `upstream/release/v7.4.2` assigns `inputValues.Seed = seed`; `upstream/release/v7.4.1` forwards the user seed.

---

### DBSCANFilter-D8

| Field | Value |
|---|---|
| **Deviation ID** | `DBSCANFilter-D8` |
| **Filter UUID** | `763dad44-fad7-4606-808f-617867257b98` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.2.2 through v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** None observed. In DREAM3D-NX v7.2.2 through v7.4.1, a mask with no true values gave the correct warning `-85640`, but only by chance.

**Root cause:** Bug. When no point passed the mask, every grid bound stayed `quiet_NaN`. The algorithm then used `static_cast<usize>(NaN)` to calculate the grid dimensions, which is undefined behavior. The filter now has an explicit check that returns warning `-85640` with a message about the mask.

**Affected users:** Users whose mask has no true values. No incorrect output was found.

**Recommendation:** Trust SIMPLNX. The result no longer depends on undefined behavior.

**Evidence:** PR #1702 added the empty-mask guard, which is present at `upstream/release/v7.4.2`. `SimplnxCore::DBSCAN: Analytical Fixture F3 - All Points Masked` checks warning `-85640`, zero labels and one AttributeMatrix tuple.
