# Morphological Gradient Image Filter

Grayscale morphological gradient of an image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Compute the grayscale morphological gradient of an image. The morphological gradient is the grayscale **dilation minus the grayscale erosion** through the same flat structuring element (kernel) centered on each voxel, giving the local intensity range (a measure of edge strength). Neighbors that fall outside the image bounds are skipped, which is equivalent to ITK's extremum-fill boundary, so the dilation and erosion each ignore out-of-image voxels.

The input array must be single-component (scalar), and the output element type matches the input type. Moderate kernels on multi-plane 8-bit images with at most 96 offsets use a flat-offset fold that tracks the minimum and maximum, then writes their difference. Other data types, true-2D images, and larger kernels use the applicable moving-histogram or checked-gather path. The out-of-core path uses bounded slabs or tiles and serial bulk store transfers.

### Kernel Type

The *Kernel Type* parameter selects the structuring element used for the morphological operation:

- **Annulus [0]**: A ring/shell-shaped structuring element (the outer Ball with an inner Ball removed).
- **Ball [1]**: A spherical/ellipsoidal structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element (only the axis-aligned neighbors).

**Note on Annulus:** This filter builds a *proper, non-empty* Annulus kernel (a thickness-1 shell, matching ITK's real `FlatStructuringElement::Annulus`). This corrects a bug in the legacy ITK/SimpleITK wrapper, in which the Annulus kernel collapsed to an **empty** structuring element (its `thickness` argument was bound to `false`/`0`), effectively passing the input through unchanged. Results with the Annulus kernel therefore differ from the legacy filter *by design*.

**Note on a zero radius component (e.g. `{2, 1, 0}`) with a Box kernel on a 3D image:** This filter computes the correct grayscale morphology (a radius of 0 on an axis simply means the kernel has extent 1 on that axis). The legacy ITK filter's decomposable-Box (van Herk–Gil–Werman anchor) path produces an incorrect result for a zero-radius axis on a 3D image, reaching values outside the intended neighborhood. Results therefore differ from the legacy filter *by design* for this case; a radius of 0 on the Z axis of a genuinely 2D (single-slice) image is unaffected.

## Algorithm

### In-Core Path

The resident `MorphGradientDirect` path uses separate flat-offset maximum and minimum folds for moderate kernels on multi-plane 8-bit images. The folds process interior X rows with SIMD extrema operations before subtraction and use a checked scalar fold at the borders. Other cases use the applicable moving-histogram path.

### Out-of-Core Path

The `MorphGradientScanline` out-of-core path keeps store reads and writes serial while local buffers run in parallel. For 3D images it bulk-reads bounded slabs with a shared Z halo. Moderate kernels on multi-plane 8-bit images use separate flat-offset maximum and minimum folds. The folds process interior X rows with SIMD extrema operations before subtraction and use a checked scalar fold at the borders. Other cases use the fused moving-histogram path. The 3D slab normally targets about 16 MiB, but it keeps at least one output plane and its complete Z halo. A wide plane can exceed the target. A Z radius at least as large as the image depth can require a full-depth input slab.

For a single-slice image, the path uses a checked 64 MiB aggregate target. A fitting out-of-core input may use a fixed 48 MiB input buffer and at most 16 MiB of output staging; otherwise the image is processed as full-width row blocks or, for exceptionally wide rows, X tiles with their clipped X/Y halos. Empty structuring elements emit zero through the same bounded-write policy. No datastore access occurs inside the parallel voxel loop.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
