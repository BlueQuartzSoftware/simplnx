# ITK Double Threshold Image Filter (ITKDoubleThresholdImage)

Binarize an input image using double thresholding.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Double threshold addresses the difficulty in selecting a threshold that will select the objects of interest without selecting extraneous objects. Double threshold considers two threshold ranges: a narrow range and a wide range (where the wide range encompasses the narrow range). If the wide range was used for a traditional threshold (where values inside the range map to the foreground and values outside the range map to the background), many extraneous pixels may survive the threshold operation. If the narrow range was used for a traditional threshold, then too few pixels may survive the threshold.

Double threshold uses the narrow threshold image as a marker image and the wide threshold image as a mask image in the geodesic dilation. Essentially, the marker image (narrow threshold) is dilated but constrained to lie within the mask image (wide threshold). Thus, only the objects of interest (those pixels that survived the narrow threshold) are extracted but the those objects appear in the final image as they would have if the wide threshold was used.

\see GrayscaleGeodesicDilateImageFilter 


\see MorphologyImageFilter , GrayscaleDilateImageFilter , GrayscaleFunctionDilateImageFilter , BinaryDilateImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Threshold1 | float64 | Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4. |
| Threshold2 | float64 | Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4. |
| Threshold3 | float64 | Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4. |
| Threshold4 | float64 | Set the thresholds. Four thresholds should be specified. The two lower thresholds default to NumericTraits<InputPixelType>::NonpositiveMin() . The two upper thresholds default NumericTraits<InputPixelType>::max . Threshold1 <= Threshold2 <= Threshold3 <= Threshold4. |
| InsideValue | uint8 | Set the "inside" pixel value. The default value NumericTraits<OutputPixelType>::max() |
| OutsideValue | uint8 | Set the "outside" pixel value. The default value NumericTraits<OutputPixelType>::ZeroValue() . |
| FullyConnected | bool | Set/Get whether the connected components are defined strictly by face connectivity or by face+edge+vertex connectivity. Default is FullyConnectedOff. For objects that are 1 pixel wide, use FullyConnectedOn. |

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


