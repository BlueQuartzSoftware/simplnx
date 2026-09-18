# H Minima Image Filter

Suppress local minima whose depth (contrast) is below a user-supplied value.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Applies the **H-Minima transform** to a grayscale image. The transform removes every *regional minimum* whose **depth** -- its contrast below the surrounding level -- is less than the **Height** parameter, and raises each surviving minimum by exactly **Height**. Shallow dark features (whose contrast is below **Height**) are filled up to their surrounding background, while deeper dark features are preserved but raised in intensity by **Height**.

The H-Minima transform is commonly used to remove insignificant intensity valleys caused by noise prior to a minima-detection or watershed step, so that only sufficiently prominent minima remain. It is the erosion counterpart of the H-Maxima transform.

Internally the filter performs a **reconstruction-by-erosion** of a marker image equal to the saturating sum `input + Height`, using the input image as the mask. Because the marker is formed with a *saturating* cast, a sum that overflows the element type's range is **clamped** to the type maximum (rather than wrapping), matching the legacy ITK behavior exactly.

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK H Minima Image Filter. When all data is resident it uses a fast in-core hybrid reconstruction; when the arrays are out-of-core it streams the computation so memory use stays bounded regardless of image size. Both paths produce bit-identical output, and the result matches the legacy ITK filter exactly.

### Height

The *Height* parameter sets the contrast threshold in the same units as the image intensities:

- Regional minima with a contrast **less than** *Height* are removed (filled up to the surrounding level).
- Regional minima with a contrast **greater than or equal to** *Height* survive but are raised by exactly *Height*.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for minima that are one voxel wide.

Geodesic morphology and grayscale reconstruction are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter raises the input by **Height** and reconstructs that marker under the input by erosion. Resident arrays use the Vincent hybrid reconstruction, whose raster, anti-raster, and fold-to-convergence sweeps run as dependency-exact, block-wavefront parallel plane sweeps when the reconstruction uses face connectivity (**Fully Connected** off); with full connectivity it falls back to the exact serial sweeps, since a same-row diagonal read can land in the horizontally adjacent block, which no rectangular block schedule can order. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
