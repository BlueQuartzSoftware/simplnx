# ITK Sigmoid Image Filter (ITKSigmoidImage)

Computes the sigmoid function pixel-wise.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

filter_data.detail_desc=A linear transformation is applied first on the argument of the sigmoid function. The resulting total transform is given by

 \f[ f(x) = (Max-Min) \cdot \frac{1}{\left(1+e^{- \frac{ x - \beta }{\alpha}}\right)} + Min \f] 

Every output pixel is equal to f(x). Where x is the intensity of the homologous input pixel, and alpha and beta are user-provided constants.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Alpha | float64 |  |
| Beta | float64 |  |
| OutputMaximum | float64 |  |
| OutputMinimum | float64 |  |

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


