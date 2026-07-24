# ITK Black Top Hat Image Filter

Extracts small dark features by subtracting the original image from its grayscale closing.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

The **black top-hat** isolates small **dark** features — dark spots and narrow dark valleys smaller than the **structuring element** (the probe shape swept over the image). It is computed as the grayscale **closing** of the image minus the original image, which turns the dark valleys into bright peaks on a flat (near-zero) background. A common use is correcting uneven dark background or extracting small dark details for measurement. The bright-feature counterpart is [ITK White Top Hat Image Filter](ITKWhiteTopHatImageFilter.md).

### Parameter Guidance

- **Kernel Radius** — the radius of the structuring element, **in pixels** (one value per axis). It sets the size scale: features smaller than the structuring element are extracted, larger structures are removed.
- **Safe Border** — when on (default), a temporary border is added during processing to avoid edge artifacts and removed afterward.

#### Kernel Type

The *Kernel Type* parameter selects the structuring element shape:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

### Required Input Sources

Operates on any scalar (grayscale) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

![Black top-hat example.](Images/ITKBlackTopHat_1.png)
![Black top-hat example.](Images/ITKBlackTopHat_2.png)

## Reference

Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003 (Chapter 4.5).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
