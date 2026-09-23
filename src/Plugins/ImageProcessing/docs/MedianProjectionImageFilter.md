# Median Projection Image Filter

Projects a scalar image along one axis by taking the median value along that axis.

## Group (Subgroup)

ImageProcessing (Projection)

## Description

Collapses the selected **Projection Dimension** of a scalar Image Geometry to a single voxel by taking, for each column of voxels running along that axis, the **median** value in the column. For an even-length column the upper-middle element is returned (the value at index `size / 2` after partial sorting), matching the legacy convention. The projected axis becomes size 1 in the output geometry; the other two axes keep their sizes. The output geometry's origin and spacing are copied verbatim from the input (they are not rescaled), and the output element type matches the input element type.

The **Projection Dimension** selects the collapsed axis: 0 = X, 1 = Y, 2 = Z.

When **Perform In-Place** is enabled, the input Image Geometry is replaced by the projected (collapsed) geometry containing the output array. Because the whole original geometry is replaced, any *other* arrays that were in its cell-data Attribute Matrix are discarded (they have the original, non-collapsed shape and cannot survive the projection); the filter emits a preflight warning listing them. Disable **Perform In-Place** to keep the original geometry: the input geometry is then preserved and the result is written to a newly created geometry named by **Created Image Geometry**, whose cell-data Attribute Matrix name is copied from the input geometry.

The input array must be single-component (scalar) and one of the supported types: `uint8`, `int16`, `uint16`, or `float32`. This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Median Projection Image Filter.

## Algorithm

The filter reduces each input pencil along the selected X, Y, or Z axis and writes one value to the corresponding slot in the collapsed image. In-memory and three-dimensional inputs use axis-aware slab staging. For a true two-dimensional image with an out-of-core endpoint, fitting Y pencils are read in bounded rectangular X tiles and gathered through at most eight fixed-budget worker pencils before upper-middle `nth_element`. If one pencil is larger than the 64 MiB working-memory budget, it is reduced with bounded external sorting, and a strided Y pencil is first transposed into generic out-of-core temporary storage. All datastore transfers are bulk operations.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
