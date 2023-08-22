# ITK::Binary Dilate Image Filter (KW)  #


## Group (Subgroup) ##

ITKImageProcessing (ITKImageProcessing)

## Description ##

BinaryDilateImageFilter is a binary dilation morphologic operation on the foreground of an image. Only the value designated by the intensity value \"SetForegroundValue()\" (alias as SetDilateValue() ) is considered as foreground, and other intensity values are considered background.\n\nGrayscale images can be processed as binary images by selecting a \"ForegroundValue\" (alias \"DilateValue\"). Pixel values matching the dilate value are considered the \"foreground\" and all other pixels are \"background\". This is useful in processing segmented images where all pixels in segment #1 have value 1 and pixels in segment #2 have value 2, etc. A particular \"segment number\" can be processed. ForegroundValue defaults to the maximum possible value of the PixelType.\n\nThe structuring element is assumed to be composed of binary values (zero or one). Only elements of the structuring element having values > 0 are candidates for affecting the center pixel. A reasonable choice of structuring element is itk::BinaryBallStructuringElement .\n\nThis implementation is based on the papers:\n\nL.Vincent \"Morphological transformations of binary images with\narbitrary structuring elements\", and\n\nN.Nikopoulos et al. \"An efficient algorithm for 3d binary\nmorphological transformations with 3d structuring elements\nfor arbitrary size and shape\". IEEE Transactions on Image Processing. Vol. 9. No. 3. 2000. pp. 283-286.\n\n\\see ImageToImageFilter BinaryErodeImageFilter BinaryMorphologyImageFilter

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| BackgroundValue | double| N/A |
| ForegroundValue | double| N/A |
| BoundaryToForeground | bool| N/A |
| KernelRadius | FloatVec3_t| N/A |
| KernelType | int| N/A |


## Required Geometry ##

Image

## Required Objects ##

| Kind                     | Default Name | Type | Component Dimensions | Description                  |
|--------------------------|--------------|------|----------------------|------------------------------|
| **Cell Attribute Array** | None         | N/A  | (1)                  | Array containing input image |

## Created Objects ##

| Kind | Default Name | Type | Component Dimensions | Description |
|------|--------------|------|----------------------|-------------|
| **Cell Attribute Array** | None |  | (1)  | Array containing filtered image|

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
