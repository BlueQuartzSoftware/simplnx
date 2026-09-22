# Label Contour Image Filter

Labels the voxels on the border (contour) of the objects in a labeled image.

## Group (Subgroup)

ImageProcessing (Contour)

## Description

Extract the contours (object borders) of a labeled image. Every value other than the **Background Value** is treated as its own object/region (there is no single foreground value). A voxel keeps its own label in the output **only** when at least one of its connectivity neighbors has a **different** value — a different label, or the background; a voxel in the interior of a region (all neighbors the same label) is set to the **Background Value**. A background-valued voxel is **never** on a contour: it always stays the Background Value, even when it sits next to a labeled region.

The labels are preserved exactly between input and output on the contour, so the output is the set of object borders drawn with each object's own label, on a background field. This matches the legacy ITK Label Contour Image Filter exactly.

The input array must be single-component (scalar) and of an integer type. The output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation. It uses bounded local buffers and produces the same result whether the data is in memory or out-of-core.

### Border rule

The filter has **no boundary condition**: a voxel on the edge of the image is compared only against its neighbors that actually exist inside the image. An edge voxel is therefore a contour only when a real in-image neighbor differs from it, and a region that fills the entire image produces no contour. This matches the legacy ITK filter.

### Fully Connected

The *Fully Connected* parameter selects the neighbor connectivity used to decide the contour:

- **Off (default):** face connectivity (6-connected in 3D, 4-connected in 2D) — the axis-aligned neighbors only.
- **On:** face + edge + vertex connectivity (26-connected in 3D, 8-connected in 2D). Full connectivity produces thicker contours; for objects that are one voxel wide, use Fully Connected on.

## Algorithm

For a 3D image with an out-of-core input or output, the filter first asks the shared cache budget for enough temporary working memory to hold the complete input and output. A complete grant lets the filter read the input once, evaluate the radius-one neighborhoods from local memory, and write the output once. A partial grant or allocation failure uses the bounded fallback, which bulk-reads each output Z-plane with its adjacent-plane halo and bulk-writes the completed output plane.

For a single-slice image, a checked 64 MiB plan may retain a fitting disk-backed output plane (up to 48 MiB), stream the input through the remaining allowance, and issue one final bulk write. If that route does not fit, the plan uses a fixed input allowance or bounded full-width row blocks; exceptionally wide rows use one-row X tiles with clipped X/Y halos. Datastore access remains serial and outside the parallel voxel loop.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
