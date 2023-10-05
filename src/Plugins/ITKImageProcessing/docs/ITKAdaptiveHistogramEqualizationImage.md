# ITK Adaptive Histogram Equalization Image Filter

Power Law Adaptive Histogram Equalization.

## Group (Subgroup)

ITKImageStatistics (ImageStatistics)

## Description

Histogram equalization modifies the contrast in an image. The AdaptiveHistogramEqualizationImageFilter is a superset of many contrast enhancing filters. By modifying its parameters (alpha, beta, and window), the AdaptiveHistogramEqualizationImageFilter can produce an adaptively equalized histogram or a version of unsharp mask (local mean subtraction). Instead of applying a strict histogram equalization in a window about a pixel, this filter prescribes a mapping function (power law) controlled by the parameters alpha and beta.

The parameter alpha controls how much the filter acts like the classical histogram equalization method (alpha=0) to how much the filter acts like an unsharp mask (alpha=1).

The parameter beta controls how much the filter acts like an unsharp mask (beta=0) to much the filter acts like pass through (beta=1, with alpha=1).

The parameter window controls the size of the region over which local statistics are calculated. The size of the window is controlled by SetRadius the default Radius is 5 in all directions.

By altering alpha, beta and window, a host of equalization and unsharp masking filters is available.

The boundary condition ignores the part of the neighborhood outside the image, and over-weights the valid part of the neighborhood.

For detail description, reference "Adaptive Image Contrast Enhancement using Generalizations of Histogram Equalization." J.Alex Stark. IEEE Transactions on Image Processing, May 2000.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues) GItHub site where the community of DREAM3D-NX users can help answer your questions.
