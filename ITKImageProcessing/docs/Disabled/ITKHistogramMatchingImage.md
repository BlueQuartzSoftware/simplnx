# ITK::Histogram Matching Image Filter (KW)  #


## Group (Subgroup) ##

ImageProcessing (ITK IntensityTransformation)

## Description ##

Normalize the grayscale values between two images by histogram matching.

HistogramMatchingImageFilter normalizes the grayscale values of a source image based on the grayscale values of a reference image. This filter uses a histogram matching technique where the histograms of the two images are matched only at a specified number of quantile values.

This filter was originally designed to normalize MR images of the same MR protocol and same body part. The algorithm works best if background pixels are excluded from both the source and reference histograms. A simple background exclusion method is to exclude all pixels whose grayscale values are smaller than the mean grayscale value. ThresholdAtMeanIntensityOn() switches on this simple background exclusion method.

The source image can be set via either SetInput() or SetSourceImage() . The reference image can be set via SetReferenceImage() .

SetNumberOfHistogramLevels() sets the number of bins used when creating histograms of the source and reference images. SetNumberOfMatchPoints() governs the number of quantile values to be matched.

This filter assumes that both the source and reference are of the same type and that the input and output image type have the same number of dimension and have scalar pixel types.

\par REFERENCE
Laszlo G. Nyul, Jayaram K. Udupa, and Xuan Zhang, "New Variants of a Method
of MRI Scale Standardization", IEEE Transactions on Medical Imaging, 19(2):143-150, 2000.

## Parameters ##

| Name | Type | Description |
|------|------|-------------|
| NumberOfHistogramLevels | double| Set/Get the number of histogram levels used. |
| NumberOfMatchPoints | double| Set/Get the number of match points used. |
| ThresholdAtMeanIntensity | bool| Set/Get the threshold at mean intensity flag. If true, only source (reference) pixels which are greater than the mean source (reference) intensity is used in the histogram matching. If false, all pixels are used. |


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
