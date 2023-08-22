# ITK Erode Object Morphology Image Filter (ITKErodeObjectMorphologyImage)

Erosion of an object in an image.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

Erosion of an image using binary morphology. Pixel values matching the object value are considered the "object" and all other pixels are "background". This is useful in processing mask images containing only one object.

If the pixel covered by the center of the kernel has the pixel value ObjectValue and the pixel is adjacent to a non-object valued pixel, then the kernel is centered on the object-value pixel and neighboring pixels covered by the kernel are assigned the background value. The structuring element is assumed to be composed of binary values (zero or one).

\see ObjectMorphologyImageFilter , BinaryFunctionErodeImageFilter 


\see BinaryErodeImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| KernelRadius | uint32 | Set the radius of the kernel structuring element. |
| KernelType | KernelEnum | Set the kernel or structuring element used for the morphology. |
| ObjectValue | float64 |  |
| BackgroundValue | float64 | Set the value to be assigned to eroded pixels |

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
+ Class Name: ITKErodeObjectMorphologyImage
+ Displayed Name: ITK Erode Object Morphology Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| background_value | BackgroundValue | Set the value to be assigned to eroded pixels | complex.Float64Parameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| kernel_radius | KernelRadius | The radius of the kernel structuring element. | complex.VectorUInt32Parameter |
| kernel_type | KernelType | The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross | complex.ChoicesParameter |
| object_value | ObjectValue |  | complex.Float64Parameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

