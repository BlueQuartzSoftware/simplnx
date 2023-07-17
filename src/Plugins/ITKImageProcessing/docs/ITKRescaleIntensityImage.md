# ITK::Rescale Intensity Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Applies a linear transformation to the intensity levels of the input Image .

RescaleIntensityImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should have.

The following equation gives the mapping of the intensity values

\par
 \f[ outputPixel = ( inputPixel - inputMin) \cdot \frac{(outputMax - outputMin )}{(inputMax - inputMin)} + outputMin \f]

All computations are performed in the precision of the input pixel's RealType. Before assigning the computed value to the output pixel.

NOTE: In this filter the minimum and maximum values of the input image are computed internally using the MinimumMaximumImageCalculator . Users are not supposed to set those values in this filter. If you need a filter where you can set the minimum and maximum values of the input, please use the IntensityWindowingImageFilter . If you want a filter that can use a user-defined linear transformation for the intensity, then please use the ShiftScaleImageFilter .

\see IntensityWindowingImageFilter

\par Wiki Examples:

\li All Examples

\li Rescale the intensity values of an image to a specified range

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| OutputMinimum | double| N/A |
| OutputMaximum | double| N/A |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None |  | (1)  | Array containing filtered image

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
