# Binary Dilate Image Filter

Binary dilation of the foreground of an image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Dilate the foreground of a binary image. A voxel is set to the **Foreground Value** in the output when **any** neighbor covered by the flat structuring element (kernel) centered on it is foreground, and to the **Background Value** otherwise. The output therefore contains only the two values `{Foreground Value, Background Value}`.

**Binary-input requirement:** this filter operates on a strictly binary image. Every input voxel must equal either the Foreground Value or the Background Value; the filter **errors at execution** if the input contains any other value. To process a segmented/label image, first threshold or relabel it so the label of interest is the Foreground Value and everything else is the Background Value.

The input array must be single-component (scalar) and have an integer type. Moderate kernels on multi-plane 8-bit images with at most 96 offsets use portable 64-bit packed membership words. The fold uses the configured boundary rule. Other cases use the applicable moving-window or checked-gather path.

### Boundary To Foreground

The *Boundary To Foreground* parameter controls how neighbors that fall outside the image bounds are treated: when enabled, an out-of-image neighbor counts as foreground; when disabled (the default for dilation), it counts as background.

### Kernel Type

The *Kernel Type* parameter selects the structuring element used for the morphological operation:

- **Annulus [0]**: A ring/shell-shaped structuring element (the outer Ball with an inner Ball removed).
- **Ball [1]**: A spherical/ellipsoidal structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element (only the axis-aligned neighbors).

**Note on Annulus:** This filter builds a *proper, non-empty* Annulus kernel (a thickness-1 shell, matching ITK's real `FlatStructuringElement::Annulus`). This corrects a bug in the legacy ITK/SimpleITK wrapper, in which the Annulus kernel collapsed to an **empty** structuring element (its `thickness` argument was bound to `false`/`0`), effectively passing the input through unchanged. Results with the Annulus kernel therefore differ from the legacy filter *by design*.

## Algorithm

### In-Core Path

The resident `BinaryMorphDirect` path packs equality membership into portable 64-bit X-row words for moderate kernels on multi-plane 8-bit images. It folds each structuring-element offset with word OR and emits exact foreground or background values. It releases the raw input after packing. Its scratch use stays below the former two-raw-volume peak. Other cases use the applicable moving foreground-count path.

### Out-of-Core Path

The `BinaryMorphScanline` out-of-core path bulk-reads bounded slabs with a shared Z halo. Moderate kernels on multi-plane 8-bit images pack each staged slab once into O(slab/8) words. They fold local words with OR and use the configured boundary rule. Other cases use the applicable moving-window or checked-gather path. Reads and writes use serial bulk DataStore transfers. Completed planes are written in batches. The 3D slab normally targets about 16 MiB, but it keeps at least one output plane and its complete Z halo. A wide plane can exceed the target. A Z radius at least as large as the image depth can require a full-depth input slab.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
