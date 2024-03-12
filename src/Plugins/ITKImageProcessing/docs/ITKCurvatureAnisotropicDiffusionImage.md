# ITK Curvature Anisotropic Diffusion Image Filter (ITKCurvatureAnisotropicDiffusionImage)

This filter performs anisotropic diffusion on a scalar itk::Image using the modified curvature diffusion equation (MCDE).

## Group (Subgroup)

ITKAnisotropicSmoothing (AnisotropicSmoothing)

## Description

For detailed information on anisotropic diffusion and the MCDE see itkAnisotropicDiffusionFunction and itkCurvatureNDAnisotropicDiffusionFunction.

\par Inputs and Outputs
The input and output to this filter must be a scalar itk::Image with numerical pixel types (float or double). A user defined type which correctly defines arithmetic operations with floating point accuracy should also give correct results.


\par Parameters
Please first read all the documentation found in AnisotropicDiffusionImageFilter and AnisotropicDiffusionFunction . Also see CurvatureNDAnisotropicDiffusionFunction .


The default time step for this filter is set to the maximum theoretically stable value: 0.5 / 2^N, where N is the dimensionality of the image. For a 2D image, this means valid time steps are below 0.1250. For a 3D image, valid time steps are below 0.0625.

\see AnisotropicDiffusionImageFilter 


\see AnisotropicDiffusionFunction 


\see CurvatureNDAnisotropicDiffusionFunction

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


