# ITK Threshold Image Filter (ITKThresholdImage)

Set image values to a user-specified value if they are below, above, or between simple threshold values.

## Group (Subgroup)

ITKThresholding (Thresholding)

## Description

ThresholdImageFilter sets image values to a user-specified "outside" value (by default, "black") if the image values are below, above, or between simple threshold values.

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




## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKThresholdImage
+ Displayed Name: ITK Threshold Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| lower | Lower | Set/Get methods to set the lower threshold. | complex.Float64Parameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| outside_value | OutsideValue | The pixel type must support comparison operators. Set the 'outside' pixel value. The default value NumericTraits<PixelType>::ZeroValue() . | complex.Float64Parameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |
| upper | Upper | Set/Get methods to set the upper threshold. | complex.Float64Parameter |

