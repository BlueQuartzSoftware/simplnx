# ITK Laplacian Recursive Gaussian Image Filter

Computes the Laplacian of Gaussian (LoG) of an image.

## Group (Subgroup)

ITKImageFeature (ImageFeature)

## Description

The Laplacian of Gaussian (LoG) first smooths the image with a Gaussian of a chosen width and then takes its second spatial derivative (the Laplacian). It is a classic blob and edge detector: it responds strongly to features whose size matches the Gaussian width, and the zero crossings of its output mark edge locations. A common workflow is to run this filter and then locate those edges with [ITK Zero Crossing Image Filter](ITKZeroCrossingImageFilter.md). This implementation uses ITK's recursive Gaussian filters, so the cost does not grow with the smoothing width.

### Parameter Guidance

- **Sigma**: Standard deviation of the Gaussian, measured in the units of the image spacing (e.g. micrometers, not pixels). Larger Sigma smooths more and tunes the filter to larger blobs/coarser edges.
- **Normalize Across Scale**: Selects the normalization factor applied to the Gaussian. Leave off (default) for standard LoG output; turn on when comparing responses computed at different Sigma values (scale-space analysis) so that magnitudes remain comparable across scales.

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
