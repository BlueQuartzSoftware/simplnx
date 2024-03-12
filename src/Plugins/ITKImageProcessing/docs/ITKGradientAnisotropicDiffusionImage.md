# ITK Gradient Anisotropic Diffusion Image Filter (ITKGradientAnisotropicDiffusionImage)

This filter performs anisotropic diffusion on a scalar itk::Image using the classic Perona-Malik, gradient magnitude based equation.

## Group (Subgroup)

ITKAnisotropicSmoothing (AnisotropicSmoothing)

## Description

For detailed information on anisotropic diffusion, see itkAnisotropicDiffusionFunction and itkGradientNDAnisotropicDiffusionFunction.

\par Inputs and Outputs
The input to this filter should be a scalar itk::Image of any dimensionality. The output image will be a diffused copy of the input.


\par Parameters
Please see the description of parameters given in itkAnisotropicDiffusionImageFilter.


\see AnisotropicDiffusionImageFilter 


\see AnisotropicDiffusionFunction 


\see GradientAnisotropicDiffusionFunction

## Parameters

| Name | Type | Description |
|------|------|-------------|
| TimeStep | float64 |  |
| ConductanceParameter | float64 |  |
| ConductanceScalingUpdateInterval | uint32 |  |
| NumberOfIterations | uint32 |  |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching RealPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching RealPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


