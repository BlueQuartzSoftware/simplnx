# ITK Otsu Multiple Thresholds Image Filter (ITKOtsuMultipleThresholdsImage)

Threshold an image using multiple Otsu Thresholds.

## Group (Subgroup)

ITKThresholding (Thresholding)

## Description

This filter creates a labeled image that separates the input image into various classes. The filter computes the thresholds using the OtsuMultipleThresholdsCalculator and applies those thresholds to the input image using the ThresholdLabelerImageFilter . The NumberOfHistogramBins and NumberOfThresholds can be set for the Calculator. The LabelOffset can be set for the ThresholdLabelerImageFilter .

This filter also includes an option to use the valley emphasis algorithm from H.F. Ng, "Automatic thresholding for defect detection", Pattern Recognition Letters, (27): 1644-1649, 2006. The valley emphasis algorithm is particularly effective when the object to be thresholded is small. See the following tests for examples: itkOtsuMultipleThresholdsImageFilterTest3 and itkOtsuMultipleThresholdsImageFilterTest4 To use this algorithm, simple call the setter: SetValleyEmphasis(true) It is turned off by default.* ScalarImageToHistogramGenerator 
- OtsuMultipleThresholdsCalculator 
- ThresholdLabelerImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| NumberOfThresholds | uint8 | Set/Get the number of thresholds. Default is 1. |
| LabelOffset | uint8 | Set/Get the offset which labels have to start from. Default is 0. |
| NumberOfHistogramBins | uint32 | Set/Get the number of histogram bins. Default is 128. |
| ValleyEmphasis | bool | Set/Get the use of valley emphasis. Default is false. |
| ReturnBinMidpoint | bool | Should the threshold value be mid-point of the bin or the maximum? Default is to return bin maximum. |

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


## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKOtsuMultipleThresholdsImage
+ Displayed Name: ITK Otsu Multiple Thresholds Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| label_offset | LabelOffset | Set/Get the offset which labels have to start from. Default is 0. | complex.UInt8Parameter |
| number_of_histogram_bins | NumberOfHistogramBins | Set/Get the number of histogram bins. Default is 128. | complex.UInt32Parameter |
| number_of_thresholds | NumberOfThresholds | Set/Get the number of thresholds. Default is 1. | complex.UInt8Parameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| return_bin_midpoint | ReturnBinMidpoint | Should the threshold value be mid-point of the bin or the maximum? Default is to return bin maximum. | complex.BoolParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |
| valley_emphasis | ValleyEmphasis | Set/Get the use of valley emphasis. Default is false. | complex.BoolParameter |

