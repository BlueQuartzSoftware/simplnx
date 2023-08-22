# ITK Acos Image Filter (ITKAcosImage)

Computes the inverse cosine of each pixel.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

filter_data.detail_desc=This filter is templated over the pixel type of the input image and the pixel type of the output image.

The filter walks over all the pixels in the input image, and for each pixel does do the following:



\li cast the pixel value to double , 


\li apply the std::acos() function to the double value 


\li cast the double value resulting from std::acos() to the pixel type of the output image 


\li store the casted value into the output image.



The filter expects both images to have the same dimension (e.g. both 2D, or both 3D, or both ND).

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


