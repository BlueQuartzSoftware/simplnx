# ITK Gradient Magnitude Image Filter

Computes the gradient magnitude (edge strength) of an image at each pixel.

## Group (Subgroup)

ITKImageGradient (ImageGradient)

## Description

The gradient magnitude is the length of the local intensity gradient vector — large where the image changes rapidly (edges) and near zero in smooth regions. This filter estimates it directly from simple finite differences between neighboring pixels, which makes it fast but sensitive to noise; for noisy data prefer the smoothed variant [ITK Gradient Magnitude Recursive Gaussian Image Filter](ITKGradientMagnitudeRecursiveGaussianImageFilter.md), which blurs with a Gaussian before differentiating.

### Parameter Guidance

- **Use Image Spacing**: When on (default), derivatives are computed in physical space using the image spacing, giving results in intensity units per physical length unit. When off, spacing is ignored and the gradient is computed in isotropic voxel space.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
