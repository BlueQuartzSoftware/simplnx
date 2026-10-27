# H Convex Image Filter

Extract the h-convex regional maxima (the "domes") of a grayscale image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Computes the *h-convex* transform of a grayscale image: **HConvex(f, h) = f − HMaxima(f, h)**. Each regional maximum of the input is replaced by its own height (its contrast to the surrounding level), **capped at the Height parameter**, while everything that is not part of a maximum becomes zero. A regional maximum of contrast `c` therefore maps to `min(c, h)`.

This isolates bright "domes" and measures their prominence: peaks taller than `Height` all map to `Height`, while shallower peaks map to their true contrast. It is a common pre-step for peak detection and marker extraction.

Internally the filter reconstructs the `input − Height` marker by dilation under the original input, then subtracts that reconstruction from the input. The reconstruction cannot exceed an integer input, so the integer contrast is nonnegative. A nonnegative contrast can still exceed a signed output type's maximum. The filter returns an execution error in that case.

The input array must have one component per tuple (a **scalar image**). Output type, tuple dimensions and component dimensions match the input. Output storage does not increase automatically. Resident and disk-backed arrays use the same numeric rules.

### Height

**Height** is a contrast offset in the input's intensity units. It must be finite and nonnegative.

- Integer images require a whole-number Height. A fractional value, such as *2.5*, fails preflight. Choose an intentional whole value, or explicitly convert the image to a floating type before using a fraction.
- Floating images allow fractional Height and keep their existing floating arithmetic and precision.
- With integer images, the marker is the exact native value minus Height, saturated at the input type's minimum. Native 64-bit image values do not pass through floating point. Height *0* preserves the marker and reconstruction exactly; the HConvex output is therefore zero everywhere.
- Height is stored as a float64 parameter. Above *2^53*, float64 cannot distinguish every consecutive integer Height. The filter uses the whole value represented by that parameter. A Height of *2^64* or more lowers every integer marker value to the type minimum.

If the actual signed integer contrast does not fit the unchanged output type, execution fails with the input array and first failing tuple in the message. For example, int8 input `[-128, 127, -128]` with Height *255* would require contrast `[0, 255, 0]`; *255* cannot fit in int8, so the filter returns an error. Input `[-128, -1, -128]` at the same Height has representable contrast `[0, 127, 0]`.

For an existing integer pipeline that used fractional Height, choose a whole Height or convert its input explicitly. Exact marker arithmetic can also change results that previously lost native 64-bit units. Review downstream peak selection, watershed markers and segmentation when migrating such a pipeline.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for maxima that are one voxel wide.

Geodesic morphology, the h-maxima transform, and the h-convex transform are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter lowers the input by **Height**, reconstructs that marker under the input by dilation, and subtracts the reconstruction from the input. Resident arrays keep the Vincent reconstruction route and direct parallel subtraction. Signed subtraction checks each contrast before performing arithmetic that could overflow.

Disk-backed input supplies its marker from owned reconstruction buffers, so it does not require a separate resident marker image. Reconstruction keeps its existing working-memory reservation and sweep routes. A granted working set can hold a complete image when it fits. Otherwise, 2-D data uses rows or tiles, and 3-D data uses slabs with a boundary plane. A 3-D streaming plan needs enough space for two working planes and one boundary plane; it returns an error if the grant cannot hold that minimum. This is a working-buffer budget, not a limit on total process memory.

For disk-backed or other nonresident stores, the signed contrast phase transfers data serially through two owned buffers of at most 32 MiB each. It performs parallel arithmetic only on those buffers and propagates transfer errors and warnings. Resident signed contrast uses direct parallel spans without these transfer buffers. The buffer limits exclude the data stores, their cache and the reconstruction working set. If execution fails, some output values or completed chunks may already have been written; an error does not promise an unchanged output array.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
