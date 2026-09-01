# Require Minimum Number of Neighbors

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes **Features** that have fewer contiguous neighboring **Features** than the selected minimum. It uses a precomputed *Number of Neighbors* array, which is normally created by the [Compute Feature Neighbors](./ComputeFeatureNeighborsFilter.md) **Filter**. **Feature** tuple 0 is retained as the background tuple.

The most common use case is cleaning up isolated single-feature islands left over from segmentation -- features whose surroundings turned out to be too sparsely connected to support a meaningful grain. After flagged features are removed, the remaining features grow outward via [isotropic coarsening](RequireMinimumSizeFeaturesFilter.md) until every cell is reassigned.

The threshold is a **count of contiguous neighbors** (a non-negative integer). Setting the threshold to 0 removes nothing. Setting it larger than the maximum number of neighbors any feature has produces an error (since all features would be removed). Inspect the *Number of Neighbors* output of [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) before choosing a threshold.

When *Apply to Single Phase Only* is disabled, the minimum is applied to every non-background **Feature**. When it is enabled, only **Features** belonging to the selected *Phase Index* are tested against the minimum; **Features** in other **Ensembles** remain active. This is useful when isolated features are noise in one phase but meaningful in another.

Cells belonging to removed **Features**, along with cells that already have a negative Feature ID, are reassigned iteratively. During each pass, every unresolved **Cell** examines its valid face neighbors and selects a neighbor associated with the most frequently occurring non-negative Feature ID. Vote ties are resolved by the face-neighbor traversal order. Every currently fillable **Cell** is updated during the pass, so the process may fill many cells per iteration. This produces an isotropic, face-connected growth of retained **Features** into the regions left by removed **Features**.

After coarsening completes, inactive feature tuples are removed from the feature **Attribute Matrix** and the remaining Feature IDs are remapped.

## Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Number of Neighbors** -- produced by [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md).
- **Feature Phases** (only when *Apply to Single Phase* is enabled) -- produced by [Compute Feature Phases](ComputeFeaturePhasesFilter.md).

## Required Geometry and Inputs

- An **Image Geometry**.
- A scalar `int32` *Cell Feature Ids* **Data Array** containing exactly one tuple per geometry **Cell**.
- A scalar `int32` *Number of Neighbors* **Data Array** containing one tuple per **Feature**.
- When *Apply to Single Phase Only* is enabled, a scalar `int32` *Feature Phases* **Data Array** with the same tuple count as *Number of Neighbors*.
- Every non-negative Feature ID must be less than the number of tuples in the feature **Attribute Matrix**. Negative Feature IDs are treated as unresolved cells and are reassigned during coarsening.

## Data Modified by the Filter

This **Filter** operates in place and does not create a new geometry or output array.

- *Cell Feature Ids* is always updated, even if it is included in *Cell Arrays to Ignore*.
- Every other nonignored **Data Array** in the cell **Attribute Matrix** copies tuple values from the same selected face neighbor used to reassign the Feature ID.
- Arrays selected by *Cell Arrays to Ignore* are not modified.
- Feature arrays in the parent **Attribute Matrix** of *Number of Neighbors* are compacted to remove inactive feature tuples.
- Feature `NeighborList` arrays are removed because their contents become invalid after the topology changes.

## Validation and Failure Conditions

- Preflight fails with `-55571` when *Cell Feature Ids* does not contain exactly one tuple per **Image Geometry** cell.
- Preflight fails with `-252` when *Number of Neighbors* and the enabled *Feature Phases* array do not have matching tuple counts.
- Execution fails with `-5555` when the selected *Phase Index* is not present in *Feature Phases*.
- Execution fails with `-55569` when the selected minimum would reject every eligible non-background **Feature**.
- Execution fails with `-55567` when a non-negative Feature ID is outside the feature tuple range.
- Execution fails with `-55572` when unresolved cells remain but none has a non-negative face neighbor. This prevents the non-terminating coarsening behavior present in DREAM3D 6.5.171. Ensure that every negative or rejected region is face-connected to at least one retained **Feature**.

Setting *Minimum Number Neighbors* to *0* retains all existing feature tuples, but negative Feature IDs may still be reassigned.

## Algorithm and Performance

The filter first marks features below the neighbor threshold as inactive. It scans the per-cell Feature IDs in fixed-size bulk chunks, setting removed IDs to `-1` and compacting surviving IDs in the same pass. This avoids both per-voxel OOC writes and a second full-volume renumber pass.

The resulting negative voxels are filled iteratively by majority vote among their six face neighbors. Each pass processes one cell array at a time with rolling previous/current/next Z-slices of Feature IDs and target tuples. Feature IDs are updated last, so all transferred arrays use the same iteration snapshot without a cell-count-wide source/destination map. All DataStore access uses bulk slice transfers.

Peak cell-level scratch is `O(Dx * Dy * largest_tuple_width)`: three Feature ID slices plus three slices of the one target array currently being processed. No resident allocation scales with the volume cell count.

## WARNING: Feature Data Will Become Invalid

Modifying Feature IDs changes the feature topology. Previously computed feature-level data may therefore be invalid even after its tuples are compacted. Rerun filters that compute feature data after this **Filter**.

## WARNING: NeighborList Removal

If the feature **Attribute Matrix** contains any `NeighborList` arrays, those arrays are **REMOVED** because the lists are no longer valid. Rerun the [Compute Feature Neighbors](./ComputeFeatureNeighborsFilter.md) **Filter** to recreate them.

% Auto generated parameter table will be inserted here

## Typical Workflow

1. Run the [Compute Feature Neighbors](./ComputeFeatureNeighborsFilter.md) **Filter** to create the *Number of Neighbors* array.
2. Run this **Filter** with a conservative minimum that leaves at least one retained **Feature** connected to every rejected region.
3. Rerun the [Compute Feature Neighbors](./ComputeFeatureNeighborsFilter.md) **Filter** and any other filters that calculate feature-level data.

## Example Pipelines

- `(02) Small IN100 Full Reconstruction`

## Differences from DREAM3D 6.5.171

SIMPLNX safely handles three malformed or unresolved Feature ID conditions that can cause invalid memory access or non-termination in DREAM3D 6.5.171:

- [D1: Negative Feature IDs with a valid face neighbor](../vv/deviations/RequireMinNumNeighborsFilter.md#requireminnumneighborsfilter-d1)
- [D2: Non-negative Feature IDs outside the feature tuple range](../vv/deviations/RequireMinNumNeighborsFilter.md#requireminnumneighborsfilter-d2)
- [D3: Coarsening cannot make progress](../vv/deviations/RequireMinNumNeighborsFilter.md#requireminnumneighborsfilter-d3)

For valid, non-negative, in-range Feature IDs that can be fully coarsened, the verified SIMPLNX and DREAM3D 6.5.171 outputs match.

## Related Filters

- [Compute Feature Neighbors](./ComputeFeatureNeighborsFilter.md)

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
