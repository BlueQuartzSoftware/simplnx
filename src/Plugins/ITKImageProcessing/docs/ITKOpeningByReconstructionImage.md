# ITK Opening By Reconstruction Image Filter (ITKOpeningByReconstructionImage)

Opening by reconstruction of an image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

This filter preserves regions, in the foreground, that can completely contain the structuring element. At the same time, this filter eliminates all other regions of foreground pixels. Contrary to the morphological opening, the opening by reconstruction preserves the shape of the components that are not removed by erosion. The opening by reconstruction of an image "f" is defined as:

OpeningByReconstruction(f) = DilationByReconstruction(f, Erosion(f)).

Opening by reconstruction not only removes structures destroyed by the erosion, but also levels down the contrast of the brightest regions. If PreserveIntensities is on, a subsequent reconstruction by dilation using a marker image that is the original image for all unaffected pixels.

Opening by reconstruction is described in Chapter 6.3.9 of Pierre Soille's book "Morphological Image Analysis: Principles and
Applications", Second Edition, Springer, 2003.

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

* GrayscaleMorphologicalOpeningImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| KernelRadius | uint32 | Set the radius of the kernel structuring element. |
| KernelType | KernelEnum | Set the kernel or structuring element used for the morphology. |
| FullyConnected | bool | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. |
| PreserveIntensities | bool | Set/Get whether the original intensities of the image retained for those pixels unaffected by the opening by reconstruction. If Off, the output pixel contrast will be reduced. |

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
+ Class Name: ITKOpeningByReconstructionImage
+ Displayed Name: ITK Opening By Reconstruction Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| fully_connected | FullyConnected | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. | complex.BoolParameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| kernel_radius | KernelRadius | The radius of the kernel structuring element. | complex.VectorUInt32Parameter |
| kernel_type | KernelType | The shape of the kernel to use. 0=Annulas, 1=Ball, 2=Box, 3=Cross | complex.ChoicesParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| preserve_intensities | PreserveIntensities | Set/Get whether the original intensities of the image retained for those pixels unaffected by the opening by reconstruction. If Off, the output pixel contrast will be reduced. | complex.BoolParameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

