# ITK Danielsson Distance Map Image Filter

This filter computes the distance map of the input image as an approximation with pixel accuracy to the Euclidean distance.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

The input is assumed to contain numeric codes defining objects. The filter will produce as output the following images:

- A Voronoi partition using the same numeric codes as the input.
- A distance map with the approximation to the euclidean distance. from a particular pixel to the nearest object to this pixel in the input image.
- A vector map containing the component of the vector relating the current pixel with the closest point of the closest object to this pixel. Given that the components of the distance are computed in "pixels", the vector is represented by an itk::Offset. That is, physical coordinates are not used.

This filter is N-dimensional and known to be efficient in computational time. The algorithm is the N-dimensional version of the 4SED algorithm given for two dimensions in:

Danielsson, Per-Erik. Euclidean Distance Mapping. Computer Graphics and Image Processing 14, 227-248 (1980).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
