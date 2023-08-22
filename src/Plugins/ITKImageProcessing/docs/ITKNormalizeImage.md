# ITK Normalize Image Filter (ITKNormalizeImage)

Normalize an image by setting its mean to zero and variance to one.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

filter_data.detail_desc=NormalizeImageFilter shifts and scales an image so that the pixels in the image have a zero mean and unit variance. This filter uses StatisticsImageFilter to compute the mean and variance of the input and then applies ShiftScaleImageFilter to shift and scale the pixels.

NB: since this filter normalizes the data such that the mean is at 0, and \f$-\sigma\f$ to \f$+\sigma\f$ is mapped to -1.0 to 1.0, output image integral types will produce an image that DOES NOT HAVE a unit variance due to 68% of the intensity values being mapped to the real number range of -1.0 to 1.0 and then cast to the output integral value.

\see NormalizeToConstantImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching BasicPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching BasicPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


