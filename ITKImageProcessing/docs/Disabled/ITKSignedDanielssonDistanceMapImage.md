# ITK::Signed Danielsson Distance Map Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##



This class is parametrized over the type of the input image and the type of the output image.

This filter computes the distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.

For purposes of evaluating the signed distance map, the input is assumed to be binary composed of pixels with value 0 and non-zero.

The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.

As a convention, the distance is evaluated from the boundary of the ON pixels.

The filter returns

\li A signed distance map with the approximation to the euclidean distance.

\li A voronoi partition. (See itkDanielssonDistanceMapImageFilter)

\li A vector map containing the component of the vector relating the current pixel with the closest point of the closest object to this pixel. Given that the components of the distance are computed in "pixels", the vector is represented by an itk::Offset . That is, physical coordinates are not used. (See itkDanielssonDistanceMapImageFilter)



This filter internally uses the DanielssonDistanceMap filter. This filter is N-dimensional.

\see itkDanielssonDistanceMapImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| InsideIsPositive | bool| Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels. |
| SquaredDistance | bool| Set if the distance should be squared. |
| UseImageSpacing | bool| Set if image spacing should be used in computing distances. |


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
