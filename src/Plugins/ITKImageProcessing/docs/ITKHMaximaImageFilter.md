# ITK H Maxima Image Filter

Suppress local maxima whose height above the baseline is less than h.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

HMaximaImageFilter suppresses local maxima that are less than h intensity units above the (local) background. This has the effect of smoothing over the "high" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMinimaImageFilter to suppress the local minima whose depth is less than h intensity units below the (local) background.

If the output of HMaximaImageFilter is subtracted from the original image, the significant "peaks" in the image can be identified. This is what the HConvexImageFilter provides.

This filter uses the ReconstructionByDilationImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image minus the height parameter h.

Geodesic morphology and the H-Maxima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.

The height parameter is set using SetHeight.* ReconstructionByDilationImageFilter , HMinimaImageFilter , HConvexImageFilter

- MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
