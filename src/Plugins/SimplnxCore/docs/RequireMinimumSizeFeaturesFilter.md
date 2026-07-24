# Remove Minimum Size Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes small **Features** from a segmented dataset and fills in the space they leave behind by expanding their neighbors. Features whose cell count is below the user-specified *Minimum Allowed Features Size* are erased; the remaining features grow outward into the erased cells until every cell is reassigned to a surviving feature.

This is typically used immediately after segmentation ([Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or similar) to discard spurious single-cell or few-cell "grains" that are almost always noise rather than real features.

### How This Filter Works

1. For each **Feature**, look up its cell count from the input *Feature Num. Cells* array.
2. If the feature's count is below the threshold, mark it for removal. All of its cells are temporarily set to Feature Id 0 ("unassigned").
3. After all small features are removed, the remaining features are *isotropically coarsened* to fill the gaps.

### What is Isotropic Coarsening?

"Isotropic coarsening" is an iterative dilation that grows each surviving feature outward by one cell layer at a time, uniformly in all directions. On each pass, an unassigned cell is given the Feature Id of whichever neighbor feature touches it; if multiple features touch it, one is picked. The process repeats until no unassigned cells remain. The result is that every removed cell is absorbed into the closest surviving feature, with no preferred direction.

### Minimum Size Units

The *Minimum Allowed Features Size* is in **cells** (integer voxel count), not physical units. A threshold of 10 means "remove any feature with fewer than 10 cells," regardless of the image's spacing. To convert a physical-volume threshold into a cell count, divide by the product of the cell spacing (dx * dy * dz).

Entering a number larger than the size of the largest feature produces an error, since every feature would be removed. If the size distribution is unknown, compute feature sizes first with [Compute Feature Sizes](ComputeFeatureSizesFilter.md) and inspect the range.

### Apply to Single Phase

When *Apply to Single Phase* is enabled, the size threshold is applied only to features of the specified phase. Features of other phases are untouched regardless of their size. This is useful when small features are noise in one phase but meaningful in another (e.g., small precipitates in a matrix).

## WARNING: Feature Data Will Become Invalid

By modifying the cell-level Feature Ids, any feature-level data that was previously computed (sizes, centroids, average orientations, etc.) will almost certainly be invalid after this filter runs. Re-run any feature-level computation filters downstream of this one to ensure accurate results.

## WARNING: NeighborList Removal

If the Cell Feature **Attribute Matrix** contains any *NeighborList* arrays, they will be **removed** because the list of neighbors for each surviving feature has changed. Re-run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) afterward to rebuild the neighbor relationships.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Feature Num. Cells** -- the cell count per feature, produced by [Compute Feature Sizes](ComputeFeatureSizesFilter.md).
- **Feature Phases** (only when *Apply to Single Phase* is enabled) -- produced by [Compute Feature Phases](ComputeFeaturePhasesFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

+ (02) Small IN100 Full Reconstruction

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
