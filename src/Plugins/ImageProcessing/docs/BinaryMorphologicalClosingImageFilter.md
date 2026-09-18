# Binary Morphological Closing Image Filter

Binary morphological closing of an image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Close a binary image using mathematical morphology. Closing is a binary **dilation followed by a binary erosion** through the same flat structuring element (kernel) centered on each voxel. It fills background holes and thin gaps smaller than the structuring element while preserving the overall shape of larger structures. A voxel is foreground only when it equals the **Foreground Value**; every other value is background. The output contains only the two values `{Foreground Value, internal background}`. Boundary handling near the image edge is controlled by the *Safe Border* parameter (see below).

Because closing is *extensive* (it only fills background, never adds it), it exposes no Background Value: like the legacy ITK filter it uses an internal background derived from the Foreground Value (0, or the image type's maximum when the Foreground Value is 0 so the two values stay distinct).

**Binary-input requirement:** this filter operates on a strictly binary image. Every input voxel must equal either the Foreground Value or that internal background; the filter **errors at execution** if the input contains any other value. To process a segmented/label image, first threshold or relabel it so the label of interest is the Foreground Value and everything else is 0 (or the type maximum when the Foreground Value is 0).

The input array must be single-component (scalar) and have an integer type. Moderate kernels on multi-plane 8-bit images with at most 96 offsets use portable 64-bit packed membership words in resident passes. Each fold uses its configured boundary rule. Other cases use the applicable moving-window or checked-gather path.

### Kernel Type

The *Kernel Type* parameter selects the structuring element used for the morphological operation:

- **Annulus [0]**: A ring/shell-shaped structuring element (the outer Ball with an inner Ball removed).
- **Ball [1]**: A spherical/ellipsoidal structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element (only the axis-aligned neighbors).

**Note on Annulus:** This filter builds a *proper, non-empty* Annulus kernel (a thickness-1 shell, matching ITK's real `FlatStructuringElement::Annulus`). This corrects a bug in the legacy ITK/SimpleITK wrapper, in which the Annulus kernel collapsed to an **empty** structuring element (its `thickness` argument was bound to `false`/`0`), effectively passing the input through unchanged. Results with the Annulus kernel therefore differ from the legacy filter *by design*.

**Note on a zero radius component (e.g. `{2, 1, 0}`) with a Box kernel on a 3D image:** This filter computes the correct binary morphology (a radius of 0 on an axis simply means the kernel has extent 1 on that axis). The legacy ITK filter's decomposable-Box path produces an incorrect result for a zero-radius axis on a 3D image. Results therefore differ from the legacy filter *by design* for this case; a radius of 0 on the Z axis of a genuinely 2D (single-slice) image is unaffected.

### Safe Border

When *Safe Border* is enabled (the default), the image is padded by the kernel radius before the dilation/erosion passes and cropped back afterward, matching the legacy ITK filter and avoiding boundary artifacts near the image edge. When disabled, the two passes run directly on the image; results are identical in the image interior and differ only within one kernel radius of the border.

## Algorithm

Closing applies binary dilation followed by erosion through the same structuring element.

### In-Core Path

Each resident `BinaryMorphDirect` pass inherits the packed 64-bit word route for moderate kernels on multi-plane 8-bit images. Other cases use the applicable moving foreground-count path.

### Out-of-Core Path

The out-of-core composite path uses Z-rings bounded by the kernel radius and serial, batched bulk store transfers. For moderate multi-plane 8-bit kernels with at most 96 offsets, persistent circular rings store one-bit membership planes. Each appended input plane is packed once. The first stage folds OR/AND words into a packed output plane. The second stage folds packed words and writes only the final raw foreground/background plane. Circular slots avoid ring memory moves. No full intermediate array is materialized.

Other cases retain the raw-ring fallback. If the ring reservation is unavailable, `BinaryMorphScanline` uses bounded halo slabs. Its moderate multi-plane 8-bit route can use packed slabs. Other cases use the applicable fallback route. The 3D slab normally targets about 16 MiB, but it keeps at least one output plane and its complete Z halo. A wide plane can exceed the target. A Z radius at least as large as the image depth can require a full-depth input slab.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
