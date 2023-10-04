# ITK Binary Threshold Image Filter

Binarize an input image by thresholding.

## Group (Subgroup)

ITKThresholding (Thresholding)

## Description

This filter produces an output image whose pixels are either one of two values ( OutsideValue or InsideValue ), depending on whether the corresponding input image pixels lie between the two thresholds ( LowerThreshold and UpperThreshold ). Values equal to either threshold is considered to be between the thresholds.

More precisely \f[ Output(x_i) = \begin{cases} InsideValue & \text{if \f$LowerThreshold \leq x_i \leq UpperThreshold\f$} \\ OutsideValue & \text{otherwise} \end{cases} \f]

This filter is templated over the input image type and the output image type.

The filter expect both images to have the same number of dimensions.

The default values for LowerThreshold and UpperThreshold are: LowerThreshold = NumericTraits<TInput>::NonpositiveMin() ; UpperThreshold = NumericTraits<TInput>::max() ; Therefore, generally only one of these needs to be set, depending on whether the user wants to threshold above or below the desired threshold.

Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is NumericTraits<InputPixelType>::max . An exception is thrown if the lower threshold is greater than the upper threshold.

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| LowerThreshold | float64 | The lower threshold that a pixel value could be and still be considered 'Inside Value' |
| UpperThreshold | float64 | The upper threshold that a pixel value could be and still be considered 'Inside Value'|
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

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
