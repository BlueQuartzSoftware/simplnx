# ITK Threshold Image Filter (ITKThresholdImage)

Set image values to a user-specified value if they are below, above, or between simple threshold values.

## Group (Subgroup)

ITKThresholding (Thresholding)

## Description

filter_data.detail_desc=ThresholdImageFilter sets image values to a user-specified "outside" value (by default, "black") if the image values are below, above, or between simple threshold values.

The available methods are:

ThresholdAbove() : The values greater than the threshold value are set to OutsideValue

ThresholdBelow() : The values less than the threshold value are set to OutsideValue

ThresholdOutside() : The values outside the threshold range (less than lower or greater than upper) are set to OutsideValue

Note that these definitions indicate that pixels equal to the threshold value are not set to OutsideValue in any of these methods

The pixels must support the operators >= and <=.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Lower | float64 | Set/Get methods to set the lower threshold. |
| Upper | float64 | Set/Get methods to set the upper threshold. |
| OutsideValue | float64 | The pixel type must support comparison operators. Set the "outside" pixel value. The default value NumericTraits<PixelType>::ZeroValue() . |

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


