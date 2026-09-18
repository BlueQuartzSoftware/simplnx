# Grayscale Dilate Image Filter

Grayscale dilation of an image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Dilate an image using grayscale morphology. Dilation takes the **maximum** of all the input values covered by the flat structuring element (kernel) centered on each voxel. Neighbors that fall outside the image bounds are skipped, which is equivalent to substituting the type minimum for out-of-image voxels (ITK's extremum-fill boundary), so they never win the maximum.

The input array must be single-component (scalar), and the output element type matches the input type. Moderate kernels on multi-plane 8-bit images with at most 96 offsets use a flat-offset maximum fold. Other data types, true-2D images, and larger kernels use the applicable moving-histogram or checked-gather path. The out-of-core path uses bounded slabs and serial bulk store transfers.

### Kernel Type

The *Kernel Type* parameter selects the structuring element used for the morphological operation:

- **Annulus [0]**: A ring/shell-shaped structuring element (the outer Ball with an inner Ball removed).
- **Ball [1]**: A spherical/ellipsoidal structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element (only the axis-aligned neighbors).

**Note on Annulus:** This filter builds a *proper, non-empty* Annulus kernel (a thickness-1 shell, matching ITK's real `FlatStructuringElement::Annulus`). This corrects a bug in the legacy ITK/SimpleITK wrapper, in which the Annulus kernel collapsed to an **empty** structuring element (its `thickness` argument was bound to `false`/`0`), effectively passing the input through unchanged. Results with the Annulus kernel therefore differ from the legacy filter *by design*.

**Note on a zero radius component (e.g. `{2, 1, 0}`) with a Box kernel on a 3D image:** This filter computes the correct grayscale dilation (a radius of 0 on an axis simply means the kernel has extent 1 on that axis). The legacy ITK filter's decomposable-Box (van Herk–Gil–Werman anchor) path produces an incorrect result for a zero-radius axis on a 3D image, reaching values outside the intended neighborhood. Results therefore differ from the legacy filter *by design* for this case; a radius of 0 on the Z axis of a genuinely 2D (single-slice) image is unaffected.

## Algorithm

### In-Core Path

The resident `MorphDirect` path uses the flat-offset maximum fold for moderate kernels on multi-plane 8-bit images. The fold processes interior X rows with SIMD maximum operations and uses a checked scalar fold at the borders. Other cases use the applicable moving-histogram path.

### Out-of-Core Path

The `MorphScanline` out-of-core path bulk-reads bounded slabs with a shared Z halo. Moderate kernels on multi-plane 8-bit images use the same flat-offset maximum fold. The fold processes interior X rows with SIMD maximum operations and uses a checked scalar fold at the borders. Other cases use the applicable moving-histogram or checked-gather path. The path writes completed planes with serial bulk store transfers. The 3D slab normally targets about 16 MiB, but it keeps at least one output plane and its complete Z halo. A wide plane can exceed the target. A Z radius at least as large as the image depth can require a full-depth input slab.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
