# Remove/Extract Flagged Features

## Group (Subgroup)

Processing (Cleanup)

## Description

This **Filter** removes or extracts **Features** that have been flagged by an earlier filter in the pipeline. The user supplies a boolean array at the **Feature** level; features with a value of *false* are targeted for removal (or extraction). Removed features leave gaps in the cell-level **Feature Ids** array, which are then filled by [isotropic coarsening](RequireMinimumSizeFeaturesFilter.md) -- neighboring features grow outward uniformly until every cell is reassigned to a surviving feature.

This filter is the general-purpose tool for discarding features based on any criterion you can express as a boolean flag (biased features, minority phases, user-selected grains, etc.). For the specific case of removing features smaller than a size threshold, see [Remove Minimum Size Features](RequireMinimumSizeFeaturesFilter.md) instead.

### Selected Operation

The *Selected Operation* parameter provides the following choices:

- **Remove [0]**: Remove the flagged **Features** from the geometry. Neighboring features grow outward isotropically to fill the gaps.
- **Extract [1]**: Copy the flagged **Features** into a new separate geometry without modifying the original.
- **Extract then Remove [2]**: Copy the flagged **Features** into a new geometry and then remove them from the original (combining the previous two modes).

### WARNING: NeighborList Removal

When the operation is *Remove* or *Extract then Remove*, any *NeighborList* arrays in the Cell Feature **Attribute Matrix** will be **removed** because the neighbor relationships have changed. Re-run [Compute Feature Neighbors](ComputeFeatureNeighborsFilter.md) afterward to rebuild them.

### Caveats

This filter will **only** run on an **Image Geometry**.

### Required Input Sources

- **Cell Feature Ids** -- produced by a segmentation filter such as [Segment Features (Misorientation)](../OrientationAnalysis/EBSDSegmentFeaturesFilter.md) or [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md).
- **Flag Array** -- a boolean feature-level array. Typical producers: [Compute Biased Features](ComputeBiasedFeaturesFilter.md), [Compute Surface Features](ComputeSurfaceFeaturesFilter.md), or a custom flag built via threshold/boolean operations on any feature-level statistic.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
