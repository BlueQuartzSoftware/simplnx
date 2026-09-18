# Binary Opening By Reconstruction Image Filter

Binary opening by reconstruction: remove foreground objects smaller than the structuring element while preserving the exact shape of those that survive.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Performs a **binary opening by reconstruction**: a binary **erosion** with the chosen structuring element, followed by a binary **reconstruction-by-dilation** of the eroded image using the original image as the mask.

Like a plain binary opening, it removes foreground objects that are smaller than the structuring element. Unlike a plain opening (erode-then-dilate), the reconstruction step regrows each surviving object to its **exact original shape** (its full connected component), rather than the shrunken/rounded shape a plain dilation would produce. It is the standard tool for removing small objects from a binary image without distorting the ones that remain.

Only voxels equal to the **Foreground Value** are treated as foreground; each connected foreground component that survives the erosion is painted the foreground value, and everything else the background value.

The input array must be single-component (scalar), of an integer type, and strictly binary (containing only the foreground or background value). The output element type matches the input. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Binary Opening By Reconstruction Image Filter, and matches it exactly.

### Kernel Type / Kernel Radius

The structuring element shape (Annulus, Ball, Box, or Cross) and its per-axis radius, used by the erosion.

### Foreground Value / Background Value

The value identifying foreground objects, and the value written to everything that is not a surviving foreground object.

### Fully Connected

Selects the connectivity of the reconstruction: **Off** (default) uses face connectivity only; **On** uses face + edge + vertex connectivity.

Geodesic morphology and opening by reconstruction are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter erodes the binary foreground and reconstructs the surviving marker within the original foreground. Resident arrays use the Vincent hybrid reconstruction, whose raster, anti-raster, and fold-to-convergence sweeps run as dependency-exact, block-wavefront parallel plane sweeps when the reconstruction uses face connectivity (**Fully Connected** off); with full connectivity it falls back to the exact serial sweeps, since a same-row diagonal read can land in the horizontally adjacent block, which no rectangular block schedule can order. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
