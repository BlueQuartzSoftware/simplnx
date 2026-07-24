# ITK Valued Regional Minima Image Filter

Marks regional minima while preserving their original pixel intensities; all other pixels are set to the pixel-type maximum.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

A regional minimum is a connected flat zone of pixels that all share the same value and whose every neighboring pixel has a strictly higher value. Unlike the binary [ITK Regional Minima Image Filter](ITKRegionalMinimaImageFilter.md), this filter keeps each regional-minimum pixel at its original intensity and sets every other pixel to the maximum value for the pixel type, so the relative darkness of the detected valleys is retained. A completely flat image is marked entirely as a regional minimum.

The pixel neighborhood is controlled by the **Fully Connected** attribute. When it is off ("face connected"), only face-adjacent pixels are considered neighbors, corresponding to a connectivity of 4 in 2D and 6 in 3D. When it is on, diagonally adjacent pixels are also included, corresponding to a connectivity of 8 in 2D and 26 in 3D; this tends to merge nearby minima into a single connected region.

### Parameter Guidance

- **Fully Connected Components**: See the connectivity discussion above. Leave off for standard face-connected behavior.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or a prior ITK image filter.

### See Also

- [ITK Valued Regional Maxima Image Filter](ITKValuedRegionalMaximaImageFilter.md)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
