# ITK Binary Projection Image Filter

Collapses a 3D volume to a 2D image using a logical-OR projection: a pixel is foreground if any voxel along the axis is foreground.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

A **projection** collapses a 3D volume into a 2D image by combining all voxels along one axis into a single output value. This filter uses a **binary (logical-OR) rule**: for each line of voxels parallel to the selected axis, the output pixel is set to the *Foreground Value* if **any** voxel along that line equals the *Foreground Value*; otherwise it is set to the *Background Value*. The result has one fewer dimension than the input.

Use binary projection to answer "is this feature present anywhere along this axis?" — for example, to flatten a segmented 3D mask into a 2D footprint showing where labeled material exists at any depth.

```{note}
This filter changes the dimensionality of the **Image Geometry** that the data is tied to. As a side effect, **every** Data Array stored in the same Attribute Matrix as the input and output array is also affected.
```

### Parameter Guidance

- *Projection Dimension* — the axis to collapse, given as an index: *0*, *1*, or *2*. Index *0* is the slowest-moving dimension. The output image keeps the two remaining axes.
- *Foreground Value* — the voxel value treated as "foreground." An output pixel is set to this value if any voxel along the collapsed line matches it. Defaults to *1*.
- *Background Value* — the value written to output pixels where no foreground voxel was found along the line. Defaults to *0*.
- *Perform In-Place* — when enabled, the projection replaces the input geometry rather than creating a new one.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
