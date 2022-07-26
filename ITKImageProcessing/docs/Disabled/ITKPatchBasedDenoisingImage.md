# ITK::Patch Based Denoising Image Filter (KW)  #


## Group (Subgroup) ##

ImageProcessing (ITK ImageProcessing)

## Description ##

Derived class implementing a specific patch-based denoising algorithm, as detailed below.

This class is derived from the base class PatchBasedDenoisingBaseImageFilter ; please refer to the documentation of the base class first. This class implements a denoising filter that uses iterative non-local, or semi-local, weighted averaging of image patches for image denoising. The intensity at each pixel 'p' gets updated as a weighted average of intensities of a chosen subset of pixels from the image.

This class implements the denoising algorithm using a Gaussian kernel function for nonparametric density estimation. The class implements a scheme to automatically estimated the kernel bandwidth parameter (namely, sigma) using leave-one-out cross validation. It implements schemes for random sampling of patches non-locally (from the entire image) as well as semi-locally (from the spatial proximity of the pixel being denoised at the specific point in time). It implements a specific scheme for defining patch weights (mask) as described in Awate and Whitaker 2005 IEEE CVPR and 2006 IEEE TPAMI.

\see PatchBasedDenoisingBaseImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| KernelBandwidthSigma | double| Set/Get initial kernel bandwidth estimate. To prevent the class from automatically modifying this estimate, set KernelBandwidthEstimation to false in the base class. |
| PatchRadius | double| Set/Get the patch radius specified in physical coordinates. Patch radius is preferably set to an even number. Currently, only isotropic patches in physical space are allowed; patches can be anisotropic in voxel space.
 |
| NumberOfIterations | double| Set/Get the number of denoising iterations to perform. Must be a positive integer. Defaults to 1.
 |
| NumberOfSamplePatches | double| Set/Get the number of patches to sample for each pixel.
 |
| SampleVariance | double| Set/Get the variance of the domain where patches are sampled.
 |
| NoiseSigma | double| Set/Get the noise sigma. Used by the noise model where appropriate, defaults to 5% of the image intensity range |
| NoiseModelFidelityWeight | double| Set/Get the weight on the fidelity term (penalizes deviations from the noisy data). This option is used when a noise model is specified. This weight controls the balance between the smoothing and the closeness to the noisy data.
 |
| AlwaysTreatComponentsAsEuclidean | bool| Set/Get flag indicating whether all components should always be treated as if they are in euclidean space regardless of pixel type. Defaults to false.
 |
| KernelBandwidthEstimation | bool| Set/Get flag indicating whether kernel-bandwidth should be estimated automatically from the image data. Defaults to true.
 |
| KernelBandwidthMultiplicationFactor | double| Set/Get the kernel bandwidth sigma multiplication factor used to modify the automatically-estimated kernel bandwidth sigma. At times, it may be desirable to modify the value of the automatically-estimated sigma. Typically, this number isn't very far from 1. Note: This is used only when KernelBandwidthEstimation is True/On. |
| KernelBandwidthUpdateFrequency | double| Set/Get the update frequency for the kernel bandwidth estimation. An optimal bandwidth will be re-estimated based on the denoised image after every 'n' iterations. Must be a positive integer. Defaults to 3, i.e. bandwidth updated after every 3 denoising iteration.
 |
| KernelBandwidthFractionPixelsForEstimation | double| Set/Get the fraction of the image to use for kernel bandwidth sigma estimation. To reduce the computational burden for computing sigma, a small random fraction of the image pixels can be used. |


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
