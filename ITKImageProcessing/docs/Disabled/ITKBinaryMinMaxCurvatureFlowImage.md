# ITK::Binary Min Max Curvature Flow Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Denoise a binary image using min/max curvature flow.

BinaryMinMaxCurvatureFlowImageFilter implements a curvature driven image denosing algorithm. This filter assumes that the image is essentially binary: consisting of two classes. Iso-brightness contours in the input image are viewed as a level set. The level set is then evolved using a curvature-based speed function:

 \f[ I_t = F_{\mbox{minmax}} |\nabla I| \f]

where \f$ F_{\mbox{minmax}} = \min(\kappa,0) \f$ if \f$ \mbox{Avg}_{\mbox{stencil}}(x) \f$ is less than or equal to \f$ T_{thresold} \f$ and \f$ \max(\kappa,0) \f$ , otherwise. \f$ \kappa \f$ is the mean curvature of the iso-brightness contour at point \f$ x \f$ .

In min/max curvature flow, movement is turned on or off depending on the scale of the noise one wants to remove. Switching depends on the average image value of a region of radius \f$ R \f$ around each point. The choice of \f$ R \f$ , the stencil radius, governs the scale of the noise to be removed.

The threshold value \f$ T_{threshold} \f$ is a user specified value which discriminates between the two pixel classes.

This filter make use of the multi-threaded finite difference solver hierarchy. Updates are computed using a BinaryMinMaxCurvatureFlowFunction object. A zero flux Neumann boundary condition is used when computing derivatives near the data boundary.

\warning This filter assumes that the input and output types have the same dimensions. This filter also requires that the output image pixels are of a real type. This filter works for any dimensional images.

Reference: "Level Set Methods and Fast Marching Methods", J.A. Sethian, Cambridge Press, Chapter 16, Second edition, 1999.

\see BinaryMinMaxCurvatureFlowFunction

\see CurvatureFlowImageFilter

\see MinMaxCurvatureFlowImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| TimeStep | double| N/A |
| NumberOfIterations | double| N/A |
| StencilRadius | int| N/A |
| Threshold | double| Set/Get the threshold value. |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None |  | (1)  | Array containing filtered image

## References ##

[1] T.S. Yoo, M. J. Ackerman, W. E. Lorensen, W. Schroeder, V. Chalana, S. Aylward, D. Metaxas, R. Whitaker. Engineering and Algorithm Design for an Image Processing API: A Technical Report on ITK - The Insight Toolkit. In Proc. of Medicine Meets Virtual Reality, J. Westwood, ed., IOS Press Amsterdam pp 586-592 (2002). 
[2] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Design and Functionality. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-28-3
[3] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Introduction and Development Guidelines. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-27-6

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
