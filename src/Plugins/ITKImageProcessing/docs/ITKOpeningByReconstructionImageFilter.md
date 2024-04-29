# ITK Opening By Reconstruction Image Filter

Opening by reconstruction of an image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

This filter preserves regions, in the foreground, that can completely contain the structuring element. At the same time, this filter eliminates all other regions of foreground pixels. Contrary to the morphological opening, the opening by reconstruction preserves the shape of the components that are not removed by erosion. The opening by reconstruction of an image "f" is defined as:

OpeningByReconstruction(f) = DilationByReconstruction(f, Erosion(f)).

Opening by reconstruction not only removes structures destroyed by the erosion, but also levels down the contrast of the brightest regions. If PreserveIntensities is on, a subsequent reconstruction by dilation using a marker image that is the original image for all unaffected pixels.

Opening by reconstruction is described in Chapter 6.3.9 of Pierre Soille's book "Morphological Image Analysis: Principles and
Applications", Second Edition, Springer, 2003.

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- GrayscaleMorphologicalOpeningImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
