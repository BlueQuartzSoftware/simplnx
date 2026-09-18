# Grayscale Grind Peak Image Filter

Remove local maxima (bright peaks) not connected to the boundary of the image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Grinds *peaks* in a grayscale image. A peak is a connected set of interior voxels that are brighter than the surrounding level -- a **regional maximum** in the grayscale topography that is **not connected to the boundary** of the image. Gray-level values adjacent to a peak are extrapolated across the peak, grinding it down to the level of the brightest surrounding path to the border. Local **minima** are left untouched. This is the exact dual of the Grayscale Fillhole Image Filter.

This filter is useful for smoothing over local maxima without affecting dark features. If you take the difference between the original image and this filter's output (optionally thresholding above a small value), you obtain a map of the ground-down peaks.

Internally the filter performs a **reconstruction-by-dilation** of a marker image, using the input image as the mask. The marker's border voxels match the border of the input image and its interior voxels are set to the input's global minimum; reconstruction-by-dilation then raises the marker back up to the input everywhere except inside bright interior regions not connected to the border, which are ground down. A bright region that **touches** the image border is *not* a peak and is left unchanged.

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Grayscale Grind Peak Image Filter. When all data is resident it uses a fast in-core hybrid reconstruction; when the arrays are out-of-core it streams the computation so memory use stays bounded regardless of image size. Both paths produce bit-identical output, and the result matches the legacy ITK filter exactly.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for peaks that are one voxel wide.

Geodesic morphology and the grind-peak algorithm are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter constructs a border marker and reconstructs it under the input by dilation. Resident arrays use the Vincent hybrid reconstruction; because its marker comes from the image border rather than a provided or height-derived marker, the resident sweeps always run as the exact serial raster/anti-raster passes, since a border marker overflows the bounded frontier into fold-to-convergence sweeps where per-plane parallel-dispatch overhead outweighs the gain. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
