# ITK Rescale Intensity Image Filter (ITKRescaleIntensityImage)

Applies a linear transformation to the intensity levels of the input Image .

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

filter_data.detail_desc=RescaleIntensityImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should have.

The following equation gives the mapping of the intensity values

\par 
 \f[ outputPixel = ( inputPixel - inputMin) \cdot \frac{(outputMax - outputMin )}{(inputMax - inputMin)} + outputMin \f] 


All computations are performed in the precision of the input pixel's RealType. Before assigning the computed value to the output pixel.

NOTE: In this filter the minimum and maximum values of the input image are computed internally using the MinimumMaximumImageCalculator . Users are not supposed to set those values in this filter. If you need a filter where you can set the minimum and maximum values of the input, please use the IntensityWindowingImageFilter . If you want a filter that can use a user-defined linear transformation for the intensity, then please use the ShiftScaleImageFilter .

\see IntensityWindowingImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| OutputMinimum | float64 |  |
| OutputMaximum | float64 |  |

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


