# ITK Sigmoid Image Filter

Remaps pixel intensities through an S-shaped (sigmoid) curve, useful for smoothly boosting contrast within a chosen intensity band.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This filter passes every pixel value through a **sigmoid** function — a smooth S-shaped curve. Input values near a chosen center are stretched (increasing contrast there), while values far above or below the center are gently compressed toward the output maximum or minimum. It is a common way to enhance contrast in a region of interest without the hard clipping of a threshold.

Each output pixel is computed as:

$$
\text{output} = (\text{OutputMax} - \text{OutputMin}) \cdot \frac{1}{1 + e^{-(x - \beta)/\alpha}} + \text{OutputMin}
$$

where $x$ is the input pixel value.

### Parameter Guidance

- **Alpha** — the width of the transition region, in input intensity units. Larger values give a gentler, more gradual slope; smaller values give a sharper, more threshold-like transition. A negative value inverts the curve.
- **Beta** — the center (inflection point) of the curve, in input intensity units. Input values near *Beta* land in the steep part of the S and are spread out the most.
- **Output Minimum** / **Output Maximum** — the range the output is mapped into (defaults *0* and *255*).

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
