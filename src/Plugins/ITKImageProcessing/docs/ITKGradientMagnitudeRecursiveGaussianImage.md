# ITK Gradient Magnitude Recursive Gaussian Image Filter (ITKGradientMagnitudeRecursiveGaussianImage)

Computes the Magnitude of the Gradient of an image by convolution with the first derivative of a Gaussian.

## Group (Subgroup)

ITKImageGradient (ImageGradient)

## Description

This filter is implemented using the recursive gaussian filters

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Sigma | float64 | Set/Get Sigma value. Sigma is measured in the units of image spacing. |
| NormalizeAcrossScale | bool | Set/Get the normalization factor that will be used for the Gaussian. \sa RecursiveGaussianImageFilter::SetNormalizeAcrossScale |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching BasicPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching BasicPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


