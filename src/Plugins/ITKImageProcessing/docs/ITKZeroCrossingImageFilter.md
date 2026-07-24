# ITK Zero Crossing Image Filter

Marks the pixels that sit at sign changes (zero crossings) of a signed image, which typically correspond to edges.

## Group (Subgroup)

ITKImageFeature (ImageFeature)

## Description

A **zero crossing** is a place where a signed image changes sign — going from positive to negative or vice versa. When the input is a second-derivative image such as a Laplacian, these sign changes line up with the edges in the original image, so detecting them is a common edge-finding step.

This filter examines each pixel and its immediate neighbors using city-block (face) connectivity — 4 neighbors in 2D, 6 in 3D. Where a sign change is found, the pixel nearest the crossing is labeled as foreground; every other pixel is labeled as background. The result is a thin, binary edge map.

The input must be a **signed** image. Zero crossings are not defined for unsigned data, since unsigned values never go negative.

Use this filter after computing a Laplacian (or other signed response) to extract edge locations.

### Parameter Guidance

- **Foreground Value** — the label written to pixels that lie on a zero crossing (the detected edges). Default *1*.
- **Background Value** — the label written to all other pixels. Default *0*.

### Required Input Sources

Requires a **signed** scalar image whose sign changes are meaningful — most commonly the output of a Laplacian-of-Gaussian step such as [ITK Laplacian Recursive Gaussian Image Filter](ITKLaplacianRecursiveGaussianImageFilter.md).

## See Also

- [ITK Laplacian Recursive Gaussian Image Filter](ITKLaplacianRecursiveGaussianImageFilter.md)
- [ITK ZeroCrossingImageFilter (ITK Doxygen)](https://itk.org/Doxygen/html/classitk_1_1ZeroCrossingImageFilter.html)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
