# ITK Gradient Magnitude Recursive Gaussian Image Filter

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

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
