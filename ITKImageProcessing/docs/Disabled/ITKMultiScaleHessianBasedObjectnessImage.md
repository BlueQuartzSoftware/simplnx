# ITK::Multi-Scale Hessian Based Objectness Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITK Edge)

## Description ##

Computes a feature image where the output represents
objectness. The type of objectness is determined by the **ObjectDimension**, M
(M = 0 for blobs, M = 1 for vessels, M = 2 for plates, M = 3 for hyper-plates,
...). The ObjectDimension is less than the image dimension. The objectness
measure is based the eigenvalues of the local Hessian. Three terms are
combined to make the objectness measure, two based on the ratio of eigenvalues
that have been ranked by their magnitude and a third based on the Frobenius
norm of the Hessian matrix.

For more information, see Antiga, L. "Generalizing vesselness with respect to
dimensionality and shape." The Insight Journal. 2007.
http://hdl.handle.net/1926/576
http://www.insight-journal.org/browse/publication/175

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| ObjectDimension | int| the dimensionality of the object (0: points (blobs), 1: lines (vessels), 2: planes (plate-like structures), 3: hyper-planes. ObjectDimension must be smaller than ImageDimension. |
| Alpha | double| Weight correponding to R_A (the ratio of the smallest eigenvalue that has to be large to the larger ones). Smaller values lead to increased sensitivity to the object dimensionality. |
| Beta | double| Weight correponding to R_B (the ratio of the largest eigenvalue that has to be small to the larger ones). Smaller values lead to increased sensitivity to the object dimensionality. |
| Gamma | double| The weight corresponding to S (the Frobenius norm of the Hessian matrix, or second-order structureness). |
| ScaleObjectnessMeasure | bool| Toggle scaling the objectness measure with the magnitude of the largest absolute eigenvalue. |
| BrightObject | bool| Enhance bright structures on a dark background if true, the opposite if false. |
| SigmaMinimum | double| Scale for the smallest Hessian estimator. |
| SigmaMaximum | double| Scale for the largest Hessian estimator. |
| NumberOfSigmaSteps | unsigned int| Number of scales to estimate. |

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

## Example Pipelines ##



## License & Copyright ##

Please see the description file distributed with this plugin.

## DREAM3D Mailing Lists ##

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users
