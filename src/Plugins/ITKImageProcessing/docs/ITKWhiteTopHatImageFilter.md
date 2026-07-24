# ITK White Top Hat Image Filter

Extracts small bright features by subtracting the grayscale opening from the original image.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

The **white top-hat** isolates small **bright** features — bright spots and narrow bright ridges smaller than the **structuring element** (the probe shape swept over the image). It is computed as the original image minus its grayscale **opening**, which leaves the small bright details on a flat (near-zero) background. A common use is correcting uneven (bright) background illumination or extracting small bright details for measurement. The dark-feature counterpart is [ITK Black Top Hat Image Filter](ITKBlackTopHatImageFilter.md).

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

![White top-hat example.](Images/ITKWhiteTopHat.png)

## Reference

Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003 (Chapter 4.5).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
