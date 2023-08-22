# ITK Valued Regional Minima Image Filter (ITKValuedRegionalMinimaImage)

Transforms the image so that any pixel that is not a regional minima is set to the maximum value for the pixel type. Pixels that are regional minima retain their value.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

filter_data.detail_desc=Regional minima are flat zones surrounded by pixels of higher value. A completely flat image will be marked as a regional minima by this filter.

This code was contributed in the Insight Journal paper: "Finding regional extrema - methods and performance" by Beare R., Lehmann G. https://www.insight-journal.org/browse/publication/65 

\author Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.


\see ValuedRegionalMaximaImageFilter , ValuedRegionalExtremaImageFilter , 


\see HMinimaImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| FullyConnected | bool |  |

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


