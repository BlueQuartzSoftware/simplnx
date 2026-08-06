# Deviations from Pre-SIMPLNX Implementation: ComputeGroupingDensityFilter

> **Note (this filter only):** the title has been re-scoped from the template
> default "Deviations from DREAM3D 6.5.171." `ComputeGroupingDensityFilter` is
> a port of the pre-SIMPLNX `FindGroupingDensity` (SIMPL UUID
> `708be082-8b08-4db2-94be-52781ed4d53d`) on `tuks188/DREAM3D`
> `feature/770_Grouping_Density` — not the shipped 6.5.171 baseline.
> The legacy filter was never officially released, but a small set of customers
> consumed a custom DREAM3D 6.5.x build that included it. See the V&V report's
> Algorithm Relationship section for context.

## Headline

**No deviations observed.** All 4 `(UseNonContiguousNeighbors, FindCheckedFeatures)`
configurations of `ComputeGroupingDensityFilter` (SIMPLNX) and `FindGroupingDensity`
(a local build of the legacy DREAM3D 6.5 source with the feature-branch sources) produced **bit-identical**
`GroupingDensities` and `CheckedFeatures` arrays when run against identical
input data.

## Comparison method

| | |
|---|---|
| **Comparison type** | Runtime A/B (not analytical — both implementations actually executed) |
| **Identical inputs** | Same legacy-format `.dream3d` file consumed by both sides (no per-implementation data prep) |
| **Tolerance** | Bit-identical (`np.array_equal` on raw float32 / int32 bytes — stricter than any float epsilon) |
| **Configurations exercised** | All 4 — `(NC, CF) ∈ {(0,0), (0,1), (1,0), (1,1)}` covering every template specialization of `FindDensityGrouping<UseNonContiguousNeighbors, FindCheckedFeatures>()` |
| **Comparison driver** | `src/Plugins/SimplnxCore/vv/comparisons/ComputeGroupingDensityFilter/compare_outputs.py` |
| **Comparison fixture archive** | `compute_grouping_densities_v2.tar.gz` |
| **Archive SHA512** | `3aaabb63c4fa16f7fa192ae4ee9dbba9394ec7f1cd19aff55e399a624d495a3a778c7f6f282911f681e85cea99e4c6d15344274e9107f337af7d4a19f93784ff` |

### Inputs

Hand-built minimal dataset matching `createTestDataStructure()` in
`test/ComputeGroupingDensityTest.cpp` (lines 62-141):

| Path | Values |
|---|---|
| `DataContainer/FeatureData/Volumes` (Float32, 6) | `[0, 10, 20, 15, 25, 30]` |
| `DataContainer/FeatureData/ParentIds` (Int32, 6) | `[0, 1, 1, 1, 2, 2]` |
| `DataContainer/FeatureData/ContiguousNeighborList` | `[[], [2], [1,3], [2,4], [3,5], [4]]` |
| `DataContainer/FeatureData/NonContiguousNeighborList` | `[[], [4], [5], [], [1], [2]]` |
| `DataContainer/ParentData/ParentVolumes` (Float32, 3) | `[0, 45, 55]` |

### Per-configuration result

| (NC, CF) | `GroupingDensities` (both sides, float32) | `CheckedFeatures` (both sides, int32) | Diff |
|---|---|---|---|
| (0, 0) | `[0.0, 0.6428571343421936, 0.7857142686843872]` | array not produced (CF=false) | bit-identical |
| (0, 1) | `[0.0, 0.6428571343421936, 0.7857142686843872]` | `[0, 1, 1, 2, 2, 2]` | bit-identical |
| (1, 0) | `[0.0, 0.44999998807907104, 0.550000011920929]` | array not produced (CF=false) | bit-identical |
| (1, 1) | `[0.0, 0.44999998807907104, 0.550000011920929]` | `[0, 2, 2, 2, 2, 2]` | bit-identical |

The numerical values match the SIMPLNX unit-test hand calculations exactly
(45/70, 55/70 for the contiguous-only cases; 45/100, 55/100 for the
non-contiguous-included cases — these are the exact float32 representations).

### Build/source provenance

| Side | Build / source |
|---|---|
| Legacy (local rebuild) | Local build of the legacy DREAM3D 6.5 source (`tuks188/DREAM3D` `feature/770_Grouping_Density` sources pulled in). `FindGroupingDensity.{cpp,h}` placed at `Source/Plugins/Statistics/StatisticsFilters/`. |
| SIMPLNX | `Workspace6/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel/Bin/nxrunner` (1.7.0 build 2026/05/07). Filter at `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.{hpp,cpp}` and `.../Filters/ComputeGroupingDensityFilter.{hpp,cpp}`. |

## Algorithmic deltas observed (none affect output)

For audit completeness, the SIMPLNX port made the following structural changes
versus the legacy `execute()` body. The runtime A/B above confirms each is
output-preserving:

1. **Container swap:** `QVector<int32_t>` totalCheckList (linear `.contains()`, O(n²) per parent)
   → `std::unordered_set<int32>` (O(1) membership). **No output change** — both have set-membership semantics on the same set of feature ids; floating-point accumulation order is unaffected.
2. **Boolean dispatch:** runtime `if (m_FindCheckedFeatures == true)` inside the inner loop
   → compile-time `if constexpr (FindingCheckedFeatures)` template specialization. **No output change.**
3. **Neighbor-list unroll:** runtime `for(k=0; k<kmax; ++k)` over contiguous + optional non-contiguous lists
   → unrolled `processNeighborListData()` call + compile-time `if constexpr (UsingNonContiguousNeighbors)`. **No output change** — same neighbor-walk order.
4. **Conditional allocation:** `QVector<float> checkedfeaturevolumes(numfeatures, 0.0f)` always allocated
   → conditionally allocated only when `FindCheckedFeatures==true`. **No output change** (legacy zeros were never read when the flag was false).
5. **Cancellation support:** added `m_ShouldCancel` check in the outer parent loop. (Legacy had no cancel support; A/B was run without cancel so this code path is not exercised by the comparison.)
6. **Progress reporting:** added `ThrottledMessenger` per-parent progress. (Legacy emits one terminal "Complete" message; SIMPLNX emits per-parent updates. Affects logs only.)

Both implementations also behave the same way for the `FindCheckedFeatures=false`
cases — **neither** writes the `CheckedFeatures` output array (it's omitted, not
written-and-empty).

## Migration recommendation for customers of the legacy custom build

**Trust SIMPLNX. Output is bit-identical to the legacy filter for matched
inputs across all 4 configurations.** No migration tolerance band is required.
Downstream consumers can expect numerically identical `GroupingDensities` and
`CheckedFeatures` arrays.
