# ITK::Danielsson Distance Map Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

This filter computes the distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.

TInputImage

Input Image Type



TOutputImage

Output Image Type



TVoronoiImage

Voronoi Image Type. Note the default value is TInputImage.



The input is assumed to contain numeric codes defining objects. The filter will produce as output the following images:


\li A Voronoi partition using the same numeric codes as the input.

\li A distance map with the approximation to the euclidean distance. from a particular pixel to the nearest object to this pixel in the input image.

\li A vector map containing the component of the vector relating the current pixel with the closest point of the closest object to this pixel. Given that the components of the distance are computed in "pixels", the vector is represented by an itk::Offset . That is, physical coordinates are not used.


This filter is N-dimensional and known to be efficient in computational time. The algorithm is the N-dimensional version of the 4SED algorithm given for two dimensions in:

Danielsson, Per-Erik. Euclidean Distance Mapping. Computer Graphics and Image Processing 14, 227-248 (1980).

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| InputIsBinary | bool| Set if the input is binary. If this variable is set, each nonzero pixel in the input image will be given a unique numeric code to be used by the Voronoi partition. If the image is binary but you are not interested in the Voronoi regions of the different nonzero pixels, then you need not set this. |
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
