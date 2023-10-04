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

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| KernelRadius | uint32 | Set the radius of the kernel structuring element. |
| KernelType | KernelEnum | Set the kernel or structuring element used for the morphology. |
| BackgroundValue | float64 | Set the value in eroded part of the image. Defaults to zero |
| ForegroundValue | float64 | Set the value in the image to consider as "foreground". Defaults to maximum value of PixelType. |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching IntegerPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching IntegerPixelIDTypeList |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
