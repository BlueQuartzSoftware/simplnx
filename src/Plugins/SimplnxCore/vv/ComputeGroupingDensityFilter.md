# V&V Report: ComputeGroupingDensityFilter

|        |              |
|--------|--------------|
| Plugin | SimplnxCore |
| SIMPLNX UUID | `ff46afcf-de32-4f37-98bc-8f0fd4b3c122` |
| SIMPLNX Human Name | Compute Grouping Densities |
| DREAM.3D v6.5.171 equivalent | None. `FindGroupingDensity` exists in a local proof version only. |
| Verified commit | *<filled at SBIR deliverable assembly>* |
| Status | COMPLETE |
| Sign-off | Michael Jackson, 2026-05-27; second engineer: Nathan Young, 2026-06-09 |

## At a glance

| Aspect | Current state |
|--------|---------------|
| Algorithm Relationship | Port of `FindGroupingDensity` from a local proof version. DREAM.3D v6.5.171 has no equivalent filter. |
| Oracle (confirmed) | Class 1 set-union sums and volume ratios, with Class 4 invariants, are encoded in four archive configurations and one empty-parent fixture. All pass in CI. |
| Code paths enumerated | 19 of 25 exercised. Gaps include cancellation, failed neighbor retrieval, rejected parent claims, an empty parent loop and the HDF5 boundary path. |
| Tests today | 8 test cases: one four-configuration archive comparison, one empty-parent case, four preflight rejections, one hidden HDF5 fixture and one SIMPL conversion case. |
| Exemplar archive | `compute_grouping_densities_v2.tar.gz` supplies inputs and outputs for all four option combinations. It replaces `compute_grouping_densities.tar.gz`. |
| Legacy comparison | Not run — DREAM.3D v6.5.171 has no equivalent filter. A local proof version matched all 4 option combinations bit for bit on 2026-05-27; the algorithm changed afterward. |
| Bug flags | None |
| V&V phase | COMPLETE. Analytical values, invariants, input rejection and parameter conversion are covered. |

## Summary

`ComputeGroupingDensityFilter` divides each parent volume by the summed volume of its assigned features and their unique selected neighbors. Class 1 expectations check all four option combinations, and Class 4 checks cover output bounds and the empty-parent sentinel. All pass in CI.

## Algorithm Relationship

*Classification:* Port

*Evidence:* `src/Plugins/SimplnxCore/src/SimplnxCore/SimplnxCoreLegacyUUIDMapping.hpp` maps `FindGroupingDensity` to this filter; the archive README identifies the port source as a local proof version, with the same set-union density formula.

## Oracle

*Class:* 1 (Analytical), with 4 (Invariant).

*Applied:* Sum each unique assigned or neighboring feature volume once, divide the parent volume by that sum, and select the largest positive-volume parent for each checked feature; a zero sum gives density `-1.0f`.

*Encoded:* `src/Plugins/SimplnxCore/test/ComputeGroupingDensityTest.cpp::SimplnxCore::ComputeGroupingDensityFilter: Exemplar A/B — all 4 configurations` checks four configurations; `Empty-parent edge case (-1.0f sentinel)` adds one fixture. All pass in CI.

*Second-engineer review:* Skipped — the recorded rationale uses the five-feature analytical derivation and the local proof comparison as the cross-check.

## Bugs found and fixed

None.

## Code path coverage

19 of 25 paths exercised. The uncovered rows identify the remaining gaps.

Source: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ComputeGroupingDensity.cpp` (237 lines) and `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ComputeGroupingDensityFilter.cpp` (290 lines).

Test names below omit the common `SimplnxCore::ComputeGroupingDensityFilter: ` prefix. NC means Use Non-Contiguous Neighbors; CF means Find Checked Features.

| # | Phase | Path | Test case |
|---|-------|------|-----------|
| 1 | Dispatch | NC=true, CF=true | `Exemplar A/B — all 4 configurations` — `NC1_CF1` checks densities and checked features |
| 2 | Dispatch | NC=true, CF=false | `Exemplar A/B — all 4 configurations` — `NC1_CF0` checks densities |
| 3 | Dispatch | NC=false, CF=true | `Exemplar A/B — all 4 configurations` — `NC0_CF1` checks densities and checked features |
| 4 | Dispatch | NC=false, CF=false | `Exemplar A/B — all 4 configurations` — `NC0_CF0` checks densities |
| 5 | Finalize | Zero checked volume gives density `-1.0f` | `Empty-parent edge case (-1.0f sentinel)` |
| 6 | Cancellation | Cancel before processing a parent | *Not directly tested. No fixture injects cancellation.* |
| 7 | Preflight | Feature tuple counts differ; reject with `-15671` | `Preflight Error - Feature tuple count mismatch (-15671)` — checks rejection |
| 8 | Preflight | Selected non-contiguous list has a different tuple count; reject with `-15672` | `Preflight Error - NonContiguousNL tuple count mismatch (-15672)` — checks rejection |
| 9 | Preflight | Feature volumes are outside an AttributeMatrix; reject with `-15673` | `Preflight Error - Volumes not in AttributeMatrix (-15673)` — checks rejection |
| 10 | Preflight | Parent volumes are outside an AttributeMatrix; reject with `-15670` | `Preflight Error - Parent Volumes not in AttributeMatrix (-15670)` — checks rejection |
| 11 | Finalize | Positive checked volume gives the parent-to-checked-volume ratio | `Exemplar A/B — all 4 configurations`; `Empty-parent edge case (-1.0f sentinel)` |
| 12 | Iteration | Skip parent and feature index zero | `Exemplar A/B — all 4 configurations` — asserts zero placeholder outputs |
| 13 | Iteration | Feature belongs to another parent; skip its neighbor traversal | `Exemplar A/B — all 4 configurations` — two parents with different volumes |
| 14 | Accumulation | Assigned feature is new to the set; add its volume | `Empty-parent edge case (-1.0f sentinel)` — empty neighbor lists |
| 15 | Accumulation | Assigned feature is already in the set; do not add its volume again | `Exemplar A/B — all 4 configurations` — chain neighbors include later assigned features |
| 16 | Accumulation | Neighbor is new to the set; add its volume | `Exemplar A/B — all 4 configurations` |
| 17 | Accumulation | Neighbor is already in the set; do not add its volume again | `Exemplar A/B — all 4 configurations` — reciprocal chain neighbors |
| 18 | Accumulation | Empty neighbor list; skip the neighbor loop | `Empty-parent edge case (-1.0f sentinel)` |
| 19 | Neighbor access | `getValue()` fails; return from the neighbor helper | *Not directly tested. No fixture injects a neighbor retrieval failure.* |
| 20 | Checked features | Assigned feature accepts a larger positive parent volume | `Exemplar A/B — all 4 configurations` — CF=true variants |
| 21 | Checked features | Neighbor accepts a larger positive parent volume | `Exemplar A/B — all 4 configurations` — parent 2 replaces parent 1 on shared features |
| 22 | Checked features | Assigned feature rejects a parent volume less than or equal to its current claim | *Not directly tested. No fixture makes this comparison false for a newly visited assigned feature.* |
| 23 | Checked features | Neighbor rejects a parent volume less than or equal to its current claim | *Not directly tested. No fixture gives an earlier claimant an equal or larger volume.* |
| 24 | Iteration | No parent after index zero; skip the parent loop | *Not directly tested. Every execution fixture has nonzero parent indices.* |
| 25 | Storage | Traverse neighbor lists across an HDF5 chunk boundary | *Not directly tested. The hidden `real HDF5 neighbor-list chunk oracle` requires the HDF5 store provider.* |

## Test inventory

The seven ordinary cases are covered by the supplied Linux, macOS and Windows CI result. All pass in CI.

| Test case | Status | Notes |
|-----------|--------|-------|
| `SimplnxCore::ComputeGroupingDensityFilter: Exemplar A/B — all 4 configurations` | new-for-V&V | Checks `NC0_CF0`, `NC0_CF1`, `NC1_CF0` and `NC1_CF1`. Compares densities with absolute tolerance `0.0001` and checked features exactly; also checks placeholder values, density bounds, parent-ID bounds and tuple dimensions. Replaces the v1 archive consumer and four redundant execution cases. |
| `SimplnxCore::ComputeGroupingDensityFilter: Empty-parent edge case (-1.0f sentinel)` | new-for-V&V | Constructs two assigned features and one empty parent. Checks density 1 with relative tolerance `0.0001`, density `-1.0f` exactly and output tuple dimensions. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - Feature tuple count mismatch (-15671)` | kept | Supplies ten ParentIds tuples and six volume/list tuples. Asserts invalid preflight, without checking the error code. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - NonContiguousNL tuple count mismatch (-15672)` | new-for-V&V | Supplies four non-contiguous list tuples and six feature tuples. Asserts invalid preflight, without checking the error code. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - Volumes not in AttributeMatrix (-15673)` | kept | Places feature volumes directly under the geometry. Asserts invalid preflight, without checking the error code. |
| `SimplnxCore::ComputeGroupingDensityFilter: Preflight Error - Parent Volumes not in AttributeMatrix (-15670)` | kept | Places parent volumes directly under the geometry. Asserts invalid preflight, without checking the error code. |
| `SimplnxCore::ComputeGroupingDensityFilter: real HDF5 neighbor-list chunk oracle` | new-for-V&V | Hidden fixture. Encodes all four option combinations, duplicate-neighbor suppression, densities `0.75` or `0.5`, the empty-parent sentinel, exact checked-feature IDs and absence of the disabled output. |
| `SimplnxCore::ComputeGroupingDensityFilter: SIMPL Backwards Compatibility` | kept | The `SIMPL 6.5 (UUID)` dynamic section checks filter identity, empty comments and nine converted arguments. It does not execute the algorithm. |

## Exemplar archive

- **Archive:** `compute_grouping_densities_v2.tar.gz`
- **SHA512:** `3aaabb63c4fa16f7fa192ae4ee9dbba9394ec7f1cd19aff55e399a624d495a3a778c7f6f282911f681e85cea99e4c6d15344274e9107f337af7d4a19f93784ff`
- **Provenance:** `src/Plugins/SimplnxCore/vv/provenance/ComputeGroupingDensityFilter.md`

## Deviations from DREAM.3D v6.5.171

Not applicable: DREAM.3D v6.5.171 has no equivalent filter. There are no Part 1 entries in `vv/deviations/ComputeGroupingDensityFilter.md`.

| Deviation | Observed difference |
|-----------|---------------------|
