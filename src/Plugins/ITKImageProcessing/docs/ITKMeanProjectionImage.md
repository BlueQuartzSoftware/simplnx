# ITK Mean Projection Image Filter (ITKMeanProjectionImage)

Mean projection.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

This class was contributed to the Insight Journal by Gaetan Lehmann. The original paper can be found at https://www.insight-journal.org/browse/publication/71 

\author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.


\see ProjectionImageFilter 


\see MedianProjectionImageFilter 


\see MinimumProjectionImageFilter 


\see StandardDeviationProjectionImageFilter 


\see SumProjectionImageFilter 


\see BinaryProjectionImageFilter 


\see MaximumProjectionImageFilter

## Parameters

| Name | Type | Description |
|------|------|-------------|
| ProjectionDimension | uint32 |  |

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

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GItHub site where the community of DREAM3D-NX users can help answer your questions.
