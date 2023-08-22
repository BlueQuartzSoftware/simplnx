# ITK Sigmoid Image Filter (ITKSigmoidImage)

Computes the sigmoid function pixel-wise.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

A linear transformation is applied first on the argument of the sigmoid function. The resulting total transform is given by

![Images/ITKSigmoidImage_Equation.png](Images/ITKSigmoidImage_Equation.png)

Every output pixel is equal to f(x). Where x is the intensity of the homologous input pixel, and alpha and beta are user-provided constants.

## Parameters

| Name | Type | Description              |
|------|------|--------------------------|
| Alpha | float64 | Alpha in the above equation |
| Beta | float64 | Beta in the abvove equation |
| OutputMaximum | float64 | The maximum output value |
| OutputMinimum | float64 | The minimum output value |

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


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users




## Python Filter Arguments

+ module: ITKImageProcessing
+ Class Name: ITKSigmoidImage
+ Displayed Name: ITK Sigmoid Image Filter

| argument key | Human Name | Description | Parameter Type |
|--------------|------------|-------------|----------------|
| alpha | Alpha | The Alpha value from the Sigmoid equation.  | complex.Float64Parameter |
| beta | Beta | The Beta value from teh sigmoid equation | complex.Float64Parameter |
| input_image_data_path | Input Image Data Array | The image data that will be processed by this filter. | complex.ArraySelectionParameter |
| output_image_data_path | Output Image Data Array | The result of the processing will be stored in this Data Array. | complex.DataObjectNameParameter |
| output_maximum | OutputMaximum | The maximum output value | complex.Float64Parameter |
| output_minimum | OutputMinimum | The minimum output value | complex.Float64Parameter |
| selected_image_geom_path | Image Geometry | Select the Image Geometry Group from the DataStructure. | complex.GeometrySelectionParameter |

