# ITK Normalize Image Filter

Normalize an image by setting its mean to zero and variance to one.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

NormalizeImageFilter shifts and scales an image so that the pixels in the image have a zero mean and unit variance. This filter uses StatisticsImageFilter to compute the mean and variance of the input and then applies ShiftScaleImageFilter to shift and scale the pixels.

NB: since this filter normalizes the data such that the mean is at 0, and \f$-\sigma\f$ to \f$+\sigma\f$ is mapped to -1.0 to 1.0, output image integral types will produce an image that DOES NOT HAVE a unit variance due to 68% of the intensity values being mapped to the real number range of -1.0 to 1.0 and then cast to the output integral value.* NormalizeToConstantImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
