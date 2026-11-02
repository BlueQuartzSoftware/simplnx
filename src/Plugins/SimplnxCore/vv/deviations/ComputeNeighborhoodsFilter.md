# Deviations from DREAM.3D v6.5.171: ComputeNeighborhoodsFilter

Part 1 lists differences from `FindNeighborhoods` in DREAM.3D v6.5.171 (D1–D3). Part 2 lists fixed SIMPLNX defects (D4–D5). All entries use one stable ID sequence and the filter UUID as the permanent cross-reference anchor.

---

## Part 1 — Differences from DREAM.3D v6.5.171

### ComputeNeighborhoodsFilter-D1

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D1` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |

**Symptom:** The same centroids, diameters, and multiplier can give different neighbor counts and lists in SIMPLNX and DREAM.3D v6.5.171.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 converts centroids to integer bins using the average diameter. It requires each absolute bin difference to be strictly less than `equivalentDiameter[i] * multiplier / averageDiameter`. SIMPLNX compares squared Euclidean distance with `(equivalentDiameter[i] * multiplier)^2` and includes equality. The average diameter controls the SIMPLNX search grid, not the distance limit. SIMPLNX also offers an absolute-radius mode without Equivalent Diameters; DREAM.3D v6.5.171 has no such mode. SIMPLNX visits bins in X, Y, Z order and features within each bin in ID order, so list order differs from the DREAM.3D v6.5.171 candidate scan even when membership is equal.

**Affected users:** Users who migrate `FindNeighborhoods` pipelines and compare neighbor counts or lists. Bin quantization and the different inclusion rules prevent a fixed count ratio between implementations.

**Recommendation:** Trust SIMPLNX for centroid-distance neighborhoods. The sphere implements the stated search radius, and both radius modes have independent analytical fixtures. The UUID mapping preserves pipeline conversion but does not promise identical output from the different neighborhood definitions.

**Evidence:** `Source/Plugins/Statistics/StatisticsFilters/FindNeighborhoods.cpp` at the DREAM.3D `v6.5.171` tag contains the strict bin comparisons. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeNeighborhoods.cpp` contains the inclusive distance comparison. `SimplnxCore::ComputeNeighborhoods_SyntheticOracle` checks the exact-radius boundary, and `SimplnxCore::ComputeNeighborhoods_MultiplesAnalyticalOracle` checks per-feature asymmetry in `src/Plugins/SimplnxCore/test/ComputeNeighborhoodsTest.cpp`. On 2026-10-22, `neutral_absolute_oracle` matches all three implementations, `multiples_analytical_oracle` differs from stock in one list order, and `d1_boundary_box_quantization` differs from stock in six counts and six lists; a local proof version matches DREAM3D-NX exactly in all three cases (`vv_work:ComputeNeighborhoodsFilter/comparison/LegacyComparison.md`, `vv_work:ComputeNeighborhoodsFilter/notes/proof_commits.md`).

---

### ComputeNeighborhoodsFilter-D2

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D2` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |

**Symptom:** SIMPLNX has no Feature Phases input; DREAM.3D v6.5.171 requires that array.

**Root cause:** Algorithmic choice. DREAM.3D v6.5.171 validates Feature Phases in `FindNeighborhoods::dataCheck()` but does not read its values in the neighborhood calculation. SIMPLNX removes the parameter and its conversion mapping. Removing this unused input does not change neighborhood values.

**Affected users:** Users who convert a DREAM.3D v6.5.171 pipeline. The `FeaturePhasesArrayPath` setting is no longer used.

**Recommendation:** Trust SIMPLNX. Select Centroids and, for multiples mode, Equivalent Diameters; no phase array is required.

**Evidence:** The tagged legacy `FindNeighborhoods.cpp` requires Feature Phases only during input validation. Current `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeNeighborhoodsFilter.cpp` has no phase parameter or `FromSIMPLJson()` mapping. The two analytical fixtures create no phase array, and the SIMPL conversion test checks the remaining argument values. On 2026-10-22, `neutral_absolute_oracle` includes neighbors with different phases and matches all three implementations without a D2 proof patch; the package supplies phases for legacy compatibility and has no dedicated D2 case (`vv_work:ComputeNeighborhoodsFilter/ReadMe.md`, `vv_work:ComputeNeighborhoodsFilter/notes/source_evidence.md`, `vv_work:ComputeNeighborhoodsFilter/comparison/LegacyComparison.md`).

---

### ComputeNeighborhoodsFilter-D3

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D3` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |

**Symptom:** SIMPLNX rejects a non-positive multiplier with error `-5732`; DREAM.3D v6.5.171 accepts it. SIMPLNX also rejects a non-positive absolute radius with `-5733`.

**Root cause:** Algorithmic choice. SIMPLNX validates the active radius parameter before creating outputs. DREAM.3D v6.5.171 has no multiplier positivity check. With positive feature diameters, its strict comparison cannot accept a neighbor when the multiplier is zero or negative.

**Affected users:** Users whose converted pipelines contain a zero or negative `MultiplesOfAverage` value, or who supply a non-positive absolute radius in SIMPLNX.

**Recommendation:** Trust SIMPLNX. Supply a positive value for the selected radius mode.

**Evidence:** Current `ComputeNeighborhoodsFilter::preflightImpl()` contains both positivity checks. `SimplnxCore::ComputeNeighborhoods_InvalidSearchRadius` asserts both error codes with zero values. DREAM.3D v6.5.171 `FindNeighborhoods::dataCheck()` contains no equivalent check and has no absolute-radius mode. On 2026-10-22, the local proof version matches DREAM3D-NX in `neutral_absolute_oracle`, `multiples_analytical_oracle`, and `d1_boundary_box_quantization` with positive radius parameters; these cases do not check invalid inputs, and the package has no D3 case or D3 proof patch (`vv_work:ComputeNeighborhoodsFilter/ReadMe.md`, `vv_work:ComputeNeighborhoodsFilter/comparison/LegacyComparison.md`, `vv_work:ComputeNeighborhoodsFilter/notes/proof_commits.md`).

---

## Part 2 — SIMPLNX defects and behavior changes

### ComputeNeighborhoodsFilter-D4

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D4` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Multiples mode used the same radius for every feature and changed neighbor membership relative to each feature's own diameter.

**Root cause:** Bug. The Euclidean rewrite used `averageDiameter * multiplier / 2` for every feature. It removed the per-feature reach and introduced a division by two. The corrected algorithm uses `equivalentDiameter[i] * multiplier`; average diameter sizes the search grid only.

**Affected users:** Users of multiples mode in DREAM3D-NX v7.4.1. Large features could lose neighbors because the radius no longer followed their own diameter.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Recompute neighborhood arrays produced by the affected release.

**Evidence:** Direct inspection of `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeNeighborhoods.cpp` on all release branches finds per-feature bin limits in `upstream/release/v7.0.0` through `upstream/release/v7.4.0`, the global half-diameter radius in `upstream/release/v7.4.1`, and per-feature radii in `upstream/release/v7.4.2`. PR #1485 (`f34c38916`) introduced the regression; PR #1652 (`d967a166a`) fixed it. DREAM.3D tag `v6.5.171`, `Source/Plugins/Statistics/StatisticsFilters/FindNeighborhoods.cpp`, uses per-feature limits. `SimplnxCore::ComputeNeighborhoods_MultiplesAnalyticalOracle` asserts counts `{0,3,0,0,1,0}` and asymmetric membership. All ordinary tests pass in CI at `419f9db35`.

---

### ComputeNeighborhoodsFilter-D5

| Field | Value |
|---|---|
| **Deviation ID** | `ComputeNeighborhoodsFilter-D5` |
| **Filter UUID** | `924c10e3-2f39-4c08-9d7a-7fe029f74f6d` |
| **Status** | active |
| **Affected released versions** | DREAM3D-NX v7.4.1. DREAM.3D v6.5.171 was not affected. |
| **Fixed in** | DREAM3D-NX v7.4.2 |

**Symptom:** Background feature 0 could have a nonzero neighborhood count while its output neighbor list stayed empty.

**Root cause:** Bug. The Euclidean rewrite started the source range at 0 without the earlier worker guard that skipped background. It could find real features near the background centroid. Final list writes still started at feature 1. The corrected algorithm starts source searches at feature 1 and writes an explicit empty list for feature 0. Candidate insertion also excludes feature 0.

**Affected users:** Users of DREAM3D-NX v7.4.1 whose background centroid is near a real feature. Background counts could disagree with the corresponding list size.

**Recommendation:** Trust SIMPLNX in DREAM3D-NX v7.4.2 or later. Recompute affected arrays so background has zero count and an empty list.

**Evidence:** The same release algorithm files checked for D4 show an explicit `start == 0` guard through `upstream/release/v7.4.0`. `upstream/release/v7.4.1` has `Range(0, totalFeatures)` without that guard and writes lists from index 1. `upstream/release/v7.4.2` has `Range(1, totalFeatures)` and writes lists from index 0. PR #1652 (`d967a166a`) includes the background correction. DREAM.3D v6.5.171 `FindNeighborhoodsImpl::convert()` skips source 0. Both analytical tests place background at feature 1's centroid and assert its zero count and empty list.
