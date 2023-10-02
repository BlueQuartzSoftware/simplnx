# ITK Valued Regional Maxima Image Filter (ITKValuedRegionalMaximaImage)

Transforms the image so that any pixel that is not a regional maxima is set to the minimum value for the pixel type. Pixels that are regional maxima retain their value.

The pixel neighborhood is controlled by the FullyConnected attribute. All adjacent pixels are included in
the neighborhood when FullyConnected=True and the diagonally adjacent pixels are not included when
FullyConnected=False. Different terminology is often used to describe neighborhoods – one common ex-
ample is the “connectivity” notation, which refers to the number of pixels in the neighborhood. FullyCon-
nected=False corresponds to a connectivty of 4 in 2D and 6 in 3D, while FullyConnected=True corresponds
to a connectivity of 8 in 2D and 26 in 3D. FullyConnected=False is also commonly referred to as “face
connected”.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional maxima are flat zones surrounded by pixels of lower value. A completely flat image will be marked as a regional maxima by this filter.

This code was contributed in the Insight Journal paper: "Finding regional extrema - methods and performance" by Beare R., Lehmann G. https://www.insight-journal.org/browse/publication/65 

### Author

 Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.

### Related Filters

- ValuedRegionalMinimaImageFilter 
- ValuedRegionalExtremaImageFilter 
- HMinimaImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| FullyConnected | bool | Whether the connected components are defined strictly by face connectivity (False) or by face+edge+vertex connectivity (True). Default is False |

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


## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


