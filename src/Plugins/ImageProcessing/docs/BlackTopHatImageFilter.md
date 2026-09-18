# Black Top Hat Image Filter

Grayscale black top-hat transform of an image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Compute the black top-hat transform of an image using grayscale morphology. The black top-hat is the **grayscale morphological closing minus the input** (a closing is a dilation followed by an erosion through the same flat structuring element). It isolates the dark features (valleys/holes) smaller than the structuring element — those the closing filled. Boundary handling of the internal closing near the image edge is controlled by the *Safe Border* parameter (see below). Because the closing never falls below the input, the difference is always non-negative.

The input array must be single-component (scalar), and the output element type matches the input type. Moderate kernels on multi-plane 8-bit images with at most 96 offsets use flat-offset folds. Other data types, true-2D images, and larger kernels use the applicable moving-histogram or checked-gather path.

### Kernel Type

The *Kernel Type* parameter selects the structuring element used for the morphological operation:

- **Annulus [0]**: A ring/shell-shaped structuring element (the outer Ball with an inner Ball removed).
- **Ball [1]**: A spherical/ellipsoidal structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element (only the axis-aligned neighbors).

**Note on Annulus:** This filter builds a *proper, non-empty* Annulus kernel (a thickness-1 shell, matching ITK's real `FlatStructuringElement::Annulus`). This corrects a bug in the legacy ITK/SimpleITK wrapper, in which the Annulus kernel collapsed to an **empty** structuring element (its `thickness` argument was bound to `false`/`0`), effectively passing the input through unchanged. Results with the Annulus kernel therefore differ from the legacy filter *by design*.

**Note on a zero radius component (e.g. `{2, 1, 0}`) with a Box kernel on a 3D image:** This filter computes the correct grayscale morphology (a radius of 0 on an axis simply means the kernel has extent 1 on that axis). The legacy ITK filter's decomposable-Box (van Herk–Gil–Werman anchor) path produces an incorrect result for a zero-radius axis on a 3D image, reaching values outside the intended neighborhood. Results therefore differ from the legacy filter *by design* for this case; a radius of 0 on the Z axis of a genuinely 2D (single-slice) image is unaffected.

### Safe Border

When *Safe Border* is enabled (the default), the internal closing pads the image by the kernel radius before its dilation/erosion passes and crops back afterward, matching the legacy ITK filter and avoiding boundary artifacts near the image edge. When disabled, the closing runs directly on the image with out-of-bounds neighbors skipped (equivalent to ITK's `SafeBorder = false`); results are identical in the image interior and differ only within one kernel radius of the border.

## Algorithm

The filter performs grayscale closing and subtracts the original image from that result.

### In-Core Path

The resident dilation and erosion passes use `MorphDirect`. Moderate kernels on multi-plane 8-bit images use flat-offset folds that process interior X rows with SIMD extrema operations and use a checked scalar fold at the borders. Other cases use the applicable moving-histogram path.

### Out-of-Core Path

The out-of-core composite path uses Z-rings bounded by the kernel radius and serial bulk store transfers. Moderate kernels on multi-plane 8-bit images use flat-offset folds in both ring stages. Each ring stage processes interior X rows with SIMD extrema operations and uses a checked scalar fold at the borders. Other cases use the applicable moving-histogram path. The rings avoid a full intermediate array. If the ring reservation is unavailable, bounded halo slabs use the applicable fallback route. The 3D slab normally targets about 16 MiB, but it keeps at least one output plane and its complete Z halo. A wide plane can exceed the target. A Z radius at least as large as the image depth can require a full-depth input slab. With *Safe Border* enabled, the final crop and closing-minus-input subtraction are fused into the plane write. The subtraction processes independent output rows in parallel and does not access the data store from worker threads.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
