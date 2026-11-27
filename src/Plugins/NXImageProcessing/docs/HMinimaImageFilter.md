# H Minima Image Filter

Suppress local minima whose depth (contrast) is below a user-supplied value.

## Group (Subgroup)

NXImageProcessing (Morphology)

## Description

Applies the **H-Minima transform** to a grayscale image. A **regional minimum** is a connected region below its surrounding intensities. Its **depth** is the intensity difference to that surrounding level. The transform suppresses minima whose depth is less than **Height**. Deeper minima rise by Height, subject to the input type's maximum.

The H-Minima transform is commonly used to remove insignificant intensity valleys caused by noise prior to a minima-detection or watershed step, so that only sufficiently prominent minima remain. It is the erosion counterpart of the H-Maxima transform.

The filter performs **reconstruction by erosion** of the raised marker `input + Height`, with the original input as its lower mask. Erosion lowers marker values through connected neighbors, but cannot lower them below the input mask. Output contains these reconstructed intensities. For example, an isolated minimum *30* in background *50* becomes *40* at Height *10*. It does not become a contrast value of *10*. A one-pixel image has no neighbor to lower its marker, so its output is the raised marker.

The input array must have one component per tuple (a **scalar image**). The output has the same type and dimensions as the input.

### Height

**Height** is in the intensity units of the input. It must be finite and not negative.

- An integer image needs a whole-number Height. A fractional value, such as *0.5* or *2.5*, fails preflight. To use a fractional Height, first convert the image to a floating-point type.
- A floating-point image accepts a fractional Height.
- Height *0* gives an output equal to the input.
- For an integer image, `input + Height` stops at the maximum of the input type and does not wrap around.
- Height is a float64 value, so it cannot represent every whole number above *2^53*.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for minima that are one voxel wide.

Geodesic morphology and grayscale reconstruction are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter raises the input by **Height** and reconstructs that marker under the input by erosion.

Arrays in memory and out-of-core arrays give the same result. The filter reads and writes out-of-core arrays in blocks, so it does not need a second complete image in memory.

If the filter stops with an error or is cancelled, the output array can hold partial results.

% Auto generated parameter table will be inserted here

## Example Pipelines

The **Powder Particle Watershed Segmentation** bookmark suppresses shallow minima before it creates watershed markers. [Example pipeline guide](https://github.com/BlueQuartzSoftware/simplnx/blob/develop/src/Plugins/NXImageProcessing/pipelines/08_Powder_Particle_Watershed_Segmentation/08_Powder_Particle_Watershed_Segmentation.md)

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
