# ITK Smoothing Recursive Gaussian Image Filter (ITKSmoothingRecursiveGaussianImage)

Computes the smoothing of an image by convolution with the Gaussian kernels implemented as IIR filters.

## Group (Subgroup)

ITKSmoothing (Smoothing)

## Description

This filter is implemented using the recursive gaussian filters. For multi-component images, the filter works on each component independently.

For this filter to be able to run in-place the input and output image types need to be the same and/or the same type as the RealImageType.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Sigma | float64 | Set the standard deviation of the Gaussian used for smoothing. Sigma is measured in the units of image spacing. You may use the method SetSigma to set the same value across each axis or use the method SetSigmaArray if you need different values along each axis. |
| NormalizeAcrossScale | bool | Set/Get the flag for normalizing the Gaussian over scale-space. This method does not effect the output of this filter.

\see RecursiveGaussianImageFilter::SetNormalizeAcrossScale |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching typelist2::append<BasicPixelIDTypeList, VectorPixelIDTypeList>::type |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching typelist2::append<BasicPixelIDTypeList, VectorPixelIDTypeList>::type |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


