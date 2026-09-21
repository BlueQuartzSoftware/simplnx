# Valued Regional Minima Image Filter

Keep the regional minima of a grayscale image at their value; set everything else to the type maximum.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Transforms the image so that any pixel that is **not** a regional minimum is set to the maximum value of the pixel type, while pixels that **are** regional minima retain their original value. It is the exact dual of the Valued Regional Maxima Image Filter.

A regional minimum is a connected flat zone surrounded entirely by pixels of strictly higher value (a local "pit" plateau). This filter isolates those plateaus at their true intensity and suppresses everything else, which is useful for seeded segmentation (e.g. watershed markers). A completely flat image is left unchanged (the whole image is treated as a single regional minimum).

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Valued Regional Minima Image Filter, and matches it exactly for integer and float types. When all data is resident it uses a fast in-core flood; when the arrays are out-of-core it streams the computation so memory use stays bounded regardless of image size. Both paths produce bit-identical output.

### Fully Connected

The *Fully Connected* parameter selects the connectivity used to define the flat zones:

- **Off (default)**: face connectivity only (the axis-aligned neighbors).
- **On**: face + edge + vertex connectivity.

Regional extrema are described in Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003, and in the Insight Journal article "Finding regional extrema - methods and performance" (Beare & Lehmann, 2005).

## Algorithm

For resident arrays, the filter uses a stack flood over equal-valued flat zones. For disk-backed arrays, it first marks every voxel with a strictly lower neighbor, then propagates the type-maximum marker through equal-valued zones until stable. Propagation alternates a forward raster sweep and a reverse anti-raster sweep. Each sweep reads consecutive Z-plane slabs with one halo plane on each side -- skipping any slab whose planes are already proven unchanged since that direction's last visit -- processes voxels sequentially in scan order, and writes only the core planes with contiguous bulk I/O. Each input and output buffer targets 16 MiB including the halos; at least one core plane is always processed, so an unusually wide plane plus its required halos can exceed that target. The two paths converge to the same valued regional-minima result.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
