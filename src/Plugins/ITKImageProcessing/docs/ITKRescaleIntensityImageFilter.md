# ITK Rescale Intensity Image Filter

Linearly rescales the full intensity range of an image into a chosen output range.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This filter applies a linear transform that maps the **full** intensity range of the input image onto a user-specified output range. The smallest input value maps to *Output Minimum*, the largest input value maps to *Output Maximum*, and everything in between scales proportionally. It is the standard way to normalize an image to a fixed range (for example 0-255 for display, or 0-1 for downstream processing).

$$
\text{output} = (\text{input} - \text{inputMin}) \cdot \frac{\text{OutputMax} - \text{OutputMin}}{\text{inputMax} - \text{inputMin}} + \text{OutputMin}
$$

The input minimum and maximum are detected automatically, so only the output range is set by the user. To stretch a *chosen* input window rather than the full range (clamping outside it), use [ITK Intensity Windowing Image Filter](ITKIntensityWindowingImageFilter.md).

### Parameter Guidance

- **Output Minimum** / **Output Maximum** — the target output range, in **output intensity units** (defaults *0* and *255*).

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
