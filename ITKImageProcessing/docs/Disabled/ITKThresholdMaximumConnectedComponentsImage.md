# ITK::Threshold Maximum Connected Components Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

Finds the threshold value of an image based on maximizing the number of objects in the image that are larger than a given minimal size.

\par
This method is based on Topological Stable State Thresholding to calculate the threshold set point. This method is particularly effective when there are a large number of objects in a microscopy image. Compiling in Debug mode and enable the debug flag for this filter to print debug information to see how the filter focuses in on a threshold value. Please see the Insight Journal's MICCAI 2005 workshop for a complete description. References are below.

\par Parameters
The MinimumObjectSizeInPixels parameter is controlled through the class Get/SetMinimumObjectSizeInPixels() method. Similar to the standard itk::BinaryThresholdImageFilter the Get/SetInside and Get/SetOutside values of the threshold can be set. The GetNumberOfObjects() and GetThresholdValue() methods return the number of objects above the minimum pixel size and the calculated threshold value.

\par Automatic Thresholding in ITK
There are multiple methods to automatically calculate the threshold intensity value of an image. As of version 4.0, ITK has a Thresholding ( ITKThresholding ) module which contains numerous automatic thresholding methods.implements two of these. Topological Stable State Thresholding works well on images with a large number of objects to be counted.

\par References:
1) Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber
counting in immunoflourescent images". Insight Journal. ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005) Dspace handle: https://hdl.handle.net/1926/48 2) Pikaz A, Averbuch, A. "Digital image thresholding based on topological
stable-state". Pattern Recognition, 29(5): 829-843, 1996.

\par
Questions: email Ken Urish at ken.urish(at)gmail.com Please cc the itk list serve for archival purposes.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| MinimumObjectSizeInPixels | double| The pixel type must support comparison operators. Set the minimum pixel area used to count objects on the image. Thus, only objects that have a pixel area greater than the minimum pixel area will be counted as an object in the optimization portion of this filter. Essentially, it eliminates noise from being counted as an object. The default value is zero. |
| UpperBoundary | double| The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity. |
| InsideValue | int| The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity. |
| OutsideValue | int| The following Set/Get methods are for the binary threshold function. This class automatically calculates the lower threshold boundary. The upper threshold boundary, inside value, and outside value can be defined by the user, however the standard values are used as default if not set by the user. The default value of the: Inside value is the maximum pixel type intensity. Outside value is the minimum pixel type intensity. Upper threshold boundary is the maximum pixel type intensity. |


## Required Geometry ##

Image

## Required Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | N/A | (1)  | Array containing input image

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None | uint8_t | (1)  | Array containing filtered image

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
