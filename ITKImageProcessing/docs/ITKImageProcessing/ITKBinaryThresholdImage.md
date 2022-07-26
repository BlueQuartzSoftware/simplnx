# ITK::Binary Threshold Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Binarize an input image by thresholding.

This filter produces an output image whose pixels are either one of two values ( OutsideValue or InsideValue ), depending on whether the corresponding input image pixels lie between the two thresholds ( LowerThreshold and UpperThreshold ). Values equal to either threshold is considered to be between the thresholds.

More precisely \f[ Output(x_i) = \begin{cases} InsideValue & \text{if \f$LowerThreshold \leq x_i \leq UpperThreshold\f$} \\ OutsideValue & \text{otherwise} \end{cases} \f]

This filter is templated over the input image type and the output image type.

The filter expect both images to have the same number of dimensions.

The default values for LowerThreshold and UpperThreshold are: LowerThreshold = NumericTraits<TInput>::NonpositiveMin() ; UpperThreshold = NumericTraits<TInput>::max() ; Therefore, generally only one of these needs to be set, depending on whether the user wants to threshold above or below the desired threshold.

\par Wiki Examples:

\li All Examples

\li Threshold an image

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| LowerThreshold | double| N/A |
| UpperThreshold | double| Set the thresholds. The default lower threshold is NumericTraits<InputPixelType>::NonpositiveMin() . The default upper threshold is NumericTraits<InputPixelType>::max . An execption is thrown if the lower threshold is greater than the upper threshold. |
| InsideValue | int| Set the "inside" pixel value. The default value NumericTraits<OutputPixelType>::max() |
| OutsideValue | int| Set the "outside" pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() . |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | uint8_t | (1)  | Array containing filtered image

## References ##

[1] T.S. Yoo, M. J. Ackerman, W. E. Lorensen, W. Schroeder, V. Chalana, S. Aylward, D. Metaxas, R. Whitaker. Engineering and Algorithm Design for an Image Processing API: A Technical Report on ITK - The Insight Toolkit. In Proc. of Medicine Meets Virtual Reality, J. Westwood, ed., IOS Press Amsterdam pp 586-592 (2002). 
[2] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Design and Functionality. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-28-3
[3] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Introduction and Development Guidelines. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-27-6

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
