# ITK Signed Maurer Distance Map Image Filter

This filter calculates the Euclidean distance transform of a binary image in linear time for arbitrary dimensions.

## Group (Subgroup)

ITKDistanceMap (DistanceMap)

## Description

 Inputs and Outputs
This is an image-to-image filter. The dimensionality is arbitrary. The only dimensionality constraint is that the input and output images be of the same dimensions and size. To maintain integer arithmetic within the filter, the default output is the signed squared distance. This implies that the input image should be of type "unsigned int" or "int" whereas the output image is of type "int". Obviously, if the user wishes to utilize the image spacing or to have a filter with the Euclidean distance (as opposed to the squared distance), output image types of float or double should be used.

The inside is considered as having negative distances. Outside is treated as having positive distances. To change the convention, use the InsideIsPositive(bool) function.

 Parameters
Set/GetBackgroundValue specifies the background of the value of the input binary image. Normally this is zero and, as such, zero is the default value. Other than that, the usage is completely analogous to the itk::DanielssonDistanceImageFilter class except it does not return the Voronoi map.

Reference: C. R. Maurer, Jr., R. Qi, and V. Raghavan, "A Linear Time Algorithm
 for Computing Exact Euclidean Distance Transforms of Binary Images in
 Arbitrary Dimensions", IEEE - Transactions on Pattern Analysis and Machine Intelligence, 25(2): 265-270, 2003.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
