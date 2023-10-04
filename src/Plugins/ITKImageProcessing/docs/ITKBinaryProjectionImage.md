# ITK Binary Projection Image Filter

Binary projection.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

Image projection is a very common task in image analysis to reduce the dimension of an image. A base
filter is provided with some specialized filters which implement different projection methods.

This class was contributed to the Insight Journal by Gaetan Lehmann. The original paper can be found at <https://www.insight-journal.org/browse/publication/71>

### Author

 Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.

### Related Filters

- ProjectionImageFilter
- MedianProjectionImageFilter
- MeanProjectionImageFilter
- MeanProjectionImageFilter
- MaximumProjectionImageFilter
- MinimumProjectionImageFilter
- StandardDeviationProjectionImageFilter
- SumProjectionImageFilter

## Parameters

| Name | Type | Description |
|------------|------| --------------------------------- |
| ProjectionDimension | uint32 | The index of the projection dimension |
| ForegroundValue | float64 | Set the value in the image to consider as "foreground". Defaults to maximum value of PixelType. Subclasses may alias this to DilateValue or ErodeValue. |
| BackgroundValue | float64 | Set the value used as "background". Any pixel value which is not DilateValue is considered background. BackgroundValue is used for defining boundary conditions. Defaults to NumericTraits<PixelType>::NonpositiveMin() . |

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

## DREAM3DNX Help

Check out our GitHub community page at [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) to report bugs, ask the community for help, discuss features, or get help from the developers.
