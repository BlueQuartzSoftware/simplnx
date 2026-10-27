# H Minima Image Filter

Suppress local minima whose depth (contrast) is below a user-supplied value.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Applies the **H-Minima transform** to a grayscale image. A **regional minimum** is a connected region below its surrounding intensities. Its **depth** is the intensity difference to that surrounding level. The transform suppresses minima whose depth is less than **Height**. Deeper minima rise by Height, subject to the input type's maximum.

The H-Minima transform is commonly used to remove insignificant intensity valleys caused by noise prior to a minima-detection or watershed step, so that only sufficiently prominent minima remain. It is the erosion counterpart of the H-Maxima transform.

The filter performs **reconstruction by erosion** of the raised marker `input + Height`, with the original input as its lower mask. Erosion lowers marker values through connected neighbors, but cannot lower them below the input mask. Output contains these reconstructed intensities. For example, an isolated minimum *30* in background *50* becomes *40* at Height *10*. It does not become a contrast value of *10*. A one-pixel image has no neighbor to lower its marker, so its output is the raised marker.

The input must have one component per tuple, called a **scalar image**. Output type, tuple dimensions and component dimensions match input. Output storage does not increase automatically. Resident and disk-backed arrays use the same numeric rules.

### Height

**Height** is a contrast offset in the input's intensity units. It must be finite and nonnegative.

- Integer images require a whole-number Height. Fractions such as *0.5* or *2.5* fail preflight. Choose an intentional whole Height, or explicitly convert the input to floating point before using a fraction.
- Floating images allow fractional Height. Their marker uses floating arithmetic and rounds to the input type's precision.
- Integer markers use exact native addition and saturate at the input type's maximum. Native 64-bit input values do not pass through floating point. Height *0* preserves every integer input value exactly, including native bounds.
- Height is stored as a float64 parameter. Above *2^53*, float64 cannot distinguish every consecutive integer Height. The filter uses the whole value represented by that parameter. A Height of *2^64* or more raises every integer marker value to the type maximum.

For existing integer pipelines with fractional Height, choose a whole Height or convert the input explicitly. Exact arithmetic can change results that previously lost native 64-bit units. Review downstream minima selection, watershed seeds and segmentation when migrating such a pipeline.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for minima that are one voxel wide.

Geodesic morphology and grayscale reconstruction are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter raises the input by Height and reconstructs that marker under the input by erosion. Resident input uses a materialized marker and keeps the Vincent hybrid reconstruction route. This route can allocate complete resident images for its marker and reconstruction work.

Disk-backed input derives marker values in owned reconstruction buffers. It does not require a separate complete resident marker image. Reconstruction keeps its existing working-memory reservation and sweep routes. A granted working set can hold a complete image when it fits. Otherwise, 2-D data uses rows or tiles, and 3-D data uses slabs with a boundary plane. A 3-D streaming plan needs space for two working planes and one boundary plane. Execution fails if the grant cannot hold that minimum. The working-buffer budget excludes data stores, their cache and other process memory.

Store transfers remain serial; parallel work uses stable owned buffers. Transfer errors, cancellation and existing allocation fallbacks retain their existing behavior. A later execution failure can leave completed output values or chunks written. An error does not promise an unchanged output array.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
