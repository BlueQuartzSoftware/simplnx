# H Maxima Image Filter

Suppress local maxima whose height (contrast) is below a user-supplied value.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Applies the **H-Maxima transform** to a grayscale image. The transform removes every *regional maximum* whose **height** -- its contrast above the surrounding level -- is less than the **Height** parameter, and lowers each surviving maximum by exactly **Height**. Shallow bright features (whose contrast is below **Height**) are flattened into their surrounding background, while taller bright features are preserved but reduced in intensity by **Height**.

The H-Maxima transform is commonly used to remove insignificant intensity peaks caused by noise prior to a maxima-detection or watershed step, so that only sufficiently prominent maxima remain.

Internally the filter performs a **reconstruction-by-dilation** of a marker image equal to the saturating difference `input - Height`, using the input image as the mask. Because the marker is formed with a *saturating* cast, a difference that underflows the element type's range is **clamped** to the type minimum (rather than wrapping), matching the legacy ITK behavior exactly.

The input array must be single-component (scalar); the output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK H Maxima Image Filter. When all data is resident it uses a fast in-core hybrid reconstruction; when the arrays are out-of-core it streams the computation so memory use stays bounded regardless of image size. Both paths produce bit-identical output, and the result matches the legacy ITK filter exactly.

### Height

The *Height* parameter sets the contrast threshold in the same units as the image intensities:

- Regional maxima with a contrast **less than** *Height* are removed (flattened to the surrounding level).
- Regional maxima with a contrast **greater than or equal to** *Height* survive but are lowered by exactly *Height*.

Geodesic morphology and grayscale reconstruction are described in Chapter 6 of Pierre Soille, *Morphological Image Analysis: Principles and Applications*, Second Edition, Springer, 2003.

## Algorithm

The filter lowers the input by **Height** and reconstructs that marker under the input by dilation. Resident arrays use the Vincent hybrid reconstruction, whose raster, anti-raster, and fold-to-convergence sweeps run as dependency-exact, block-wavefront parallel plane sweeps when the reconstruction uses face connectivity; with full connectivity it falls back to the exact serial sweeps, since a same-row diagonal read can land in the horizontally adjacent block, which no rectangular block schedule can order. Disk-backed reconstruction iterates a forward raster sweep followed by a reverse anti-raster sweep over consecutive Z-plane slabs until stable, skipping any slab whose planes are already proven unchanged since that direction's last visit. Within a plane, the raster/anti-raster sweep is tiled into independent blocks that run in parallel; under face connectivity every voxel still sees exactly the values the un-blocked sweep would produce, and under full connectivity the iterated forward/reverse passes still converge to the identical final result. Each work and mask slab targets 16 MiB, plus one boundary plane. At least one plane is processed, so a single plane wider than the target can make the buffers exceed 16 MiB. Update order and output are unchanged, preserving exact storage-path parity.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
