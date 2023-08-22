# ITK Gradient Magnitude Image Filter (ITKGradientMagnitudeImage)

Computes the gradient magnitude of an image region at each pixel.

## Group (Subgroup)

ITKImageGradient (ImageGradient)

## Description

filter_data.detail_desc=\see Image 


\see Neighborhood 


\see NeighborhoodOperator 


\see NeighborhoodIterator

## Parameters

| Name | Type | Description |
|------|------|-------------|
| UseImageSpacing | bool | Set/Get whether or not the filter will use the spacing of the input image in the computation of the derivatives. Use On to compute the gradient in physical space; use Off to ignore image spacing and to compute the gradient in isotropic voxel space. Default is On. |

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


