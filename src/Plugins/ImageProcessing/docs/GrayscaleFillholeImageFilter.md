# Grayscale Fillhole Image Filter

Remove local minima (dark holes) not connected to the boundary of the image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Fills *holes* in a grayscale image. A hole is a connected set of interior voxels that are darker than the surrounding level -- a **regional minimum** in the grayscale topography that is **not connected to the boundary** of the image. Gray-level values adjacent to a hole are extrapolated across the hole, raising it to the level of the darkest surrounding path to the border. Local **maxima** are left untouched.

This filter is useful for smoothing over local minima without affecting bright features. If you take the difference between this filter's output and the original image (optionally thresholding above a small value), you obtain a map of the filled holes.

Internally the filter performs a **reconstruction-by-erosion** of a marker image, using the input image as the mask. The marker's border voxels match the border of the input image and its interior voxels are set to the input's global maximum; reconstruction-by-erosion then lowers the marker back down to the input everywhere except inside dark interior regions not connected to the border, which are filled. A dark region that **touches** the image border is *not* a hole and is left unchanged.

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Grayscale Fillhole Image Filter. When all data is resident it uses a fast in-core hybrid reconstruction; when the arrays are out-of-core it streams the computation so memory use stays bounded regardless of image size. Both paths produce bit-identical output, and the result matches the legacy ITK filter exactly.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define connected components:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity. Use this for holes that are one voxel wide.

Geodesic morphology and the fillhole algorithm are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter constructs a border marker and reconstructs it under the input by erosion. Resident arrays use the Vincent hybrid reconstruction; because its marker comes from the image border rather than a provided or height-derived marker, the resident sweeps always run as the exact serial raster/anti-raster passes, since a border marker overflows the bounded frontier into fold-to-convergence sweeps where per-plane parallel-dispatch overhead outweighs the gain. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
