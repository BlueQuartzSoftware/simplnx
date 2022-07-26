# ITK::FFT Normalized Correlation Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Calculate normalized cross correlation using FFTs.

This filter calculates the normalized cross correlation (NCC) of two images
using FFTs instead of spatial correlation. It is much faster than spatial
correlation for reasonably large structuring elements. This filter is a
subclass of the more general MaskedFFTNormalizedCorrelationImageFilter and
operates by essentially setting the masks in that algorithm to images of ones.
As described in detail in the references below, there is no computational
overhead to utilizing the more general masked algorithm because the FFTs of the
images of ones are still necessary for the computations.

Inputs: Two images are required as inputs, fixedImage and movingImage. In the
context of correlation, inputs are often defined as: "image" and "template". In
this filter, the fixedImage plays the role of the image, and the movingImage
plays the role of the template. However, this filter is capable of correlating
any two images and is not restricted to small movingImages (templates).

The RequiredNumberOfOverlappingPixels enables the user to specify how many voxels of the two images must overlap; any location in the correlation map that results from fewer than this number of voxels will be set to zero. Larger values zero-out pixels on a larger border around the correlation image. Thus, larger values remove less stable computations but also limit the capture range. If RequiredNumberOfOverlappingPixels is set to 0, the default, no zeroing will take place.

Image size: fixedImage and movingImage need not be the same size. Furthermore,
whereas some algorithms require that the "template" be smaller than the "image"
because of errors in the regions where the two are not fully overlapping, this
filter has no such restriction.

Image spacing: Since the computations are done in the pixel domain, all input
images must have the same spacing.

Outputs; The output is an image of RealPixelType that is the NCC of the two
images and its values range from -1.0 to 1.0. The size of this NCC image is, by
definition, size(fixedImage) + size(movingImage) - 1.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| RequiredNumberOfOverlappingPixels | size_t| See Description |
| RequiredFractionOfOverlappingPixels | double| See Description |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | float | (1)  | Array containing filtered image

## References ##

[1] T.S. Yoo, M. J. Ackerman, W. E. Lorensen, W. Schroeder, V. Chalana, S. Aylward, D. Metaxas, R. Whitaker. Engineering and Algorithm Design for an Image Processing API: A Technical Report on ITK - The Insight Toolkit. In Proc. of Medicine Meets Virtual Reality, J. Westwood, ed., IOS Press Amsterdam pp 586-592 (2002).
[2] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Design and Functionality. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-28-3
[3] H. Johnson, M. McCormick, L. Ibanez. The ITK Software Guide: Introduction and Development Guidelines. Fourth Edition. Published by Kitware Inc. 2015 ISBN: 9781-930934-27-6
[4] D. Padfield. "Masked object registration in the Fourier domain." Transactions on Image Processing.
[5] D. Padfield. "Masked FFT registration". In Proc. Computer Vision and Pattern Recognition, 2010.

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
