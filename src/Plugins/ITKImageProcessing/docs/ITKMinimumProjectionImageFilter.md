# ITK Minimum Projection Image Filter

Collapses a 3D volume to a 2D image by taking the minimum voxel value along one axis.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

A **projection** collapses a 3D volume into a 2D image by combining all voxels along one axis into a single output value. This filter uses the **minimum**: for each line of voxels parallel to the selected axis, the output voxel is the smallest value found along that line. The result has one fewer dimension than the input.

Minimum projection is the dark-feature counterpart of maximum projection: it preserves the darkest object along each line of sight, which is useful for emphasizing low-intensity structures such as voids, pores, or absorbing features against a bright background.

```{note}
This filter changes the dimensionality of the **Image Geometry** that the data is tied to. As a side effect, **every** Data Array stored in the same Attribute Matrix as the input and output array is also affected.
```

### Parameter Guidance

- *Projection Dimension* — the axis to collapse, given as an index: *0*, *1*, or *2*. Index *0* is the slowest-moving dimension. The output image keeps the two remaining axes.
- *Perform In-Place* — when enabled, the projection replaces the input geometry rather than creating a new one.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
