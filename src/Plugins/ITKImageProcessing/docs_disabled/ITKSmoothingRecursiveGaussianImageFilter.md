# ITK Smoothing Recursive Gaussian Image Filter

Gaussian blur computed recursively, so the run time stays constant no matter how large the blur — preferred for wide smoothing.

## Group (Subgroup)

ITKSmoothing (Smoothing)

## Description

This filter produces the same kind of **Gaussian blur** as [ITK Discrete Gaussian Image Filter](ITKDiscreteGaussianImageFilter.md), but implements it with a recursive (IIR) approximation. The key advantage is that its cost is **independent of the blur width (sigma)** — a large blur takes no longer than a small one — so it is the better choice when smoothing strongly. For multi-component images each component is smoothed independently.

### Parameter Guidance

- **Sigma** — the Gaussian standard deviation (the blur radius). Larger values blur more. By default it is measured in the geometry's **physical/world units** (or pixels if image spacing is not used). It is specified as an integer value; typical values are around 1-3.
- **Normalize Across Scale** — a scale-normalization option used mainly when comparing the responses of this filter at several different Sigma values (multi-scale analysis); leave off for ordinary smoothing.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
