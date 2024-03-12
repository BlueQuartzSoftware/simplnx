# ITK Normalize To Constant Image Filter (ITKNormalizeToConstantImage)

Scales image pixel intensities to make the sum of all pixels equal a user-defined constant.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

The default value of the constant is 1. It can be changed with SetConstant() .

This transform is especially useful for normalizing a convolution kernel.

This code was contributed in the Insight Journal paper: "FFT based
convolution" by Lehmann G. https://insight-journal.org/browse/publication/717 

\author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.


\see NormalizeImageFilter 


\see StatisticsImageFilter 


\see DivideImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Constant | float64 | Set/get the normalization constant. |

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


