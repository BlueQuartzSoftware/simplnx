# ITK::Morphological Watershed Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Watershed segmentation implementation with morphogical operators.

Watershed pixel are labeled 0. TOutputImage should be an integer type. Labels of output image are in no particular order. You can reorder the labels such that object labels are consecutive and sorted based on object size by passing the output of this filter to a RelabelComponentImageFilter .

The morphological watershed transform algorithm is described in Chapter 9.2 of Pierre Soille's book "Morphological Image Analysis:
Principles and Applications", Second Edition, Springer, 2003.

This code was contributed in the Insight Journal paper: "The watershed transform in ITK - discussion and new developments" by Beare R., Lehmann G. https://hdl.handle.net/1926/202 http://www.insight-journal.org/browse/publication/92

\author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

\see WatershedImageFilter , MorphologicalWatershedFromMarkersImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| Level | double| N/A |
| MarkWatershedLine | bool| Set/Get whether the watershed pixel must be marked or not. Default is true. Set it to false do not only avoid writing watershed pixels, it also decrease algorithm complexity. |
| FullyConnected | bool| Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | uint32_t | (1)  | Array containing filtered image

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
