# ITK Signed Danielsson Distance Map Image Filter (ITKSignedDanielssonDistanceMapImage)

This filter computes the signed distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

This class is parameterized over the type of the input image and the type of the output image.

For purposes of evaluating the signed distance map, the input is assumed to be binary composed of pixels with value 0 and non-zero.

The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.

As a convention, the distance is evaluated from the boundary of the ON pixels.

The filter returns

\li A signed distance map with the approximation to the euclidean distance.

\li A voronoi partition. (See itkDanielssonDistanceMapImageFilter)

\li A vector map containing the component of the vector relating the current pixel with the closest point of the closest object to this pixel. Given that the components of the distance are computed in "pixels", the vector is represented by an itk::Offset . That is, physical coordinates are not used. (See itkDanielssonDistanceMapImageFilter)




This filter internally uses the DanielssonDistanceMap filter. This filter is N-dimensional.

\see itkDanielssonDistanceMapImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| InsideIsPositive | bool | Set if the inside represents positive values in the signed distance map. By convention ON pixels are treated as inside pixels. |
| SquaredDistance | bool | Set if the distance should be squared. |
| UseImageSpacing | bool | Set if image spacing should be used in computing distances. |

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


