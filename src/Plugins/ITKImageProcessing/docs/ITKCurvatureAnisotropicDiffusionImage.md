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


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
