# H Convex Image Filter

Extract the h-convex regional maxima (the "domes") of a grayscale image.

## Group (Subgroup)

NXImageProcessing (Morphology)

## Description

Computes the *h-convex* transform of a grayscale image: **HConvex(f, h) = f − HMaxima(f, h)**. Each regional maximum of the input is replaced by its own height (its contrast to the surrounding level), **capped at the Height parameter**, while everything that is not part of a maximum becomes zero. A regional maximum of contrast `c` therefore maps to `min(c, h)`.

This isolates bright "domes" and measures their prominence: peaks taller than `Height` all map to `Height`, while shallower peaks map to their true contrast. It is a common pre-step for peak detection and marker extraction.

For example, with Height *10* the input `[50, 70, 50]` gives the output `[0, 10, 0]`.

The input array must have one component per tuple (a **scalar image**). The output has the same type and dimensions as the input.

For 64-bit integer images with values above 2^53 in magnitude, use this filter for peak contrast and not [H Maxima](HMaximaImageFilter.md), because H Maxima can round those values.

### Height

**Height** is in the intensity units of the input. It must be finite and not negative.

- An integer image needs a whole-number Height. A fractional value, such as *2.5*, fails preflight. To use a fractional Height, first convert the image to a floating-point type.
- A floating-point image accepts a fractional Height.
- Height *0* gives an output of zero everywhere.
- For an integer image, `input − Height` stops at the minimum of the input type and does not wrap around.
- Height is a float64 value, so it cannot represent every whole number above *2^53*.

### Signed Integer Images

The contrast of a peak is never negative, but in a signed integer image it can be larger than the maximum of the type. The filter then stops with an error that names the input array and the first tuple that failed.

| Input (int8) | Height | Contrast | Result |
|--------------|--------|----------|--------|
| `[-128, -1, -128]` | 255 | `[0, 127, 0]` | The output is `[0, 127, 0]`. |
| `[-128, 127, -128]` | 255 | `[0, 255, 0]` | Error: *255* does not fit in int8. |

To avoid the error, reduce Height or convert the image to a wider or floating-point type.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for maxima that are one voxel wide.

Geodesic morphology, the h-maxima transform, and the h-convex transform are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter lowers the input by **Height**, reconstructs that marker under the input by dilation, and subtracts the reconstruction from the input.

Arrays in memory and out-of-core arrays give the same result. The filter reads and writes out-of-core arrays in blocks, so it does not need a second complete image in memory.

If the filter stops with an error or is cancelled, the output array can hold partial results.

% Auto generated parameter table will be inserted here

## Example Pipelines

The **Grayscale Surface-Defect Morphology** bookmark uses Height 20 to measure bright features in a defect intensity image. [Example pipeline guide](https://github.com/BlueQuartzSoftware/simplnx/blob/develop/src/Plugins/NXImageProcessing/pipelines/11_Grayscale_Surface-Defect_Morphology/11_Grayscale_Surface-Defect_Morphology.md)

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
