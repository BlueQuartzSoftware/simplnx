# ITK Gradient Anisotropic Diffusion Image Filter (ITKGradientAnisotropicDiffusionImage)

This filter performs anisotropic diffusion on a scalar itk::Image using the classic Perona-Malik, gradient magnitude based equation.

## Group (Subgroup)

ITKAnisotropicSmoothing (AnisotropicSmoothing)

## Description

For detailed information on anisotropic diffusion, see itkAnisotropicDiffusionFunction and itkGradientNDAnisotropicDiffusionFunction.

## Parameter Information

The input and output to this filter must be a scalar itk::Image with numerical pixel types (float or double). A user defined type which correctly defines arithmetic operations with floating point accuracy should also give correct results.

Please first read all the documentation found in [AnisotropicDiffusionImageFilter](https://itk.org/Doxygen/html/classitk_1_1AnisotropicDiffusionImageFilter.html) and [AnisotropicDiffusionFunction](https://itk.org/Doxygen/html/classitk_1_1AnisotropicDiffusionFunction.html). Also see CurvatureNDAnisotropicDiffusionFunction.

- Set/GetNumberOfIterations specifies the number of iterations (time-step updates) that the solver will perform to produce a solution image. The appropriate number of iterations is dependent on the application and the image being processed. As a general rule, the more iterations performed, the more diffused the image will become.

- Set/GetTimeStep: In the anisotropic diffusion filter hierarchy, the time step is set explicitly by the user. The time step referred to here corresponds exactly to Δ𝑡 in the finite difference update equation described in FiniteDifferenceImageFilter (see itkFiniteDifferenceImageFilter for more information). Appropriate time steps for solving this type of p.d.e. depend on the dimensionality of the image and the order of the equation. Stable values for most 2D and 3D functions are 0.125 and 0.0625, respectively, when the pixel spacing is unity or is turned off. In general, you should keep the time step below (PixelSpacing)/2𝑁+1, where 𝑁 is the number of image dimensions. A filter will automatically attempt to constrain its time step to a stable value and generate a run-time warning if the time step is set too high.

- Set/GetConductanceParameter: The conductance parameter controls the sensitivity of the conductance term in the basic anisotropic diffusion equation. It affects the conductance term in different ways depending on the particular variation on the basic equation. As a general rule, the lower the value, the more strongly the diffusion equation preserves image features (such as high gradients or curvature). A high value for conductance will cause the filter to diffuse image features more readily. Typical values range from 0.5 to 2.0 for data like the Visible Human color data, but the correct value for your application is wholly dependent on the results you want from a specific data set and the number or iterations you perform.

[AnisotropicDiffusionImageFilter](https://itk.org/Doxygen/html/classitk_1_1AnisotropicDiffusionImageFilter.html)
[AnisotropicDiffusionFunction](https://itk.org/Doxygen/html/classitk_1_1AnisotropicDiffusionFunction.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
