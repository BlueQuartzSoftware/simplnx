# ITK Median Image Filter

Replaces each pixel with the median of its neighborhood — removes speckle/salt-and-pepper noise while keeping edges sharp.

## Group (Subgroup)

ITKSmoothing (Smoothing)

## Description

This filter replaces each pixel with the **median** value of the pixels in a neighborhood around it. The median is a nonlinear statistic that is not pulled toward extreme values, so a median filter removes isolated noise spikes (salt-and-pepper / shot noise) much more cleanly than a mean/Gaussian blur, and it preserves sharp edges better because it does not average across them.

### Parameter Guidance

- **Radius** — the neighborhood half-width per axis, **in pixels**. The neighborhood spans `2 × Radius + 1` pixels along each axis (so a radius of *1* gives a 3×3×3 window). Larger radii remove more noise but blur fine detail; typical values are 1-2.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
