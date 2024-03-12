# ITK Min Max Curvature Flow Image Filter (ITKMinMaxCurvatureFlowImage)

Denoise an image using min/max curvature flow.

## Group (Subgroup)

ITKCurvatureFlow (CurvatureFlow)

## Description

MinMaxCurvatureFlowImageFilter implements a curvature driven image denoising algorithm. Iso-brightness contours in the grayscale input image are viewed as a level set. The level set is then evolved using a curvature-based speed function:

 \f[ I_t = F_{\mbox{minmax}} |\nabla I| \f] 

where \f$ F_{\mbox{minmax}} = \max(\kappa,0) \f$ if \f$ \mbox{Avg}_{\mbox{stencil}}(x) \f$ is less than or equal to \f$ T_{threshold} \f$ and \f$ \min(\kappa,0) \f$ , otherwise. \f$ \kappa \f$ is the mean curvature of the iso-brightness contour at point \f$ x \f$ .

In min/max curvature flow, movement is turned on or off depending on the scale of the noise one wants to remove. Switching depends on the average image value of a region of radius \f$ R \f$ around each point. The choice of \f$ R \f$ , the stencil radius, governs the scale of the noise to be removed.

The threshold value \f$ T_{threshold} \f$ is the average intensity obtained in the direction perpendicular to the gradient at point \f$ x \f$ at the extrema of the local neighborhood.

This filter make use of the multi-threaded finite difference solver hierarchy. Updates are computed using a MinMaxCurvatureFlowFunction object. A zero flux Neumann boundary condition is used when computing derivatives near the data boundary.

\warning This filter assumes that the input and output types have the same dimensions. This filter also requires that the output image pixels are of a real type. This filter works for any dimensional images, however for dimensions greater than 3D, an expensive brute-force search is used to compute the local threshold.


Reference: "Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.

\see MinMaxCurvatureFlowFunction 


\see CurvatureFlowImageFilter 


\see BinaryMinMaxCurvatureFlowImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| TimeStep | float64 |  |
| NumberOfIterations | uint32 |  |
| StencilRadius | int32 | Set/Get the stencil radius. |

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


