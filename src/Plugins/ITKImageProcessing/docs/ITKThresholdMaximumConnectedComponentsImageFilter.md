# ITK Threshold Maximum Connected Components Image Filter

Automatically picks the threshold that yields the most distinct objects above a minimum size, then produces a binary image.

## Group (Subgroup)

ITKConnectedComponents (ConnectedComponents)

## Description

This filter automatically selects a threshold rather than asking you for an intensity cutoff. It searches for the threshold value that **maximizes the number of connected objects** larger than a given minimum size, then binarizes the image at that threshold. Because the chosen threshold separates as many real objects as possible while merging or dropping noise-sized blobs, the filter is well suited to **counting many small objects** — for example cells or particles in a microscopy image.

The lower threshold boundary is computed by the filter; you supply the upper boundary of the search and the minimum object size. Pixels that pass the selected threshold are written with the *Inside Value* and all others with the *Outside Value*.

### Parameter Guidance

- **Minimum Object Size In Pixels** — the smallest connected region (in **pixels**) that counts as an object. Regions smaller than this are treated as noise and ignored when the filter optimizes the threshold. Default *0* (count everything).
- **Upper Boundary** — the upper limit of the threshold search, in the **input image's intensity units**. The filter computes the lower boundary automatically. Default is the maximum intensity of the pixel type.
- **Inside Value** — the output value written for pixels that pass the selected threshold (a `uint8` label, default *1*).
- **Outside Value** — the output value written for all other pixels (a `uint8` label, default *0*).

### Required Input Sources

Operates on any scalar image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## References

1. Urish KL, August J, Huard J. "Unsupervised segmentation for myofiber counting in immunofluorescent microscopy images." Insight Journal, ISC/NA-MIC/MICCAI Workshop on Open-Source Software (2005). <https://insight-journal.org/browse/publication/40>
2. Pikaz A, Averbuch A. "Digital image thresholding based on topological stable-state." Pattern Recognition, 29(5): 829-843, 1996.

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
