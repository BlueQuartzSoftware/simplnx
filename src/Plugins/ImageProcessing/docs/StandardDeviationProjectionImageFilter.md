# Standard Deviation Projection Image Filter

Projects a scalar image along one axis, setting each output voxel to the standard deviation of its column.

## Group (Subgroup)

ImageProcessing (Projection)

## Description

Collapses the selected **Projection Dimension** of a scalar Image Geometry to a single voxel. For each column of voxels running along that axis, the output voxel is set to the **sample standard deviation** of the voxels in that column. The projected axis becomes size 1 in the output geometry; the other two axes keep their sizes. The output geometry's origin and spacing are copied verbatim from the input (they are not rescaled).

The **Projection Dimension** selects the collapsed axis: 0 = X, 1 = Y, 2 = Z.

The standard deviation is the *sample* standard deviation: the column mean is formed with the population divisor *N*, but the sum of squared deviations is divided by *N − 1* before taking the square root, matching the legacy ITK Standard Deviation Projection Image Filter. A column of length 1 (or a degenerate projected axis) yields 0. The output element type is always **Float64** (double), regardless of the input element type. The statistic is accumulated with Kahan-compensated running sums so that long columns do not lose precision.

When **Perform In-Place** is enabled, the input Image Geometry is replaced by the projected (collapsed) geometry containing the output array. Because the whole original geometry is replaced, any *other* arrays that were in its cell-data Attribute Matrix are discarded (they have the original, non-collapsed shape and cannot survive the projection); the filter emits a preflight warning listing them. Disable **Perform In-Place** to keep the original geometry: the input geometry is then preserved and the result is written to a newly created geometry named by **Created Image Geometry**, whose cell-data Attribute Matrix name is copied from the input geometry.

The input array must be single-component (scalar) and one of the scalar numeric types (`int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`, `int64`, `uint64`, `float32`, or `float64`). This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Standard Deviation Projection Image Filter.

## Algorithm

The filter reduces each input pencil along the selected X, Y, or Z axis and writes one value to the corresponding slot in the collapsed image. A pencil retains Kahan-compensated sum and squared-sum state and emits the sample standard deviation, with zero for one sample. In-memory and three-dimensional inputs use axis-aware slab staging. For a true two-dimensional image with an out-of-core endpoint, fixed-budget row blocks or X tiles and bulk datastore transfers keep resident cell data at or below 64 MiB.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
