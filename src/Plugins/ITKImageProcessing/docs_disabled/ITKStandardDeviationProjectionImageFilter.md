# ITK Standard Deviation Projection Image Filter

Collapses a 3D volume to a 2D image by taking the standard deviation of voxel values along one axis.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

A **projection** collapses a 3D volume into a 2D image by combining all voxels along one axis into a single output value. This filter uses the **standard deviation**: for each line of voxels parallel to the selected axis, the output voxel measures how much the values along that line vary about their mean. The result has one fewer dimension than the input.

Where the volume is uniform along the collapsed axis, the standard deviation is near zero (dark). Where intensity varies strongly along that axis, the result is large (bright). Use this filter to **highlight variability** — for example, to find regions that change through a stack of slices rather than to summarize their average value.

```{note}
This filter changes the dimensionality of the **Image Geometry** that the data is tied to. As a side effect, **every** Data Array stored in the same Attribute Matrix as the input and output array is also affected.
```

### Parameter Guidance

- *Projection Dimension* — the axis to collapse, given as an index: *0*, *1*, or *2*. Index *0* is the slowest-moving dimension. The output image keeps the two remaining axes.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
