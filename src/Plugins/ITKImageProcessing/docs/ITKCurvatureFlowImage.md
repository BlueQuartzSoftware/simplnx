# ITK Curvature Flow Image Filter

Denoise an image using curvature driven flow.

## Group (Subgroup)

ITKCurvatureFlow (CurvatureFlow)

## Description

CurvatureFlowImageFilter implements a curvature driven image denoising algorithm. Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is then evolved using a curvature-based speed function:

 \f[ I_t = \kappa |\nabla I| \f] where \f$ \kappa \f$ is the curvature.

The advantage of this approach is that sharp boundaries are preserved with smoothing occurring only within a region. However, it should be noted that continuous application of this scheme will result in the eventual removal of all information as each contour shrinks to zero and disappear.

Note that unlike level set segmentation algorithms, the image to be denoised is already the level set and can be set directly as the input using the SetInput() method.

This filter has two parameters: the number of update iterations to be performed and the timestep between each update.

The timestep should be "small enough" to ensure numerical stability. Stability is guarantee when the timestep meets the CFL (Courant-Friedrichs-Levy) condition. Broadly speaking, this condition ensures that each contour does not move more than one grid position at each timestep. In the literature, the timestep is typically user specified and have to manually tuned to the application.

This filter make use of the multi-threaded finite difference solver hierarchy. Updates are computed using a CurvatureFlowFunction object. A zero flux Neumann boundary condition when computing derivatives near the data boundary.

This filter may be streamed. To support streaming this filter produces a padded output which takes into account edge effects. The size of the padding is m_NumberOfIterations on each edge. Users of this filter should only make use of the center valid central region.

## Warning

This filter assumes that the input and output types have the same dimensions. This filter also requires that the output image pixels are of a floating point type. This filter works for any dimensional images.

## Reference

"Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.

## See Also

- [DenseFiniteDifferenceImageFilter](https://itk.org/Doxygen/html/classitk_1_1DenseFiniteDifferenceImageFilter.html)

- [CurvatureFlowFunction](https://itk.org/Doxygen/html/classitk_1_1CurvatureFlowFunction.html)

- [MinMaxCurvatureFlowImageFilter](https://itk.org/Doxygen/html/classitk_1_1MinMaxCurvatureFlowImageFilter.html)

- [BinaryMinMaxCurvatureFlowImageFilter](https://itk.org/Doxygen/html/classitk_1_1BinaryMinMaxCurvatureFlowImageFilter.html)


% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
