# ITK Binary Projection Image Filter (ITKBinaryProjectionImage)

Binary projection.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

This class was contributed to the Insight Journal by Gaetan Lehmann. The original paper can be found at https://www.insight-journal.org/browse/publication/71 

\author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.


\see ProjectionImageFilter 


\see MedianProjectionImageFilter 


\see MeanProjectionImageFilter 


\see MeanProjectionImageFilter 


\see MaximumProjectionImageFilter 


\see MinimumProjectionImageFilter 


\see StandardDeviationProjectionImageFilter 


\see SumProjectionImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| ProjectionDimension | uint32 |  |
| ForegroundValue | float64 | Set the value in the image to consider as "foreground". Defaults to maximum value of PixelType. Subclasses may alias this to DilateValue or ErodeValue. |
| BackgroundValue | float64 | Set the value used as "background". Any pixel value which is not DilateValue is considered background. BackgroundValue is used for defining boundary conditions. Defaults to NumericTraits<PixelType>::NonpositiveMin() . |

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

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKBinaryProjectionImage
+ Displayed Name: ITK Binary Projection Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| background_value | BackgroundValue | Set the value used as 'background'. Any pixel value which is not DilateValue is considered background. BackgroundValue is used for defining boundary conditions. Defaults to NumericTraits<PixelType>::NonpositiveMin() . | complex.Float64Parameter |
| foreground_value | ForegroundValue | Set the value in the image to consider as 'foreground'. Defaults to maximum value of PixelType. Subclasses may alias this to DilateValue or ErodeValue. | complex.Float64Parameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| projection_dimension | ProjectionDimension |  | complex.UInt32Parameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

