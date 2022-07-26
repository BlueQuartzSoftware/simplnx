# ITK::Intensity Windowing Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Applies a linear transformation to the intensity levels of the input Image that are inside a user-defined interval. Values below this interval are mapped to a constant. Values over the interval are mapped to another constant.

IntensityWindowingImageFilter applies pixel-wise a linear transformation to the intensity values of input image pixels. The linear transformation is defined by the user in terms of the minimum and maximum values that the output image should have and the lower and upper limits of the intensity window of the input image. This operation is very common in visualization, and can also be applied as a convenient preprocessing operation for image segmentation.

All computations are performed in the precision of the input pixel's RealType. Before assigning the computed value to the output pixel.

\par Wiki Examples:

\li All Examples

\li IntensityWindowingImageFilter




\see RescaleIntensityImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| WindowMinimum | double| Set/Get the values of the maximum and minimum intensities of the input intensity window. |
| WindowMaximum | double| Set/Get the values of the maximum and minimum intensities of the input intensity window. |
| OutputMinimum | double| Set/Get the values of the maximum and minimum intensities of the output image. |
| OutputMaximum | double| Set/Get the values of the maximum and minimum intensities of the output image. |


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
