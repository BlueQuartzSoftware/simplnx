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


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
