# ITK Regional Maxima Image Filter

Produces a binary image marking the regional maxima of the input image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

A regional maximum is a connected flat zone of pixels that all share the same value and whose every neighboring pixel has a strictly lower value (a local "plateau" that is brighter than everything immediately surrounding it). This filter labels those pixels in the output and labels everything else as background, producing a binary image. Use it to detect peaks, bright spots, or seed points for later segmentation.

If the input image is constant (completely flat), the entire image can be treated either as a single regional maximum or as background; the **Flat Is Maxima** option selects which behavior to use.

### Parameter Guidance

- **Flat Is Maxima**: When on (default), a completely flat input image is reported as a regional maximum; when off, a flat image produces an all-background result.
- **Fully Connected**: Controls pixel connectivity. When off (default, "face connected"), only face-adjacent neighbors are considered (connectivity 4 in 2D, 6 in 3D); when on, diagonal neighbors are also included (connectivity 8 in 2D, 26 in 3D). Turning it on tends to merge nearby maxima into a single connected region.
- **Background Value** / **Foreground Value**: These are the output binary labels written for non-maxima and maxima pixels respectively. They are not intensity thresholds applied to the input.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
