# ITK Sigmoid Image Filter

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


## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.


