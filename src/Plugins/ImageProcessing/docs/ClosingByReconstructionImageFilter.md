# Closing By Reconstruction Image Filter

Grayscale closing by reconstruction: remove dark structures smaller than the structuring element while preserving the exact contours of those that survive.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Performs a grayscale **closing by reconstruction**: a morphological **dilation** with the chosen structuring element, followed by a geodesic **reconstruction-by-erosion** of the dilated image using the original image as the mask. It is the exact dual of the Opening By Reconstruction Image Filter.

Like a plain grayscale closing, it removes dark holes and structures that are smaller than the structuring element. Unlike a plain closing (dilate-then-erode), the reconstruction step regrows the surviving structures back to their **original contours** rather than the rounded/expanded shapes a plain erosion would produce.

When **Preserve Intensities** is enabled, a second reconstruction re-seeds the original input intensities at the surviving structures (rather than the dilated intensities), so the output retains the input's exact gray levels there.

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Closing By Reconstruction Image Filter, and matches it exactly for integer and float types.

### Kernel Type / Kernel Radius

The structuring element shape (Annulus, Ball, Box, or Cross) and its per-axis radius, used by the dilation.

### Fully Connected

Selects the connectivity of the reconstruction: **Off** (default) uses face connectivity only; **On** uses face + edge + vertex connectivity.

### Preserve Intensities

**Off** (default): the surviving structures keep their dilated intensities. **On**: they are re-valued to their original input intensities.

Geodesic morphology and opening/closing by reconstruction are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter dilates the input and reconstructs the dilated marker under the input by erosion, optionally performing a second reconstruction to restore preserved intensities. Resident arrays use the Vincent hybrid reconstruction, whose raster, anti-raster, and fold-to-convergence sweeps run as dependency-exact, block-wavefront parallel plane sweeps when the reconstruction uses face connectivity (**Fully Connected** off); with full connectivity it falls back to the exact serial sweeps, since a same-row diagonal read can land in the horizontally adjacent block, which no rectangular block schedule can order. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
