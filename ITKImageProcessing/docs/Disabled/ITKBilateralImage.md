# ITK::Bilateral Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Blurs an image while preserving edges.

This filter uses bilateral filtering to blur an image using both domain and range "neighborhoods". Pixels that are close to a pixel in the image domain and similar to a pixel in the image range are used to calculate the filtered value. Two gaussian kernels (one in the image domain and one in the image range) are used to smooth the image. The result is an image that is smoothed in homogeneous regions yet has edges preserved. The result is similar to anisotropic diffusion but the implementation in non-iterative. Another benefit to bilateral filtering is that any distance metric can be used for kernel smoothing the image range. Hence, color images can be smoothed as vector images, using the CIE distances between intensity values as the similarity metric (the Gaussian kernel for the image domain is evaluated using CIE distances). A separate version of this filter will be designed for color and vector images.

Bilateral filtering is capable of reducing the noise in an image by an order of magnitude while maintaining edges.

The bilateral operator used here was described by Tomasi and Manduchi (Bilateral Filtering for Gray and ColorImages. IEEE ICCV. 1998.)

\see GaussianOperator

\see RecursiveGaussianImageFilter

\see DiscreteGaussianImageFilter

\see AnisotropicDiffusionImageFilter

\see Image

\see Neighborhood

\see NeighborhoodOperator

TodoSupport color images

Support vector images

\par Wiki Examples:

\li All Examples

\li Bilateral filter an image

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| DomainSigma | double| Convenience get/set methods for setting all domain parameters to the same values. |
| RangeSigma | double| Standard get/set macros for filter parameters. DomainSigma is specified in the same units as the Image spacing. RangeSigma is specified in the units of intensity. |
| NumberOfRangeGaussianSamples | double| Set/Get the number of samples in the approximation to the Gaussian used for the range smoothing. Samples are only generated in the range of [0, 4*m_RangeSigma]. Default is 100. |


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
