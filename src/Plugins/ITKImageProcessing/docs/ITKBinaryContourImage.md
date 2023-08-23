# ITK Binary Contour Image Filter (ITKBinaryContourImage)

Labels the pixels on the border of the objects in a binary image.

## Group (Subgroup)

ITKImageLabel (ImageLabel)

## Description

BinaryContourImageFilter takes a binary image as input, where the pixels in the objects are the pixels with a value equal to ForegroundValue. Only the pixels on the contours of the objects are kept. The pixels not on the border are changed to BackgroundValue.

The connectivity can be changed to minimum or maximum connectivity with SetFullyConnected() . Full connectivity produces thicker contours.

https://www.insight-journal.org/browse/publication/217 

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- LabelContourImageFilter BinaryErodeImageFilter SimpleContourExtractorImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| FullyConnected | bool | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. |
| BackgroundValue | float64 | Set/Get the background value used to mark the pixels not on the border of the objects. |
| ForegroundValue | float64 | Set/Get the foreground value used to identify the objects in the input and output images. |

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

## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKBinaryContourImage
+ Displayed Name: ITK Binary Contour Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| background_value | BackgroundValue | Set/Get the background value used to mark the pixels not on the border of the objects. | complex.Float64Parameter |
| foreground_value | ForegroundValue | Set/Get the foreground value used to identify the objects in the input and output images. | complex.Float64Parameter |
| fully_connected | FullyConnected | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. | complex.BoolParameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

