# ITK Valued Regional Maxima Image Filter (ITKValuedRegionalMaximaImage)

Transforms the image so that any pixel that is not a regional maxima is set to the minimum value for the pixel type. Pixels that are regional maxima retain their value.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Regional maxima are flat zones surrounded by pixels of lower value. A completely flat image will be marked as a regional maxima by this filter.

This code was contributed in the Insight Journal paper: "Finding regional extrema - methods and performance" by Beare R., Lehmann G. https://www.insight-journal.org/browse/publication/65 

### Author

 Richard Beare. Department of Medicine, Monash University, Melbourne, Australia.

### Related Filters

* ValuedRegionalMinimaImageFilter 
* ValuedRegionalExtremaImageFilter 
* HMinimaImageFilter

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


## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKValuedRegionalMaximaImage
+ Displayed Name: ITK Valued Regional Maxima Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| fully_connected | FullyConnected |  | complex.BoolParameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

