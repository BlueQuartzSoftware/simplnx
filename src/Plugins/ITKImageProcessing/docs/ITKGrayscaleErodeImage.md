# ITK Grayscale Erode Image Filter (ITKGrayscaleErodeImage)

Grayscale erosion of an image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Erode an image using grayscale morphology. Erosion takes the maximum of all the pixels identified by the structuring element.

The structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring element having values > 0 are candidates for affecting the center pixel.* MorphologyImageFilter , GrayscaleFunctionErodeImageFilter , BinaryErodeImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| KernelRadius | uint32 | Set the radius of the kernel structuring element. |
| KernelType | KernelEnum | Set the kernel or structuring element used for the morphology. |

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
+ Class Name: ITKGrayscaleErodeImage
+ Displayed Name: ITK Grayscale Erode Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| kernel_radius | KernelRadius | The radius of the kernel structuring element. | complex.VectorUInt32Parameter |
| kernel_type | KernelType | The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross | complex.ChoicesParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

