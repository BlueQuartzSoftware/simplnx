# Erode Object Morphology Image Filter

Object erosion of a single labeled object in an image.

## Group (Subgroup)

ImageProcessing (Morphology)

## Description

Erode a single object in an image using object morphology. A voxel equal to the **Object Value** is part of the object; every other value is background. Object morphology touches only the **border** of the object: a voxel is a *boundary* voxel when it equals the Object Value **and** at least one of its immediate neighbors in the fixed radius-1 full box (26-connected in 3D, 8-connected in 2D) is not the Object Value. Each boundary voxel then paints the structuring element (kernel) centered on it with the **Background Value**, carving a shell off the border of the object (shrinking it inward by one kernel).

Boundary and object state are read from the **input**, and the painted values are written to a **separate output** that begins as a copy of the input. Because of this, the erosion does not cascade: a voxel painted during this pass cannot itself trigger further painting in the same pass. Interior object voxels (whose immediate neighbors are all object) and background voxels far from the object border pass through unchanged. This differs from grayscale/binary morphology, which take a min/max over the structuring element at *every* voxel.

The input array must be single-component (scalar); the output element type matches the input element type, and all 10 scalar numeric types (integer and floating point) are supported. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Erode Object Morphology Image Filter. It uses a deterministic single-pass scatter when the data is resident or a complete shared working-memory request is granted, and otherwise uses bounded local buffers and bulk transfers. Both paths produce bit-identical output. (The legacy ITK filter rejected out-of-core arrays.)

### Object Value & Background Value

The **Object Value** identifies the object to erode. The boundary test uses **exact equality** against this value. The **Background Value** is the value written over the border of the object where erosion carves it away (it may be any value, including one already present elsewhere in the image; the boundary test always treats any value not equal to the Object Value as non-object). For an integer input image both values must be finite and within the image type's representable range (a fractional value is truncated toward zero to match the input type, with a warning). For a floating-point input image both values must be finite; use values that are exactly representable in the input type (e.g. small integers such as `1.0` or `0.0`) so the equality test behaves as intended.

### Kernel Type

The *Kernel Type* parameter selects the structuring element painted around each boundary voxel:

- **Annulus [0]**: A ring/shell-shaped structuring element (the outer Ball with an inner Ball removed).
- **Ball [1]**: A spherical/ellipsoidal structuring element (default). Most commonly used for general morphological operations.
- **Box [2]**: A rectangular/cuboid structuring element.
- **Cross [3]**: A cross-shaped structuring element (only the axis-aligned neighbors).

**Note on Annulus:** This filter builds a *proper, non-empty* Annulus kernel (a thickness-1 shell, matching ITK's real `FlatStructuringElement::Annulus`). This corrects a bug in the legacy ITK/SimpleITK wrapper, in which the Annulus kernel collapsed to an **empty** structuring element (its `thickness` argument was bound to `false`/`0`), effectively passing the input through unchanged. Results with the Annulus kernel therefore differ from the legacy filter *by design*.

**Note on determinism vs. the legacy ITK filter:** ITK's `ObjectMorphologyImageFilter` copies the input into the output per thread region and then paints boundary neighborhoods into the shared output without synchronization. A paint that crosses into a neighboring thread's region can be overwritten by that thread's non-atomic copy, making both legacy erosion and dilation nondeterministic. This filter implements the intended deterministic semantics: initialize the output from the input, then paint the **Background Value** over each boundary object's structuring element. It is validated against an independent oracle rather than the racy legacy result.

## Algorithm

### In-Core Path

When the input and output **Data Arrays** are resident, the filter uses a scatter algorithm. It reads the complete input, initializes the output from that input, scans boundary object voxels in raster order, and paints each voxel's structuring element into the output with the **Background Value**. Boundary decisions always use the original input, so painted voxels never trigger additional painting. The scatter is single-threaded because neighboring boundary voxels may paint overlapping output locations.

### Out-of-Core Path

For a true 3D image with an out-of-core input or output, the filter first asks the shared cache budget for enough temporary working memory to hold the complete input and output. A complete grant reuses the deterministic scatter with one full input read and one full output write. A partial grant or allocation failure instead inverts the scatter into an equivalent bounded gather so writes remain sequential. That fallback keeps a rolling input window covering the current output plane's kernel halo plus the extra planes needed by the fixed radius-1 boundary test. Retained planes shift forward as Z advances, and only newly entering planes are read with `copyIntoBuffer`; each input plane is read once. A second rolling window stores the boundary mask, computing each mask plane once. Worker threads read only these immutable local buffers, write disjoint output-plane slots, and the completed plane is written once with `copyFromBuffer`.

For a true 2D image, a checked planner keeps the combined input halo, boundary mask, and output below a 64 MiB target. Normal-width images use full-width row blocks; exceptionally wide images use X tiles with halos. Both routes preserve the same boundary classification, kernel-offset order, exact-value comparisons, and non-cascading output as the in-core path.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
