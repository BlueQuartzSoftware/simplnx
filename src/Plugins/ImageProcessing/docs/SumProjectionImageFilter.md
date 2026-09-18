# Sum Projection Image Filter

Projects a scalar image along one axis, setting each output voxel to the sum of its column.

## Group (Subgroup)

ImageProcessing (Projection)

## Description

Collapses the selected **Projection Dimension** of a scalar Image Geometry to a single voxel. For each column of voxels running along that axis, the output voxel is set to the **sum** of the voxels in that column. The projected axis becomes size 1 in the output geometry; the other two axes keep their sizes. The output geometry's origin and spacing are copied verbatim from the input (they are not rescaled).

The **Projection Dimension** selects the collapsed axis: 0 = X, 1 = Y, 2 = Z.

The output element type is always **Float64** (double), regardless of the input element type, matching the legacy ITK Sum Projection Image Filter (the sum of an integer column readily overflows a narrow integer type). The sum is accumulated with a Kahan-compensated running sum so that long columns do not lose precision.

When **Perform In-Place** is enabled, the input Image Geometry is replaced by the projected (collapsed) geometry containing the output array. Because the whole original geometry is replaced, any *other* arrays that were in its cell-data Attribute Matrix are discarded (they have the original, non-collapsed shape and cannot survive the projection); the filter emits a preflight warning listing them. Disable **Perform In-Place** to keep the original geometry: the input geometry is then preserved and the result is written to a newly created geometry named by **Created Image Geometry**, whose cell-data Attribute Matrix name is copied from the input geometry.

The input array must be single-component (scalar) and one of the scalar numeric types (`int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`, `int64`, `uint64`, `float32`, or `float64`). This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Sum Projection Image Filter.

## Algorithm

The filter reduces each input pencil along the selected X, Y, or Z axis and writes one value to the corresponding slot in the collapsed image. A pencil is scanned in axis order with a Kahan-compensated sum. In-memory and three-dimensional inputs use axis-aware slab staging. For a true two-dimensional image with an out-of-core endpoint, fixed-budget row blocks or X tiles and bulk datastore transfers keep resident cell data at or below 64 MiB.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
