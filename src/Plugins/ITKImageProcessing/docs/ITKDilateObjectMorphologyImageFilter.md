# ITK Dilate Object Morphology Image Filter

Grows (dilates) a single labeled object in a mask image — a fast single-object variant of binary dilation.

## Group (Subgroup)

ITKBinaryMathematicalMorphology (BinaryMathematicalMorphology)

## Description

This filter **dilates one object** in a mask image. Pixels whose value equals the *Object Value* are treated as the object (foreground); all other pixels are background. Wherever an object pixel is adjacent to a background pixel, the **structuring element** (the probe shape swept over the image) is centered on the object pixel and the background pixels it covers are set to the object value, growing the object outward. It is intended for masks that contain a single object; to dilate any of several segment values in a general segmented image, use [ITK Binary Dilate Image Filter](ITKBinaryDilateImageFilter.md).

### Parameter Guidance

- **Object Value** — the pixel value of the object to dilate. Default *1*.
- **Kernel Radius** — the radius of the structuring element, **in pixels** (one value per axis). The object grows by this amount.

#### Kernel Type

The *Kernel Type* parameter selects the structuring element shape:

- **Annulus [0]**: A ring-shaped structuring element.
- **Ball [1]**: A spherical structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element.

### Required Input Sources

Operates on a binary/single-object mask image — typically the output of a thresholding filter such as [ITK Binary Threshold Image Filter](ITKBinaryThresholdImageFilter.md) or [Multi-Threshold Objects](../SimplnxCore/MultiThresholdObjectsFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
