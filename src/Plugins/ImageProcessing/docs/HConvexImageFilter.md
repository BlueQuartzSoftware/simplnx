# H Convex Image Filter

Extract the h-convex regional maxima (the "domes") of a grayscale image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Computes the *h-convex* transform of a grayscale image: **HConvex(f, h) = f − HMaxima(f, h)**. Each regional maximum of the input is replaced by its own height (its contrast to the surrounding level), **capped at the Height parameter**, while everything that is not part of a maximum becomes zero. A regional maximum of contrast `c` therefore maps to `min(c, h)`.

This isolates bright "domes" and measures their prominence: peaks taller than `Height` all map to `Height`, while shallower peaks map to their true contrast. It is a common pre-step for peak detection and marker extraction.

Internally the filter reconstructs the input by dilation of the `input − Height` marker (the H Maxima result) and then subtracts that result from the input. Because the reconstruction result never exceeds the input, the difference is always non-negative and stays within the input type's range.

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK H Convex Image Filter, and matches it exactly for integer and float types.

### Height

The height (contrast) cap. Regional maxima taller than this value are reported at this value; shallower maxima are reported at their true height.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for maxima that are one voxel wide.

Geodesic morphology, the h-maxima transform, and the h-convex transform are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter lowers the input by **Height**, reconstructs that marker under the input by dilation, and subtracts the reconstruction from the input. Resident arrays use the Vincent hybrid reconstruction, whose raster, anti-raster, and fold-to-convergence sweeps run as dependency-exact, block-wavefront parallel plane sweeps when the reconstruction uses face connectivity (**Fully Connected** off); with full connectivity it falls back to the exact serial sweeps, since a same-row diagonal read can land in the horizontally adjacent block, which no rectangular block schedule can order. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
