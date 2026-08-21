# ITK Closing By Reconstruction Image Filter

Fills small dark gaps in a grayscale image while preserving the shape of the structures that remain.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

A standard morphological **closing** (dilation followed by erosion) fills in small dark holes and gaps that are smaller than the **structuring element**, but it also distorts the shapes of the larger features it touches. **Closing by reconstruction** achieves the same hole-filling effect while preserving the exact shape of the structures, by replacing the final erosion with a morphological reconstruction step.

It is defined as `ClosingByReconstruction(f) = ErosionByReconstruction(Dilation(f))`. In addition to filling small dark gaps, the operation tends to raise the contrast of the darkest regions.

### Parameter Guidance

- **Kernel Radius** — the radius of the structuring element, **in pixels** (one value per axis). Dark gaps smaller than this are filled.
- **Fully Connected** — controls neighbor connectivity during reconstruction (face-only versus face + edge + corner). Turn it on for thin, one-pixel-wide features.
- **Preserve Intensities** — when on, pixels that the operation did not need to change are restored to their exact original gray values (rather than the slightly altered values produced internally), so only the filled regions differ from the input.

#### Kernel Type

The *Kernel Type* parameter selects the structuring element shape:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

### Required Input Sources

Operates on any scalar (grayscale) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter. Compare with the standard [ITK Grayscale Morphological Closing Image Filter](ITKGrayscaleMorphologicalClosingImageFilter.md).

## Reference

Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003 (Chapter 6.3.9).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
