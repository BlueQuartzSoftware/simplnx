# ITK H Minima Image Filter

Suppress local minima whose depth below the baseline is less than h.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

HMinimaImageFilter suppresses local minima that are less than h intensity units below the (local) background. This has the effect of smoothing over the "low" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMaximaImageFilter to suppress the local maxima whose height is less than h intensity units above the (local) background.

If original image is subtracted from the output of HMinimaImageFilter , the significant "valleys" in the image can be identified. This is what the HConcaveImageFilter provides.

This filter uses the GrayscaleGeodesicErodeImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image plus the height parameter h.

Geodesic morphology and the H-Minima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.* GrayscaleGeodesicDilateImageFilter , HMinimaImageFilter , HConvexImageFilter

- MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
