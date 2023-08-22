# ITK Mask Image Filter (ITKMaskImage)

Mask an image with a mask.

## Group (Subgroup)

ITKImageIntensity (ImageIntensity)

## Description

This class is templated over the types of the input image type, the mask image type and the type of the output image. Numeric conversions (castings) are done by the C++ defaults.

The pixel type of the input 2 image must have a valid definition of the operator != with zero. This condition is required because internally this filter will perform the operation

\code
if pixel_from_mask_image != masking_value

 pixel_output_image = pixel_input_image

else

 pixel_output_image = outside_value

\endcode


The pixel from the input 1 is cast to the pixel type of the output image.

Note that the input and the mask images must be of the same size.

\warning Any pixel value other than masking value (0 by default) will not be masked out.


\see MaskNegatedImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| OutsideValue | float64 | Method to explicitly set the outside value of the mask. Defaults to 0 |
| MaskingValue | float64 | Method to explicitly set the masking value of the mask. Defaults to 0 |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching NonLabelPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching NonLabelPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


