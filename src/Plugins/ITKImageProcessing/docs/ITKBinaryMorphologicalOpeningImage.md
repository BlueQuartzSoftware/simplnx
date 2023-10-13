# ITK Binary Morphological Opening Image Filter

binary morphological opening of an image.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

This filter removes small (i.e., smaller than the structuring element) structures in the interior or at the boundaries of the image. The morphological opening of an image "f" is defined as: Opening(f) = Dilatation(Erosion(f)).

The structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring element having values > 0 are candidates for affecting the center pixel.

This code was contributed in the Insight Journal paper: "Binary morphological closing and opening image filters" by Lehmann G. <https://www.insight-journal.org/browse/publication/58>

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleErodeImageFilter

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
