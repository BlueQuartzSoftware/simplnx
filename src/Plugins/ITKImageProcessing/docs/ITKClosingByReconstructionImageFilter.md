# ITK Closing By Reconstruction Image Filter

Closing by reconstruction of an image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

This filter is similar to the morphological closing, but contrary to the morphological closing, the closing by reconstruction preserves the shape of the components. The closing by reconstruction of an image "f" is defined as:

ClosingByReconstruction(f) = ErosionByReconstruction(f, Dilation(f)).

Closing by reconstruction not only preserves structures preserved by the dilation, but also levels raises the contrast of the darkest regions. If PreserveIntensities is on, a subsequent reconstruction by dilation using a marker image that is the original image for all unaffected pixels.

Closing by reconstruction is described in Chapter 6.3.9 of Pierre Soille's book "Morphological Image Analysis: Principles and
Applications", Second Edition, Springer, 2003.

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- GrayscaleMorphologicalClosingImageFilter

### Kernel Type

The *Kernel Type* parameter selects the structuring element used for the morphological operation:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
