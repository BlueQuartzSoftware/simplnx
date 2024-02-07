# ITK H Convex Image Filter

Identify local maxima whose height above the baseline is greater than h.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

HConvexImageFilter extract local maxima that are more than h intensity units above the (local) background. This has the effect of extracting objects that are brighter than background by at least h intensity units.

This filter uses the HMaximaImageFilter .

Geodesic morphology and the H-Convex algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.* GrayscaleGeodesicDilateImageFilter , HMinimaImageFilter

- MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
