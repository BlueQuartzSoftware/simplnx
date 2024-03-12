# ITK Regional Maxima Image Filter (ITKRegionalMaximaImage)

Produce a binary image where foreground is the regional maxima of the input image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional maxima are flat zones surrounded by pixels of lower value.

If the input image is constant, the entire image can be considered as a maxima or not. The desired behavior can be selected with the SetFlatIsMaxima() method.

\author Gaetan Lehmann


This class was contributed to the Insight Journal by author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France. The paper can be found at https://www.insight-journal.org/browse/publication/65 

\see ValuedRegionalMaximaImageFilter 


\see HConvexImageFilter 


\see RegionalMinimaImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| BackgroundValue | float64 | Set/Get the value used as "background" in the output image. Defaults to NumericTraits<PixelType>::NonpositiveMin() . |
| ForegroundValue | float64 | Set/Get the value in the output image to consider as "foreground". Defaults to maximum value of PixelType. |
| FullyConnected | bool | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. |
| FlatIsMaxima | bool | Set/Get whether a flat image must be considered as a maxima or not. Defaults to true. |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching ScalarPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching ScalarPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


