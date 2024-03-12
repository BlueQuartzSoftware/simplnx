# ITK Approximate Signed Distance Map Image Filter (ITKApproximateSignedDistanceMapImage)

Create a map of the approximate signed distance from the boundaries of a binary image.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

The ApproximateSignedDistanceMapImageFilter takes as input a binary image and produces a signed distance map. Each pixel value in the output contains the approximate distance from that pixel to the nearest "object" in the binary image. This filter differs from the DanielssonDistanceMapImageFilter in that it calculates the distance to the "object edge" for pixels within the object.

Negative values in the output indicate that the pixel at that position is within an object in the input image. The absolute value of a negative pixel represents the approximate distance to the nearest object boundary pixel.

WARNING: This filter requires that the output type be floating-point. Otherwise internal calculations will not be performed to the appropriate precision, resulting in completely incorrect (read: zero-valued) output.

The distances computed by this filter are Chamfer distances, which are only an approximation to Euclidean distances, and are not as exact approximations as those calculated by the DanielssonDistanceMapImageFilter . On the other hand, this filter is faster.

This filter requires that an "inside value" and "outside value" be set as parameters. The "inside value" is the intensity value of the binary image which corresponds to objects, and the "outside value" is the intensity of the background. (A typical binary image often represents objects as black (0) and background as white (usually 255), or vice-versa.) Note that this filter is slightly faster if the inside value is less than the outside value. Otherwise an extra iteration through the image is required.

This filter uses the FastChamferDistanceImageFilter and the IsoContourDistanceImageFilter internally to perform the distance calculations.

\see DanielssonDistanceMapImageFilter 


\see SignedDanielssonDistanceMapImageFilter 


\see SignedMaurerDistanceMapImageFilter 


\see FastChamferDistanceImageFilter 


\see IsoContourDistanceImageFilter 


\author Zach Pincus

## Parameters

| Name | Type | Description |
|------|------|-------------|
| InsideValue | float64 | Set/Get intensity value representing the interior of objects in the mask. |
| OutsideValue | float64 | Set/Get intensity value representing non-objects in the mask. |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching IntegerPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching IntegerPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


