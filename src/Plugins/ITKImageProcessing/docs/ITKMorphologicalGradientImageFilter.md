# ITK Morphological Gradient Image Filter

Highlights edges in a grayscale image by subtracting a local minimum-filtered image from a local maximum-filtered image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

The **morphological gradient** is an edge detector. At each pixel it computes the difference between a grayscale **dilation** (the maximum value in the neighborhood defined by the structuring element) and a grayscale **erosion** (the minimum value in that same neighborhood):

`morphological gradient = dilation − erosion`

Where the image is flat, the local maximum and minimum are nearly equal and the result is near zero (dark). Where intensity changes sharply — an edge — the maximum and minimum differ greatly and the result is large (bright). The output is therefore bright along object boundaries and dark in uniform regions. The width of the detected edges grows with the size of the structuring element.

### Parameter Guidance

- **Kernel Radius** — the radius of the structuring element, **in pixels** (one value per axis). Larger radii detect broader edges.

#### Kernel Type

The *Kernel Type* parameter selects the structuring element shape:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

### Required Input Sources

Operates on any scalar (grayscale) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
