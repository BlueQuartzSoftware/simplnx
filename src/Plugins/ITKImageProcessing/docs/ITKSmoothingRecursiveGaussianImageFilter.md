# ITK Smoothing Recursive Gaussian Image Filter (ITKSmoothingRecursiveGaussianImage)

Computes the smoothing of an image by convolution with the Gaussian kernels implemented as IIR filters.

## Group (Subgroup)

ITKSmoothing (Smoothing)

## Description

This filter is implemented using the recursive gaussian filters. For multi-component images, the filter works on each component independently.

For this filter to be able to run in-place the input and output image types need to be the same and/or the same type as the RealImageType.

## See Also

- [RecursiveGaussianImageFilter::SetNormalizeAcrossScale](https://itk.org/Doxygen/html/classitk_1_1RecursiveGaussianImageFilter::SetNormalizeAcrossScale.html)|

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
