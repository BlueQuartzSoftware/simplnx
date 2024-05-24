# ITK Double Threshold Image Filter

Binarize an input image using double thresholding.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Double threshold addresses the difficulty in selecting a threshold that will select the objects of interest without selecting extraneous objects. Double threshold considers two threshold ranges: a narrow range and a wide range (where the wide range encompasses the narrow range). If the wide range was used for a traditional threshold (where values inside the range map to the foreground and values outside the range map to the background), many extraneous pixels may survive the threshold operation. If the narrow range was used for a traditional threshold, then too few pixels may survive the threshold.

Double threshold uses the narrow threshold image as a marker image and the wide threshold image as a mask image in the geodesic dilation. Essentially, the marker image (narrow threshold) is dilated but constrained to lie within the mask image (wide threshold). Thus, only the objects of interest (those pixels that survived the narrow threshold) are extracted but the those objects appear in the final image as they would have if the wide threshold was used.
- [GrayscaleGeodesicDilateImageFilter](https://itk.org/Doxygen/html/classitk_1_1GrayscaleGeodesicDilateImageFilter.html)

- [MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter ,](https://itk.org/Doxygen/html/classitk_1_1MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter ,.html)BinaryDilateImageFilter


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
