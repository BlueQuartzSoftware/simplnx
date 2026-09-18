# Binary Contour Image Filter

Labels the voxels on the border (contour) of the objects in a binary image.

## Group (Subgroup)

ImageProcessing (Contour)

## Description

Extract the contours (object borders) of a binary image. A voxel equal to the **Foreground Value** is kept as the Foreground Value in the output **only** when at least one of its connectivity neighbors is not foreground; a foreground voxel that is completely surrounded by foreground is interior and is set to the **Background Value**. Any voxel that is **not** the Foreground Value is passed through to the output **unchanged** (it keeps its own input value).

Because non-foreground voxels are passed through, on a strictly binary image (only the Foreground and Background values) the output contains just `{Foreground Value, Background Value}` — the object borders on a background field. On a labeled image, foreground objects are reduced to their borders while all other labels survive untouched. This own-value passthrough matches the legacy ITK Binary Contour Image Filter exactly, so — unlike the binary morphology filters — this filter does **not** require or enforce a strictly binary input.

The input array must be single-component (scalar) and of an integer type. The output element type matches the input element type. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Binary Contour Image Filter. It uses bounded local buffers and produces the same result whether the data is in memory or out-of-core.

### Border rule

The filter has **no boundary condition**: a voxel on the edge of the image is compared only against its neighbors that actually exist inside the image. An edge voxel is therefore a contour only when a real in-image neighbor differs from it, and a foreground region that fills the entire image produces no contour. This matches the legacy ITK filter.

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
