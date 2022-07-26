# ITK::Approximate Signed Distance Map Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Create a map of the approximate signed distance from the boundaries of a binary image.

The ApproximateSignedDistanceMapImageFilter takes as input a binary image and produces a signed distance map. Each pixel value in the output contains the approximate distance from that pixel to the nearest "object" in the binary image. This filter differs from the DanielssonDistanceMapImageFilter in that it calculates the distance to the "object edge" for pixels within the object.

Negative values in the output indicate that the pixel at that position is within an object in the input image. The absolute value of a negative pixel represents the approximate distance to the nearest object boundary pixel.

WARNING: This filter requires that the output type be floating-point. Otherwise internal calculations will not be performed to the appropriate precision, resulting in completely incorrect (read: zero-valued) output.

The distances computed by this filter are Chamfer distances, which are only an approximation to Euclidian distances, and are not as exact approximations as those calculated by the DanielssonDistanceMapImageFilter . On the other hand, this filter is faster.

This filter requires that an "inside value" and "outside value" be set as parameters. The "inside value" is the intensity value of the binary image which corresponds to objects, and the "outside value" is the intensity of the background. (A typical binary image often represents objects as black (0) and background as white (usually 255), or vice-versa.) Note that this filter is slightly faster if the inside value is less than the outside value. Otherwise an extra iteration through the image is required.

This filter uses the FastChamferDistanceImageFilter and the IsoContourDistanceImageFilter internally to perform the distance calculations.

\see DanielssonDistanceMapImageFilter

\see SignedDanielssonDistanceMapImageFilter

\see SignedMaurerDistanceMapImageFilter

\see FastChamferDistanceImageFilter

\see IsoContourDistanceImageFilter

\author Zach Pincus

\par Wiki Examples:

\li All Examples

\li Compute a distance map from objects in a binary image

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| InsideValue | double| Set/Get intensity value representing the interior of objects in the mask. |
| OutsideValue | double| Set/Get intensity value representing non-objects in the mask. |


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
