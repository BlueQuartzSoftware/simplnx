# ITK Zero Crossing Image Filter (ITKZeroCrossingImage)

This filter finds the closest pixel to the zero-crossings (sign changes) in a signed itk::Image .

## Group (Subgroup)

ITKImageFeature (ImageFeature)

## Description

Pixels closest to zero-crossings are labeled with a foreground value. All other pixels are marked with a background value. The algorithm works by detecting differences in sign among neighbors using city-block style connectivity (4-neighbors in 2d, 6-neighbors in 3d, etc.).

\par Inputs and Outputs
The input to this filter is an itk::Image of arbitrary dimension. The algorithm assumes a signed data type (zero-crossings are not defined for unsigned int data types), and requires that operator>, operator<, operator==, and operator!= are defined.


\par 
The output of the filter is a binary, labeled image of user-specified type. By default, zero-crossing pixels are labeled with a default "foreground" value of itk::NumericTraits<OutputDataType>::OneValue() , where OutputDataType is the data type of the output image. All other pixels are labeled with a default "background" value of itk::NumericTraits<OutputDataType>::ZeroValue() .


\par Parameters
There are two parameters for this filter. ForegroundValue is the value that marks zero-crossing pixels. The BackgroundValue is the value given to all other pixels.


\see Image 


\see Neighborhood 


\see NeighborhoodOperator 


\see NeighborhoodIterator

## Parameters

| Name | Type | Description |
|------|------|-------------|
| ForegroundValue | uint8 | Set/Get the label value for zero-crossing pixels. |
| BackgroundValue | uint8 | Set/Get the label value for non-zero-crossing pixels. |

## Required Geometry

Image Geometry

## Required Objects

| Name |Type | Description |
|-----|------|-------------|
| Input Image Geometry | DataPath | DataPath to the Input Image Geometry |
| Input Image Data Array | DataPath | Path to input image with pixel type matching SignedPixelIDTypeList |

## Created Objects

| Name |Type | Description |
|-----|------|-------------|
| Output Image Data Array | DataPath | Path to output image with pixel type matching SignedPixelIDTypeList |

## Example Pipelines


## License & Copyright

Please see the description file distributed with this plugin.


## DREAM3D Mailing Lists

If you need more help with a filter, please consider asking your question on the DREAM3D Users mailing list:
https://groups.google.com/forum/?hl=en#!forum/dream3d-users


