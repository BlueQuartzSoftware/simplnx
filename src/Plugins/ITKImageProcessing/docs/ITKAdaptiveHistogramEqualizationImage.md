# ITK Adaptive Histogram Equalization Image Filter (ITKAdaptiveHistogramEqualizationImage)

Power Law Adaptive Histogram Equalization.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

filter_data.detail_desc=Histogram equalization modifies the contrast in an image. The AdaptiveHistogramEqualizationImageFilter is a superset of many contrast enhancing filters. By modifying its parameters (alpha, beta, and window), the AdaptiveHistogramEqualizationImageFilter can produce an adaptively equalized histogram or a version of unsharp mask (local mean subtraction). Instead of applying a strict histogram equalization in a window about a pixel, this filter prescribes a mapping function (power law) controlled by the parameters alpha and beta.

The parameter alpha controls how much the filter acts like the classical histogram equalization method (alpha=0) to how much the filter acts like an unsharp mask (alpha=1).

The parameter beta controls how much the filter acts like an unsharp mask (beta=0) to much the filter acts like pass through (beta=1, with alpha=1).

The parameter window controls the size of the region over which local statistics are calculated. The size of the window is controlled by SetRadius the default Radius is 5 in all directions.

By altering alpha, beta and window, a host of equalization and unsharp masking filters is available.

The boundary condition ignores the part of the neighborhood outside the image, and over-weights the valid part of the neighborhood.

For detail description, reference "Adaptive Image Contrast
Enhancement using Generalizations of Histogram Equalization." J.Alex Stark. IEEE Transactions on Image Processing, May 2000.

## Parameters

| Name | Type | Description |
|------|------|-------------|
| Radius | uint32 |  |
| Alpha | float32 | Set/Get the value of alpha. Alpha = 0 produces the adaptive histogram equalization (provided beta=0). Alpha = 1 produces an unsharp mask. Default is 0.3. |
| Beta | float32 | Set/Get the value of beta. If beta = 1 (and alpha = 1), then the output image matches the input image. As beta approaches 0, the filter behaves as an unsharp mask. Default is 0.3. |

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


