# ITK Gradient Magnitude Recursive Gaussian Image Filter

Computes the gradient magnitude (edge strength) of an image after Gaussian smoothing.

## Group (Subgroup)

ITKImageGradient (ImageGradient)

## Description

The **gradient magnitude** measures how fast intensity changes at each pixel — it is high along edges and near zero in flat regions, so the output is an edge-strength image. This filter first smooths the image with a Gaussian and then takes the gradient, combining the two steps so that the smoothing scale is controlled directly. Smoothing suppresses noise, so edges are detected more reliably than with a raw (un-smoothed) gradient.

The amount of smoothing is set by *Sigma*: a small *Sigma* responds to fine, sharp edges; a large *Sigma* blurs away fine detail and responds only to broad, large-scale edges.

Use this filter to produce an edge-strength map for edge detection, segmentation seeding (for example, as a speed image for level sets), or feature extraction.

### Parameter Guidance

- **Sigma** — the standard deviation (width) of the Gaussian smoothing kernel, measured in **image-spacing (physical) units**, not pixels. For example, with a pixel spacing of 0.5 µm, a *Sigma* of *1.0* corresponds to 2 pixels. Larger values smooth more and detect broader edges.
- **Normalize Across Scale** — controls how the response is scaled when *Sigma* changes. Leave this off for ordinary edge detection. Enable it only when you compare gradient magnitudes computed at several different *Sigma* values (multi-scale analysis), so that the strongest response is not simply the smallest *Sigma*.

### Required Input Sources

Operates on any scalar (grayscale) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
