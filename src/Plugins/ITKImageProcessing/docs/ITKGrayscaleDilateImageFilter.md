# ITK Grayscale Dilate Image Filter

Replaces each pixel with the maximum value in its neighborhood — a grayscale dilation that brightens the image and grows bright regions.

## Group (Subgroup)

ITKMathematicalMorphology (MathematicalMorphology)

## Description

Grayscale **dilation** replaces each pixel with the **maximum** value found within the neighborhood defined by the **structuring element** (the probe shape — ball, box, etc. — swept over the image). The effect is to brighten the image, grow bright regions, and shrink dark ones. It is the grayscale counterpart of binary dilation and the building block of grayscale opening, closing, and top-hat filters.

### Parameter Guidance

- **Kernel Radius** — the radius of the structuring element, **in pixels** (one value per axis). Larger radii grow bright regions more.

#### Kernel Type

The *Kernel Type* parameter selects the structuring element shape:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

### Required Input Sources

Operates on any scalar (grayscale) image — typically from [Read Image](../SimplnxCore/ReadImageFilter.md), [Read Images [3D Stack]](../SimplnxCore/ReadImageStackFilter.md), or the output of a prior ITK image filter.

![Grayscale dilation example.](Images/ITKGrayscaleDilation.png)

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
