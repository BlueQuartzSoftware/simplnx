# Binary Projection Image Filter

Projects a scalar image along one axis, marking each output voxel foreground if any voxel in its column matches the foreground value.

## Group (Subgroup)

ImageProcessing (Projection)

## Description

Collapses the selected **Projection Dimension** of a scalar Image Geometry to a single voxel. For each column of voxels running along that axis, the output voxel is set to the **Foreground Value** if *any* voxel in that column equals the **Foreground Value**; otherwise it is set to the **Background Value**. The projected axis becomes size 1 in the output geometry; the other two axes keep their sizes. The output geometry's origin and spacing are copied verbatim from the input (they are not rescaled), and the output element type matches the input element type.

The **Projection Dimension** selects the collapsed axis: 0 = X, 1 = Y, 2 = Z.

The **Foreground Value** and **Background Value** are supplied as floating-point numbers and cast to the input element type; the foreground equality test is performed in the input element type. This matches the legacy ITK Binary Projection accumulator, which initializes each column to background and switches it to foreground the first time a voxel equal to the foreground value is encountered.

When **Perform In-Place** is enabled, the input Image Geometry is replaced by the collapsed geometry containing the output array. Other Cell Data arrays are discarded. Preflight warning `-8343` lists the paths of the discarded objects. The contents of each listed object are also discarded.

**Copy Non-Cell Attribute Matrices** is disabled by default. With this option disabled, the projected geometry contains only the projected Cell Data. In-place projection discards other geometry children, including Cell Feature Data and Cell Ensemble Data. Warning `-8343` names these objects even when there are no other Cell Data arrays.

Enable **Copy Non-Cell Attribute Matrices** to keep every non-cell Attribute Matrix and its contents in the projected geometry. In-place projection moves these matrices without copying their arrays. They are then omitted from warning `-8343`. This optional preservation differs from DREAM3D 6.5.171, which discards these matrices during in-place projection. The preserved data is unchanged; feature measurements are not recalculated for the projection.

Disable **Perform In-Place** to keep the original geometry and all its data. The result is written to a new geometry named by **Created Image Geometry**. Its cell-data Attribute Matrix uses the same name as the input. When **Copy Non-Cell Attribute Matrices** is enabled, the new geometry also receives independent copies of the non-cell Attribute Matrices and their contents. String arrays in the copied Attribute Matrices currently share their values with the input; they are not independent copies.

The input array must be single-component (scalar) and one of the scalar numeric types (`int8`, `uint8`, `int16`, `uint16`, `int32`, `uint32`, `int64`, `uint64`, `float32`, or `float64`). This is an ITK-free, out-of-core-capable reimplementation of the legacy ITK Binary Projection Image Filter.

## Algorithm

The filter reduces each input pencil along the selected X, Y, or Z axis and writes one value to the corresponding slot in the collapsed image. A pencil emits the foreground value when any input equals the configured foreground; otherwise it emits the background value. In-memory and three-dimensional inputs use axis-aware slab staging. For a true two-dimensional image with an out-of-core endpoint, fixed-budget row blocks or X tiles and bulk datastore transfers keep resident cell data at or below 64 MiB.

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
