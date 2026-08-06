# V&V Report: ComputeGroupingDensityFilter

|           |                  |
|-----------|------------------|
| Plugin    | SimplnxCore      |
| SIMPLNX UUID | ff46afcf-de32-4f37-98bc-8f0fd4b3c122 |
| DREAM3D 6.5.171 equivalent | `FindGroupingDensity` (SIMPL UUID `708be082-8b08-4db2-94be-52781ed4d53d`) — *unreleased pre-simplnx implementation* on `tuks188/DREAM3D` `feature/770_Grouping_Density`; never merged into `v6_5_171`. Served as the port source. |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | **COMPLETE — 2026-05-27** |
| Sign-off | Michael Jackson (BlueQuartz Software), 2026-05-27 |

## At a glance

| Aspect                 | Current state            |
|------------------------|--------------------------|
| Algorithm Relationship | **Port** of legacy `FindGroupingDensity::execute()` from `tuks188/DREAM3D` `feature/770_Grouping_Density` (an un-merged feature branch on a contributor fork — never shipped in any DREAM3D 6.5.x release). Same SIMPL UUID retained. Six port-time deltas documented (`QVector→unordered_set` membership swap; runtime `if`→`if constexpr` specializations; conditional `checkedFeatureVolumes` allocation; added `m_ShouldCancel` + throttled progress feedback) — all performance/cancel/progress-only, none change output. **Caveat:** a small number of customers consumed a custom DREAM3D 6.5.x build that included this legacy filter, so the diff-explanation policy applies with a narrower migrant audience than usual. |
| Oracle (confirmed)     | **Class 1 (Analytical) primary** — hand-derivation embedded in the Oracle section (set-union sums + ratio division on the 5-feature v2 hand-built fixture; 4-configuration expected-output table). **Class 4 (Invariant) companion** — sentinel `-1.0f` on empty parents; `density[0] == 0.0f`; `density[i] ≤ 1.0f`; `density[i] > 0 ∨ == -1.0f`; `CheckedFeatures` range — asserted inline. **Second-engineer review skipped** (recorded reason): 5-feature hand-built fixture arithmetic is high-school-level set-union + ratio; bit-identical cross-check against an independently-built legacy serves as external validation.                                                  |
| Code paths enumerated  | **7 of 7 paths** exercised: 4 template specializations of `FindDensityGrouping<UseNonContiguousNeighbors, FindCheckedFeatures>` (all 4 covered by the DYNAMIC_SECTION A/B test) + empty-parent sentinel (Class 4 edge-case test) + cancellation check (`m_ShouldCancel` in outer parent loop — structurally covered, not exercised by V&V fixtures by design) + preflight error paths (4 dedicated error tests).                                          |
| Tests today            | **7 TEST_CASEs**, 100% pass. 1 DYNAMIC_SECTION 4-config exemplar A/B (covers all 4 `(UseNonContiguous, FindCheckedFeatures)` template specializations) + 1 Class 4 empty-parent sentinel test + 4 preflight-error tests (one each for -15670/-15671/-15672/-15673) + 1 SIMPL backwards-compat test. **Restructured from 8 → 7 tests**: 5 redundant pre-V&V tests (4 `(NC,CF)` execution variants + 1 v1-exemplar consumer) replaced by the single DYNAMIC_SECTION A/B; added: empty-parent sentinel + preflight error -15672 (closed a coverage gap); kept: 3 existing preflight errors + SIMPL backwards-compat.                             |
| Exemplar archive       | `compute_grouping_densities_v2.tar.gz` (SHA512 `3aaabb63…3784ff`, wired into `test/CMakeLists.txt`). **Replaces** v1 archive `compute_grouping_densities.tar.gz` (retired from this filter's tests). The v2 archive carries inline provenance: input file + all 4 legacy/SIMPLNX outputs + comparison script + comparison report + legacy/SIMPLNX pipelines + hand-review sign-off ReadMe. **Provenance sidecar:** `vv/provenance/ComputeGroupingDensityFilter.md` — records the four-hand authorship chain (Claude Opus 4.6 initial dataset → MAJ structural sign-off → Claude Opus 4.7 coverage review → Claude Opus 4.7 Python generator + compare scripts) and the v1→v2 circular-oracle disposition.    |
| Legacy comparison      | **Empirical A/B against locally-rebuilt legacy** `FindGroupingDensity` (a local build of the legacy DREAM3D 6.5 source with the `feature/770_Grouping_Density` sources pulled in). All 4 `(UseNonContiguousNeighbors, FindCheckedFeatures)` configurations produce **BIT-IDENTICAL** `GroupingDensities` and `CheckedFeatures` output between SIMPLNX and the rebuilt legacy. **0 deviations.** See `vv/deviations/ComputeGroupingDensityFilter.md` for the per-configuration result table, comparison method, build provenance, and the migration recommendation for the legacy-custom-build customers (*trust SIMPLNX — output is bit-identical*).             |
| Bug flags              | **None — clean Port.** Algorithm verified correct as-is; no SIMPLNX-side algorithmic bug surfaced or resolved. **Cleanup-only changes applied**: `[SimplnxReview]`→`[SimplnxCore]` test-tag fix (7 tests now use the correct `[SimplnxCore][ComputeGroupingDensityFilter]` tag pair); code-comment cleanup applied to algorithm + filter source (sentinel `-1.0f` documented inline; deleted-special-members trailing comments removed; throwaway-placeholder pattern explained; `FindDensitySpecializations` + `FindDensityGrouping` class docs added); tie-break behavior surfaced in the user-facing filter doc.                                       |
| V&V phase              | **COMPLETE — 2026-05-27.** All Resolution-table items confirmed: Algorithm Relationship (Port — six port-time deltas documented), Oracle (Class 1 + Class 4), Legacy comparison (0 deviations across all 4 configs), test-tag cleanup, v2 archive published with sign-off ReadMe, test inventory restructured 8→7, no deviations, algorithm review done, filter docs updated (Required Input Sources + MyST cross-links + volume units + tie-break behavior + italicized sentinel + new ComputeGroupingDensity_Infographic SVG+PNG), verification archive materially captured by the v2 GitHub Data_Archive release.  |

## Summary

- This **Filter** computes a **Grouping Density** value for each **Parent Feature** in a hierarchical reconstruction. Hierarchical reconstructions involve more than one level of segmentation, creating a **Feature** to **Parent Feature** relationship (e.g., grains grouped into reconstructed parent grains).
- The filter was verified by generating a small data set that exercises each code path and combination of featureIds. The final calculations were done by hand and then verified by executing the filter.
- The result is that the filter generates the expected output values


## Algorithm Relationship


*Classification:* **Port**

*Evidence:* The SIMPLNX algorithm at `Algorithms/ComputeGroupingDensity.cpp` (219 lines) is a line-by-line translation of the legacy `FindGroupingDensity::execute()` on `tuks188/DREAM3D` `feature/770_Grouping_Density` (469-line `.cpp` file; ~80-line algorithm body). Identical control flow (nested parent×feature loops + neighbor-list walks), identical sentinel (`-1.0f` when `totalCheckVolume == 0.0f`), identical density formula (`curParentVolume / totalCheckVolume`), and a preserved-from-legacy variable-name lineage (`totalCheckVolume` → `totalFeatureCheckVolume`, `checkedfeaturevolumes` → `checkedFeatureVolumes`). Same SIMPL UUID retained via `SimplnxCoreLegacyUUIDMapping.hpp` + SIMPL conversion fixture at `test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json`. **Important caveat:** the legacy filter was never officially released in any DREAM3D 6.5.x — it lived on an un-merged feature branch on a contributor fork. However, a small number of important customers consumed a custom DREAM3D 6.5.x build that included this filter, and those customers have downstream data dependent on its output. Therefore the policy's diff-explanation purpose **does** apply to this filter, just with a narrower migrant audience than usual. The legacy comparison evidence is captured in `vv/deviations/ComputeGroupingDensityFilter.md` via an A/B run against a local rebuild of the legacy DREAM3D 6.5 source (with the feature-branch sources pulled in). Verification still requires an independent oracle (see Oracle section).

*Port-time deltas that do not change output (defensible "Port" rather than "Minor changes"):*

1. `QVector<int32_t>` totalCheckList (linear `.contains()`, O(n²) per parent) → `std::unordered_set<int32>` (O(1) membership, O(n) per parent) — performance, no behavior change.
2. Runtime `if (m_FindCheckedFeatures == true)` in the inner loop → `if constexpr (FindingCheckedFeatures)` template specialization — performance, no behavior change.
3. Runtime `for(k=0; k<kmax; ++k)` over contiguous + optional non-contiguous neighbor lists → unrolled `processNeighborListData()` call + compile-time `if constexpr (UsingNonContiguousNeighbors)` — performance, no behavior change.
4. `QVector<float> checkedfeaturevolumes(numfeatures, 0.0f)` always allocated → conditionally allocated only when `FindCheckedFeatures==true` — memory savings, no behavior change (legacy zeros were unread when the flag was false).
5. Added: `m_ShouldCancel` check in the outer parent loop (legacy has no cancel support).
6. Added: throttled per-parent progress feedback (legacy emits one terminal "Complete" message).

*Material PRs since baseline (2025-10-01):*

- **#1548** — "FILT: Compute Grouping Density filter added." (merge `30c9b1090`, 2026-02-25) — initial port from the legacy feature branch: filter + algorithm (.hpp/.cpp), docs (with worked example + 3 figures), `FromSIMPLJson()` conversion path, legacy UUID map entry, 431-line test file covering all 4 template specializations + 3 preflight error tests, exemplar archive.
- *(excluded — broad refactor)* #1588 — "ENH: SIMPL Backwards Compatibility Test Redesign" (merge `f854bb636`, 2026-04-22) — on the cross-cutting exclusion list; added only the per-filter SIMPL backwards-compat fixture at `test/simpl_conversion/6_5/ComputeGroupingDensityFilter.json`. No algorithm change.

## Oracle

*Class:* **1 (Analytical)** primary, **4 (Invariant)** companion.

*Applied (Class 1 — Analytical):* Expected outputs are hand-derived from the input definition without reference to any DREAM3D implementation. For each parent index `i ≥ 1`:

1. `assigned = {j : ParentIds[j] == i}`
2. `touched = assigned ∪ {nbr : nbr ∈ contiguousNL[j], j ∈ assigned}` (and additionally `∪ {nbr : nbr ∈ nonContiguousNL[j], j ∈ assigned}` when `UseNonContiguousNeighbors == true`)
3. `totalCheckVolume[i] = Σ Volumes[k] for k ∈ touched`
4. `GroupingDensities[i] = ParentVolumes[i] / totalCheckVolume[i]`, or `-1.0f` sentinel when `totalCheckVolume[i] == 0`

For `CheckedFeatures[k]` (when `FindCheckedFeatures == true`): the parent with the largest `ParentVolumes` among the parents that touched feature `k` (last-writer-wins-on-greater-volume semantics in the algorithm).

Hand-derivation on the v2 hand-built fixture (`Volumes = [0,10,20,15,25,30]`, `ParentIds = [0,1,1,1,2,2]`, `ParentVolumes = [0,45,55]`, contiguous chain `1↔2↔3↔4↔5`, non-contiguous pairs `1↔4` and `2↔5`):

| Config (NC, CF) | Parent 1 touched | Σ Vol | `density[1]` | Parent 2 touched | Σ Vol | `density[2]` |
|---|---|---|---|---|---|---|
| (0, *) | {1,2,3,4} | 70 | `45/70` ≈ `0.6428571` | {3,4,5} | 70 | `55/70` ≈ `0.7857143` |
| (1, *) | {1,2,3,4,5} | 100 | `0.45` | {1,2,3,4,5} | 100 | `0.55` |

CheckedFeatures derivations (when CF=1): NC=0 → `[0,1,1,2,2,2]` (parent 1 claims features {1,2}; parent 2 claims {3,4} as the larger-volume parent overriding parent 1's earlier claim, plus its own {5}). NC=1 → `[0,2,2,2,2,2]` (both parents touch all 5; parent 2 wins on every feature). Full derivation in `vv/comparisons/ComputeGroupingDensityFilter/README.md` and the v2 archive's `README.md`.

*Applied (Class 4 — Invariant):* Derivable properties any valid output must satisfy, asserted inline in test code (`ComputeGroupingDensityTest.cpp` Exemplar A/B + Empty-parent edge case):

- `GroupingDensities[0] == 0.0f` (placeholder parent never touched)
- For `i ≥ 1`: `GroupingDensities[i] > 0.0f ∨ == -1.0f` (positive or sentinel)
- For `i ≥ 1`: `GroupingDensities[i] ≤ 1.0f` (totalCheckVolume always includes the parent's own features → ≥ ParentVolumes[i])
- `CheckedFeatures[k] ∈ {0, …, numParents-1}` when produced; `CheckedFeatures[0] == 0`
- Empty-parent sentinel asserted directly: `REQUIRE(densities[i] == -1.0f)` when parent `i` has no assigned features

*Encoded:*

- **Class 1 (Analytical)**: `test/ComputeGroupingDensityTest.cpp::"Exemplar A/B — all 4 configurations"` — 4 fixtures (`NC0_CF0`, `NC0_CF1`, `NC1_CF0`, `NC1_CF1`). Bit-exact `CompareDataArrays<float32>` and `<int32>` against the v2 exemplar (`compute_grouping_densities_v2.tar.gz`) whose `GroupingDensities_*` and `CheckedFeatures_*` arrays equal the hand-derivation above to float32 precision.
- **Class 4 (Invariant)**: same test (inline invariant predicates run for all 4 fixtures); plus `test/ComputeGroupingDensityTest.cpp::"Empty-parent edge case (-1.0f sentinel)"` — 2 assertions for `density[1] == 1.0` (parent with assigned features) and `density[2] == -1.0f` (sentinel).

6 fixture assertions total, all pass at the verified commit.

*Second-engineer review:* **Skipped — recorded reason:** Class 1 derivation is set-union sums + ratio division on a 5-feature hand-built fixture (high-school arithmetic). External cross-validation was obtained via the independently-authored legacy `FindGroupingDensity` implementation (`tuks188/DREAM3D` `feature/770_Grouping_Density` sources rebuilt locally): the Phase 9 A/B comparison produced bit-identical agreement across all 4 configurations (see `vv/deviations/ComputeGroupingDensityFilter.md` and `compute_grouping_densities_v2/results/ab_comparison_report.txt`). Any oracle-derivation error would have surfaced as a Deviation. Formal second-engineer review of a 5-feature analytical oracle was not justified given this cross-check.

## Code path coverage


*7 of 7 paths exercised.*

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.cpp`

The algorithm dispatches on two booleans, producing 4 template specializations of `FindDensityGrouping<UseNonContiguousNeighbors, FindCheckedFeatures>()`. Two additional runtime branches handle the empty-parent sentinel and cancellation. Preflight error paths are tested separately at the filter level.

| Path | Test case |
|---|---|
| `UseNonContiguousNeighbors=true, FindCheckedFeatures=true` (full path, both neighbor lists + per-feature parent tracking) | `Exemplar A/B — all 4 configurations` (DYNAMIC_SECTION `NC1_CF1`) |
| `UseNonContiguousNeighbors=true, FindCheckedFeatures=false` (both neighbor lists, no per-feature parent tracking) | `Exemplar A/B — all 4 configurations` (DYNAMIC_SECTION `NC1_CF0`) |
| `UseNonContiguousNeighbors=false, FindCheckedFeatures=true` (contiguous neighbors only + per-feature parent tracking) | `Exemplar A/B — all 4 configurations` (DYNAMIC_SECTION `NC0_CF1`) |
| `UseNonContiguousNeighbors=false, FindCheckedFeatures=false` (contiguous neighbors only, no per-feature parent tracking) | `Exemplar A/B — all 4 configurations` (DYNAMIC_SECTION `NC0_CF0`) |
| Edge: `totalFeatureCheckVolume == 0.0f` for a parent → density sentinel `-1.0f` written at line 114 | `Empty-parent edge case (-1.0f sentinel)` |
| Cancellation: `m_ShouldCancel` checked inside the parent-id outer loop (line 76); early return without writing further densities | *Not directly tested — structurally covered; requires cancel-signal injection (low-value guard).* |
| Preflight errors: invalid / mismatched input array paths → `-15670`/`-15671`/`-15672`/`-15673` | 4 dedicated preflight-error tests (one per code) |

## Test inventory

| Test case | Status | Notes |
|---|---|---|
| `SimplnxCore::ComputeGroupingDensityFilter: Exemplar A/B — all 4 configurations` | new-for-V&V | DYNAMIC_SECTION over `NC0_CF0`/`NC0_CF1`/`NC1_CF0`/`NC1_CF1`; bit-exact `CompareDataArrays<float32>`/`<int32>` against the v2 exemplar (equals the Class 1 hand-derivation) + inline Class 4 invariants. Replaces 5 redundant pre-V&V tests (4 execution variants + 1 v1-exemplar consumer). |
| `SimplnxCore::ComputeGroupingDensityFilter: Empty-parent edge case (-1.0f sentinel)` | new-for-V&V | Class 4 — `density[1] == 1.0` (parent with features) and `density[2] == -1.0f` (empty-parent sentinel). |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight error -15670` | kept | Invalid/mismatched input array path. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight error -15671` | kept | Invalid/mismatched input array path. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight error -15672` | new-for-V&V | Closed a preflight coverage gap. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight error -15673` | kept | Invalid/mismatched input array path. |
| `SimplnxCore::ComputeGroupingDensityFilter: SIMPL Backwards Compatibility` | kept | `DYNAMIC_SECTION` over SIMPL 6.5 conversion fixture; UUID + argument conversion only. |

Restructured 8 → 7 tests. All 7 use the `[SimplnxCore][ComputeGroupingDensityFilter]` tag (corrected from a
`[SimplnxReview]` mis-tag) and pass at the verified commit.

## Exemplar archive

- **Archive:** `compute_grouping_densities_v2.tar.gz` — **replaces** v1 `compute_grouping_densities.tar.gz` (retired from this filter's tests as a circular oracle). Carries inline provenance: input file + all 4 legacy/SIMPLNX outputs + comparison script + report + both pipelines + hand-review sign-off ReadMe.
- **SHA512:** `3aaabb63c4fa16f7fa192ae4ee9dbba9394ec7f1cd19aff55e399a624d495a3a778c7f6f282911f681e85cea99e4c6d15344274e9107f337af7d4a19f93784ff`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeGroupingDensityFilter.md` — records the authorship chain and the v1→v2 circular-oracle disposition.

## Deviations from DREAM3D 6.5.171

**Baseline note (this filter):** the legacy equivalent is the pre-SIMPLNX `FindGroupingDensity` on `tuks188/DREAM3D` `feature/770_Grouping_Density`, which was never shipped in any DREAM3D 6.5.x release (a few customers consumed it via a custom 6.5.x build) — so "DREAM3D 6.5.171" here denotes that feature-branch source rebuilt locally, not the shipped 6.5.171 baseline. See the Algorithm Relationship section for context.

**No deviations observed.** Runtime A/B comparison run on the
`compute_grouping_densities_v2.tar.gz` fixture: all 4
`(UseNonContiguousNeighbors, FindCheckedFeatures)` combinations of
`ComputeGroupingDensityFilter` (SIMPLNX) and `FindGroupingDensity` (a local
build of the legacy DREAM3D 6.5 source with the feature-branch sources) produced
**bit-identical** `GroupingDensities` and `CheckedFeatures` output. See
`vv/deviations/ComputeGroupingDensityFilter.md` for the full per-configuration
result table, comparison method, build provenance, and the migration
recommendation for legacy-custom-build customers (*trust SIMPLNX, output is
bit-identical*).

| Fixture | `compute_grouping_densities_v2.tar.gz` |
|---|---|
| SHA512 | `3aaabb63c4fa16f7fa192ae4ee9dbba9394ec7f1cd19aff55e399a624d495a3a778c7f6f282911f681e85cea99e4c6d15344274e9107f337af7d4a19f93784ff` |
| Driver script | `compute_grouping_densities_v2compare_outputs.py` |
