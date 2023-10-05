# ITK Median Image Filter

Applies a median filter to an image.

## Group (Subgroup)

ITKSmoothing (Smoothing)

## Description

Computes an image where a given pixel is the median value of the the pixels in a neighborhood about the corresponding input pixel.

A median filter is one of the family of nonlinear filters. It is used to smooth an image without being biased by outliers or shot noise.

This filter requires that the input pixel type provides an operator<() (LessThan Comparable).* Image

- Neighborhood
- NeighborhoodOperator
- NeighborhoodIterator

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
