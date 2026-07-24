# ITK Bounded Reciprocal Image Filter

Computes 1/(1+x) for each pixel in the image.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

Computes `1 / (1 + x)` for each pixel value `x`. For non-negative input intensities the result is bounded in the range `(0, 1]`, which makes this filter useful for mapping an unbounded intensity range into a normalized interval. Output values are dimensionless.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
