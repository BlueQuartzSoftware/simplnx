# ITK::Signed Maurer Distance Map Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

This filter calculates the Euclidean distance transform of a binary image in linear time for arbitrary dimensions.

\par Inputs and Outputs
This is an image-to-image filter. The dimensionality is arbitrary. The only dimensionality constraint is that the input and output images be of the same dimensions and size. To maintain integer arithmetic within the filter, the default output is the signed squared distance. This implies that the input image should be of type "unsigned int" or "int" whereas the output image is of type "int". Obviously, if the user wishes to utilize the image spacing or to have a filter with the Euclidean distance (as opposed to the squared distance), output image types of float or double should be used.

The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.

\par Parameters
Set/GetBackgroundValue specifies the background of the value of the input binary image. Normally this is zero and, as such, zero is the default value. Other than that, the usage is completely analogous to the itk::DanielssonDistanceImageFilter class except it does not return the Voronoi map.

Reference: C. R. Maurer, Jr., R. Qi, and V. Raghavan, "A Linear Time Algorithm for Computing Exact Euclidean Distance Transforms of Binary Images in Arbitrary Dimensions", IEEE - Transactions on Pattern Analysis and Machine Intelligence, 25(2): 265-270, 2003.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| InsideIsPositive | bool| Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels. |
| SquaredDistance | bool| Set if the distance should be squared. |
| UseImageSpacing | bool| Set if image spacing should be used in computing distances. |
| BackgroundValue | double| Set the background value which defines the object. Usually this value is = 0. |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | float | (1)  | Array containing filtered image

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
