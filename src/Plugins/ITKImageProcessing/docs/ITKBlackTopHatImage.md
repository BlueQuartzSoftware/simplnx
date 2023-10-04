# ITK Black Top Hat Image Filter

Black top hat extracts local minima that are smaller than the structuring element.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Black top hat extracts local minima that are smaller than the structuring element. It subtracts the background from the input image. The output of the filter transforms the black valleys into white peaks.

Top-hats are described in Chapter 4.5 of Pierre Soille's book "Morphological Image Analysis: Principles and Applications", Second Edition, Springer, 2003.

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| KernelRadius | uint32 | Set the radius of the kernel structuring element. |
| KernelType | KernelEnum | Set the kernel or structuring element used for the morphology. |
| SafeBorder | bool | A safe border is added to input image to avoid borders effects and remove it once the closing is done |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching BasicPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching BasicPixelIDTypeList |

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
