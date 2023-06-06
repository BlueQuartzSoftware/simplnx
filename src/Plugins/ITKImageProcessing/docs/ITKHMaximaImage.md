# ITK::H Maxima Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Suppress local maxima whose height above the baseline is less than h.

HMaximaImageFilter suppresses local maxima that are less than h intensity units above the (local) background. This has the effect of smoothing over the "high" parts of the noise in the image without smoothing over large changes in intensity (region boundaries). See the HMinimaImageFilter to suppress the local minima whose depth is less than h intensity units below the (local) background.

If the output of HMaximaImageFilter is subtracted from the original image, the signicant "peaks" in the image can be identified. This is what the HConvexImageFilter provides.

This filter uses the ReconstructionByDilationImageFilter . It provides its own input as the "mask" input to the geodesic dilation. The "marker" image for the geodesic dilation is the input image minus the height parameter h.

Geodesic morphology and the H-Maxima algorithm is described in Chapter 6 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.

The height parameter is set using SetHeight.

\see ReconstructionByDilationImageFilter , HMinimaImageFilter , HConvexImageFilter

\see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Height | double| Set/Get the height that a local maximum must be above the local background (local contrast) in order to survive the processing. Local maxima below this value are replaced with an estimate of the local background. |


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
