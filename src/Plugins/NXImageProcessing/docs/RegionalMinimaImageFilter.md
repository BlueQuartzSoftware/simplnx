# Regional Minima Image Filter

Produce a binary image marking the regional minima of a grayscale image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Produces a binary image in which the **foreground** value marks the regional minima of the input and the **background** value marks everything else.

A regional minimum is a connected flat zone surrounded entirely by pixels of strictly higher value (a local "pit" plateau). This is the binary counterpart of the Valued Regional Minima Image Filter: instead of retaining the pit intensities, it labels the pits as foreground. It is commonly used to generate markers for a seeded watershed.

A completely flat input image has no regional structure; it is treated as a single regional minimum (all foreground) when **Flat Is Minima** is On, and as no minimum (all background) when Off.

The input array must be single-component (scalar). The output is a fixed **uint32** label image (matching the legacy ITK filter's fixed output type) into which the foreground/background values are written, regardless of the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Regional Minima Image Filter, and matches it exactly.

### Foreground Value / Background Value

The value written at regional-minimum pixels, and the value written everywhere else.

### Fully Connected

Selects the connectivity used to define the flat zones: **Off** (default) uses face connectivity only; **On** uses face + edge + vertex connectivity.

### Flat Is Minima

Whether a completely flat image is reported as a regional minimum (all foreground) or not (all background). Default On.

Regional extrema are described in Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003, and in the Insight Journal article "Finding regional extrema - methods and performance" (Beare & Lehmann, 2005).

## Algorithm

For resident arrays, the filter uses a stack flood over equal-valued flat zones. For disk-backed arrays, it first seeds every voxel with a strictly lower neighbor, then propagates those marks to a fixpoint with alternating forward-raster and reverse-anti-raster sweeps. Each disk-backed sweep reads consecutive Z-plane slabs with one halo plane on each side -- skipping any slab whose planes are already proven unchanged since that direction's last visit -- processes voxels sequentially in the same order as the former plane-at-a-time implementation, and writes only the core planes with contiguous bulk I/O. Each input and output buffer targets 16 MiB including the halos; at least one core plane is always processed, so an unusually wide plane plus its required halos can exceed that target. The valued result is converted to the fixed **uint32** foreground/background output in bounded chunks. Both storage paths produce identical results.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
