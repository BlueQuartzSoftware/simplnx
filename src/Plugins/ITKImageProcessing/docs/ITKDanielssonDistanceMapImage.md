# ITK Danielsson Distance Map Image Filter (ITKDanielssonDistanceMapImage)

This filter computes the distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

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

## Parameters

| Name | Type | Description |
|------|------|-------------|
| InputIsBinary | bool | Set/Get if the input is binary. If this variable is set, each nonzero pixel in the input image will be given a unique numeric code to be used by the Voronoi partition. If the image is binary but you are not interested in the Voronoi regions of the different nonzero pixels, then you need not set this. |
| SquaredDistance | bool | Set/Get if the distance should be squared. |
| UseImageSpacing | bool | Set/Get if image spacing should be used in computing distances. |

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

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
