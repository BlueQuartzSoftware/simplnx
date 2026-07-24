# Compute Feature Corners

## Group (Subgroup)

Reconstruction (Reconstruction)

## Description

This **Filter** computes the XYZ minimum and maximum coordinates for each **Feature** in a segmentation. A **Feature** is a contiguous group of **Cells** (voxels) that share the same Feature Id; a **Cell** is a single voxel. This data can be important for finding the smallest encompassing volume. These values are given in **Pixel** coordinates (zero-based voxel indices), not physical length units.

If you instead need the bounding box corners in physical coordinates, see [Compute Feature Bounding Boxes](ComputeFeatureBoundsFilter.md).

|       | 0 | 1 | 2 | 3 | 4 |
|-------|---|---|---|---|---|
| 0 | 0 | 0 | 1 | 0 | 0 |
| 1 | 0 | 0 | 1 | 1 | 0 |
| 2 | 0 | 1 | 1 | 1 | 1 |
| 3 | 0 | 0 | 1 | 1 | 0 |
| 4 | 0 | 0 | 0 | 0 | 0 |

If the example matrix above which represents a single feature where the feature ID = 1, the output of the filter would be:

    X Min = 1
    Y Min = 0
    Z Min = 0

    X Max = 4
    Y Max = 3
    Z Max = 0

### Required Input Sources

This filter requires a **Cell Feature Ids** array, typically produced by a segmentation filter such as [Segment Features (Scalar)](ScalarSegmentFeaturesFilter.md) or one of the misorientation-based segmentation filters in the OrientationAnalysis plugin.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
