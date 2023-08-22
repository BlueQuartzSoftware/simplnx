# ITK Binary Threshold Image Filter (ITKBinaryThresholdImage)

Binarize an input image by thresholding.

## Group (Subgroup)

ITKThresholding (Thresholding)

## Description

This filter produces an output image whose pixels are either one of two values ( OutsideValue or InsideValue ), depending on whether the corresponding input image pixels lie between the two thresholds ( LowerThreshold and UpperThreshold ). Values equal to either threshold is considered to be between the thresholds.

More precisely \f[ Output(x_i) = \begin{cases} InsideValue & \text{if \f$LowerThreshold \leq x_i \leq UpperThreshold\f$} \\ OutsideValue & \text{otherwise} \end{cases} \f] 

This filter is templated over the input image type and the output image type.

The filter expect both images to have the same number of dimensions.

The default values for LowerThreshold and UpperThreshold are: LowerThreshold = NumericTraits<TInput>::NonpositiveMin() ; UpperThreshold = NumericTraits<TInput>::max() ; Therefore, generally only one of these needs to be set, depending on whether the user wants to threshold above or below the desired threshold.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| LowerThreshold | float64 |  |
| UpperThreshold | float64 | Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is NumericTraits<InputPixelType>::max . An exception is thrown if the lower threshold is greater than the upper threshold. |
| InsideValue | uint8 | Set the "inside" pixel value. The default value NumericTraits<OutputPixelType>::max() |
| OutsideValue | uint8 | Set the "outside" pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() . |

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
+ Class Name: ITKBinaryThresholdImage
+ Displayed Name: ITK Binary Threshold Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| inside_value | InsideValue | Set the 'inside' pixel value. The default value NumericTraits<OutputPixelType>::max() | complex.UInt8Parameter |
| lower_threshold | LowerThreshold |  | complex.Float64Parameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| outside_value | OutsideValue | Set the 'outside' pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() . | complex.UInt8Parameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |
| upper_threshold | UpperThreshold | Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is NumericTraits<InputPixelType>::max . An exception is thrown if the lower threshold is greater than the upper threshold. | complex.Float64Parameter |

